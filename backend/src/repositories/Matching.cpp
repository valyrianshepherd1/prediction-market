#include "pm/repositories/Matching.h"

#include <json/json.h>
#include <json/writer.h>

#include <algorithm>
#include <cstdint>
#include <limits>
#include <memory>
#include <string_view>

using drogon::orm::DrogonDbException;
using drogon::orm::Result;
using TransactionPtr = std::shared_ptr<drogon::orm::Transaction>;

namespace pm {

namespace {

struct MakerRow {
    std::string id;
    std::string user_id;
    int price_bp{};
    std::int64_t qty_remaining_micros{};
};

MakerRow rowToMaker(const Result &r, std::size_t i) {
    MakerRow m;
    m.id = r[i]["id"].as<std::string>();
    m.user_id = r[i]["user_id"].as<std::string>();
    m.price_bp = r[i]["price_bp"].as<int>();
    m.qty_remaining_micros = r[i]["qty_remaining_micros"].as<std::int64_t>();
    return m;
}

// ceil(qty * price_bp / 10000)
std::int64_t ceilCost(std::int64_t qtyMicros, int priceBp) {
    __int128 num = static_cast<__int128>(qtyMicros) * static_cast<__int128>(priceBp);
    num = (num + 9999) / 10000;
    if (num > std::numeric_limits<std::int64_t>::max()) return std::numeric_limits<std::int64_t>::max();
    return static_cast<std::int64_t>(num);
}

std::string jsonToString(const Json::Value &v) {
    Json::StreamWriterBuilder b;
    b["indentation"] = "";
    return Json::writeString(b, v);
}

} // namespace

void matchTakerOrderInTx(const TransactionPtr &tx,
                         const std::string &takerOrderId,
                         const std::string &takerUserId,
                         const std::string &outcomeId,
                         const std::string &takerSide,
                         int takerPriceBp,
                         int maxTrades,
                         std::function<void()> onDone,
                         std::function<void(const DrogonDbException &)> onErr,
                         std::function<void(drogon::HttpStatusCode, std::string)> onBizErr) {
    struct Ctx {
        TransactionPtr tx;
        std::string takerOrderId;
        std::string takerUserId;
        std::string outcomeId;
        std::string takerSide;
        int takerPriceBp{};
        int maxTrades{};
        int tradesMade{0};
        std::int64_t takerRemaining{0};

        std::function<void()> onDone;
        std::function<void(const DrogonDbException &)> onErr;
        std::function<void(drogon::HttpStatusCode, std::string)> onBizErr;

        std::function<void()> step;
    };

    auto ctx = std::make_shared<Ctx>(Ctx{
        tx, takerOrderId, takerUserId, outcomeId, takerSide, takerPriceBp, maxTrades, 0, 0,
        std::move(onDone), std::move(onErr), std::move(onBizErr), {}
    });

    // 0) Lock taker order and read its remaining
    ctx->tx->execSqlAsync(
        "SELECT qty_remaining_micros "
        "FROM orders WHERE id = $1::uuid FOR UPDATE",
        [ctx](const Result &r) mutable {
            if (r.empty()) return ctx->onBizErr(drogon::k404NotFound, "taker order not found");
            ctx->takerRemaining = r[0]["qty_remaining_micros"].as<std::int64_t>();

            std::weak_ptr<Ctx> wctx = ctx;
            ctx->step = [wctx]() mutable {
                auto ctx = wctx.lock();
                if (!ctx) return;

                if (ctx->takerRemaining <= 0) return ctx->onDone();

                if (ctx->tradesMade >= ctx->maxTrades) return ctx->onDone();

                const bool takerIsBuy = (ctx->takerSide == "BUY");

                // 1) pick best maker on opposite side (price-time), lock it
                std::string sqlMaker;
                if (takerIsBuy) {
                    sqlMaker =
                        "SELECT id::text AS id, user_id::text AS user_id, price_bp, qty_remaining_micros "
                        "FROM orders "
                        "WHERE outcome_id=$1::uuid "
                        "  AND side='SELL' "
                        "  AND status IN ('OPEN','PARTIALLY_FILLED') "
                        "  AND price_bp <= $2 "
                        "ORDER BY price_bp ASC, created_at, id "
                        "LIMIT 1 "
                        "FOR UPDATE SKIP LOCKED";
                } else {
                    sqlMaker =
                        "SELECT id::text AS id, user_id::text AS user_id, price_bp, qty_remaining_micros "
                        "FROM orders "
                        "WHERE outcome_id=$1::uuid "
                        "  AND side='BUY' "
                        "  AND status IN ('OPEN','PARTIALLY_FILLED') "
                        "  AND price_bp >= $2 "
                        "ORDER BY price_bp DESC, created_at, id "
                        "LIMIT 1 "
                        "FOR UPDATE SKIP LOCKED";
                }

                ctx->tx->execSqlAsync(
                    sqlMaker,
                    [ctx](const Result &mr) mutable {
                        if (mr.empty()) return ctx->onDone();
                        const MakerRow maker = rowToMaker(mr, 0);

                        const std::int64_t qty = std::min<std::int64_t>(ctx->takerRemaining, maker.qty_remaining_micros);
                        const int tradePriceBp = maker.price_bp; // maker price

                        // Buyer/Seller resolution:
                        const std::string buyerUserId = (ctx->takerSide == "BUY") ? ctx->takerUserId : maker.user_id;
                        const std::string sellerUserId = (ctx->takerSide == "BUY") ? maker.user_id : ctx->takerUserId;

                        const int buyerOrderPriceBp = (ctx->takerSide == "BUY") ? ctx->takerPriceBp : maker.price_bp;

                        const std::int64_t cost = ceilCost(qty, tradePriceBp);
                        const std::int64_t buyerReservedUsed = ceilCost(qty, buyerOrderPriceBp);
                        const std::int64_t refund = (buyerReservedUsed > cost) ? (buyerReservedUsed - cost) : 0;

                        // 2) insert trade
                        ctx->tx->execSqlAsync(
                            "INSERT INTO trades(outcome_id, maker_user_id, taker_user_id, maker_order_id, taker_order_id, price_bp, qty_micros) "
                            "VALUES($1::uuid,$2::uuid,$3::uuid,$4::uuid,$5::uuid,$6,$7) "
                            "RETURNING id::text AS id",
                            [ctx, maker, qty, tradePriceBp, buyerUserId, sellerUserId, cost, buyerReservedUsed, refund](const Result &tr) mutable {
                                if (tr.empty()) return ctx->onBizErr(drogon::k500InternalServerError, "failed to insert trade");
                                const std::string tradeId = tr[0]["id"].as<std::string>();

                                // 3) update maker order remaining/status
                                ctx->tx->execSqlAsync(
                                    "UPDATE orders SET "
                                    " qty_remaining_micros = qty_remaining_micros - $2, "
                                    " status = CASE WHEN (qty_remaining_micros - $2)=0 THEN 'FILLED' ELSE 'PARTIALLY_FILLED' END, "
                                    " updated_at = now() "
                                    "WHERE id = $1::uuid AND qty_remaining_micros >= $2",
                                    [ctx, maker, qty, tradePriceBp, buyerUserId, sellerUserId, cost, buyerReservedUsed, refund, tradeId](const Result &u1) mutable {
                                        if (u1.affectedRows() != 1)
                                            return ctx->onBizErr(drogon::k500InternalServerError, "maker order update failed");

                                        // 4) update taker order remaining/status
                                        ctx->tx->execSqlAsync(
                                            "UPDATE orders SET "
                                            " qty_remaining_micros = qty_remaining_micros - $2, "
                                            " status = CASE "
                                            "   WHEN (qty_remaining_micros - $2)=0 THEN 'FILLED' "
                                            "   WHEN (qty_remaining_micros - $2) < qty_total_micros THEN 'PARTIALLY_FILLED' "
                                            "   ELSE status END, "
                                            " updated_at = now() "
                                            "WHERE id = $1::uuid AND qty_remaining_micros >= $2",
                                            [ctx, maker, qty, tradePriceBp, buyerUserId, sellerUserId, cost, buyerReservedUsed, refund, tradeId](const Result &u2) mutable {
                                                if (u2.affectedRows() != 1)
                                                    return ctx->onBizErr(drogon::k500InternalServerError, "taker order update failed");

                                                // 5) Ensure wallets exist (both)
                                                ctx->tx->execSqlAsync(
                                                    "INSERT INTO wallets(user_id) VALUES($1::uuid), ($2::uuid) ON CONFLICT DO NOTHING",
                                                    [ctx, maker, qty, tradePriceBp, buyerUserId, sellerUserId, cost, buyerReservedUsed, refund, tradeId](const Result &) mutable {

                                                        // 6) Ensure positions exist (both)
                                                        ctx->tx->execSqlAsync(
                                                            "INSERT INTO positions(user_id, outcome_id) "
                                                            "VALUES($1::uuid,$3::uuid), ($2::uuid,$3::uuid) "
                                                            "ON CONFLICT DO NOTHING",
                                                            [ctx, maker, qty, tradePriceBp, buyerUserId, sellerUserId, cost, buyerReservedUsed, refund, tradeId](const Result &) mutable {

                                                                // 7) Buyer wallet: reserved -= buyerReservedUsed; available += refund
                                                                ctx->tx->execSqlAsync(
                                                                    "UPDATE wallets SET reserved = reserved - $2, available = available + $3, updated_at = now() "
                                                                    "WHERE user_id = $1::uuid AND reserved >= $2",
                                                                    [ctx, maker, qty, tradePriceBp, buyerUserId, sellerUserId, cost, buyerReservedUsed, refund, tradeId](const Result &wb) mutable {
                                                                        if (wb.affectedRows() != 1)
                                                                            return ctx->onBizErr(drogon::k500InternalServerError, "buyer wallet update failed");

                                                                        // cash_ledger buyer: TRADE_DEBIT (cost from reserved)
                                                                        ctx->tx->execSqlAsync(
                                                                            "INSERT INTO cash_ledger(user_id, kind, delta_available, delta_reserved, ref_type, ref_id) "
                                                                            "VALUES($1::uuid, 'TRADE_DEBIT', 0, $2, 'TRADE', $3::uuid)",
                                                                            [ctx, maker, qty, tradePriceBp, buyerUserId, sellerUserId, cost, buyerReservedUsed, refund, tradeId](const Result &) mutable {

                                                                                // optional refund ledger
                                                                                auto afterRefund = [ctx, maker, qty, tradePriceBp, buyerUserId, sellerUserId, cost, tradeId]() mutable {
                                                                                    // 8) Buyer gets shares_available += qty
                                                                                    ctx->tx->execSqlAsync(
                                                                                        "UPDATE positions SET shares_available = shares_available + $3, updated_at = now() "
                                                                                        "WHERE user_id=$1::uuid AND outcome_id=$2::uuid",
                                                                                        [ctx, maker, qty, tradePriceBp, buyerUserId, sellerUserId, cost, tradeId](const Result &pb) mutable {
                                                                                            if (pb.affectedRows() != 1)
                                                                                                return ctx->onBizErr(drogon::k500InternalServerError, "buyer positions update failed");

                                                                                            ctx->tx->execSqlAsync(
                                                                                                "INSERT INTO shares_ledger(user_id, outcome_id, kind, delta_available, delta_reserved, ref_type, ref_id) "
                                                                                                "VALUES($1::uuid, $2::uuid, 'TRADE_CREDIT', $3, 0, 'TRADE', $4::uuid)",
                                                                                                [ctx, maker, qty, tradePriceBp, buyerUserId, sellerUserId, cost, tradeId](const Result &) mutable {

                                                                                                    // 9) Seller shares_reserved -= qty
                                                                                                    ctx->tx->execSqlAsync(
                                                                                                        "UPDATE positions SET shares_reserved = shares_reserved - $3, updated_at = now() "
                                                                                                        "WHERE user_id=$1::uuid AND outcome_id=$2::uuid AND shares_reserved >= $3",
                                                                                                        [ctx, maker, qty, tradePriceBp, buyerUserId, sellerUserId, cost, tradeId](const Result &ps) mutable {
                                                                                                            if (ps.affectedRows() != 1)
                                                                                                                return ctx->onBizErr(drogon::k500InternalServerError, "seller positions update failed");

                                                                                                            ctx->tx->execSqlAsync(
                                                                                                                "INSERT INTO shares_ledger(user_id, outcome_id, kind, delta_available, delta_reserved, ref_type, ref_id) "
                                                                                                                "VALUES($1::uuid, $2::uuid, 'TRADE_DEBIT', 0, $3, 'TRADE', $4::uuid)",
                                                                                                                [ctx, maker, qty, tradePriceBp, buyerUserId, sellerUserId, cost, tradeId](const Result &) mutable {

                                                                                                                    // 10) Seller wallet available += cost
                                                                                                                    ctx->tx->execSqlAsync(
                                                                                                                        "UPDATE wallets SET available = available + $2, updated_at = now() WHERE user_id = $1::uuid",
                                                                                                                        [ctx, maker, qty, tradePriceBp, buyerUserId, sellerUserId, cost, tradeId](const Result &ws) mutable {
                                                                                                                            if (ws.affectedRows() != 1)
                                                                                                                                return ctx->onBizErr(drogon::k500InternalServerError, "seller wallet update failed");

                                                                                                                            ctx->tx->execSqlAsync(
                                                                                                                                "INSERT INTO cash_ledger(user_id, kind, delta_available, delta_reserved, ref_type, ref_id) "
                                                                                                                                "VALUES($1::uuid, 'TRADE_CREDIT', $2, 0, 'TRADE', $3::uuid)",
                                                                                                                                [ctx, maker, qty, tradePriceBp, cost, tradeId](const Result &) mutable {

                                                                                                                                    // 11) outbox event (trade.created)
                                                                                                                                    Json::Value payload;
                                                                                                                                    payload["trade_id"] = tradeId;
                                                                                                                                    payload["outcome_id"] = ctx->outcomeId;
                                                                                                                                    payload["price_bp"] = tradePriceBp;
                                                                                                                                    payload["qty_micros"] = Json::Int64(qty);
                                                                                                                                    payload["maker_order_id"] = maker.id;
                                                                                                                                    payload["taker_order_id"] = ctx->takerOrderId;
                                                                                                                                    payload["maker_user_id"] = maker.user_id;
                                                                                                                                    payload["taker_user_id"] = ctx->takerUserId;

                                                                                                                                    const std::string payloadStr = jsonToString(payload);

                                                                                                                                    ctx->tx->execSqlAsync(
                                                                                                                                        "INSERT INTO outbox_events(event_type, aggregate_type, aggregate_id, payload) "
                                                                                                                                        "VALUES('trade.created', 'outcome', $1::uuid, $2::jsonb)",
                                                                                                                                        [ctx, qty](const Result &) mutable {
                                                                                                                                            ctx->takerRemaining -= qty;
                                                                                                                                            ctx->tradesMade += 1;
                                                                                                                                            ctx->step();
                                                                                                                                        },
                                                                                                                                        ctx->onErr,
                                                                                                                                        ctx->outcomeId, payloadStr);
                                                                                                                                },
                                                                                                                                ctx->onErr,
                                                                                                                                sellerUserId, static_cast<std::int64_t>(cost), tradeId);
                                                                                                                        },
                                                                                                                        ctx->onErr,
                                                                                                                        sellerUserId, static_cast<std::int64_t>(cost));
                                                                                                                },
                                                                                                                ctx->onErr,
                                                                                                                sellerUserId, ctx->outcomeId, static_cast<std::int64_t>(-qty), tradeId);
                                                                                                        },
                                                                                                        ctx->onErr,
                                                                                                        sellerUserId, ctx->outcomeId, static_cast<std::int64_t>(qty));
                                                                                                },
                                                                                                ctx->onErr,
                                                                                                buyerUserId, ctx->outcomeId, static_cast<std::int64_t>(qty), tradeId);
                                                                                        },
                                                                                        ctx->onErr,
                                                                                        buyerUserId, ctx->outcomeId, static_cast<std::int64_t>(qty));
                                                                                };

                                                                                if (refund > 0) {
                                                                                    ctx->tx->execSqlAsync(
                                                                                        "INSERT INTO cash_ledger(user_id, kind, delta_available, delta_reserved, ref_type, ref_id) "
                                                                                        "VALUES($1::uuid, 'RELEASE', $2, $3, 'TRADE', $4::uuid)",
                                                                                        [afterRefund](const Result &) mutable { afterRefund(); },
                                                                                        ctx->onErr,
                                                                                        buyerUserId,
                                                                                        static_cast<std::int64_t>(refund),
                                                                                        static_cast<std::int64_t>(-refund),
                                                                                        tradeId);
                                                                                } else {
                                                                                    afterRefund();
                                                                                }
                                                                            },
                                                                            ctx->onErr,
                                                                            buyerUserId, static_cast<std::int64_t>(-cost), tradeId);
                                                                    },
                                                                    ctx->onErr,
                                                                    buyerUserId,
                                                                    static_cast<std::int64_t>(buyerReservedUsed),
                                                                    static_cast<std::int64_t>(refund));
                                                            },
                                                            ctx->onErr,
                                                            buyerUserId, sellerUserId, ctx->outcomeId);
                                                    },
                                                    ctx->onErr,
                                                    buyerUserId, sellerUserId);
                                            },
                                            ctx->onErr,
                                            ctx->takerOrderId, static_cast<std::int64_t>(qty));
                                    },
                                    ctx->onErr,
                                    maker.id, static_cast<std::int64_t>(qty));
                            },
                            ctx->onErr,
                            ctx->outcomeId,
                            maker.user_id,
                            ctx->takerUserId,
                            maker.id,
                            ctx->takerOrderId,
                            tradePriceBp,
                            static_cast<std::int64_t>(qty));
                    },
                    ctx->onErr,
                    ctx->outcomeId, ctx->takerPriceBp);
            };

            ctx->step();
        },
        ctx->onErr,
        takerOrderId);
}

} // namespace pm