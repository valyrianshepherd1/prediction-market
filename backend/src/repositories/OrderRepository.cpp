#include "pm/repositories/OrderRepository.h"

#include <drogon/HttpTypes.h>
#include <json/json.h>
#include <json/writer.h>

#include <algorithm>
#include <cstdint>
#include <limits>
#include <memory>
#include <string_view>

using drogon::orm::DbClientPtr;
using drogon::orm::DrogonDbException;
using drogon::orm::Result;
using TransactionPtr = std::shared_ptr<drogon::orm::Transaction>;

namespace {

// ---------- mapping ----------
OrderRow rowToOrder(const Result &r, std::size_t i) {
    OrderRow o;
    o.id = r[i]["id"].as<std::string>();
    o.user_id = r[i]["user_id"].as<std::string>();
    o.outcome_id = r[i]["outcome_id"].as<std::string>();
    o.side = r[i]["side"].as<std::string>();
    o.price_bp = r[i]["price_bp"].as<int>();
    o.qty_total_micros = r[i]["qty_total_micros"].as<std::int64_t>();
    o.qty_remaining_micros = r[i]["qty_remaining_micros"].as<std::int64_t>();
    o.status = r[i]["status"].as<std::string>();
    o.created_at = r[i]["created_at"].as<std::string>();
    o.updated_at = r[i]["updated_at"].as<std::string>();
    return o;
}

// ceil(qty * price_bp / 10000)
std::int64_t calcReserveCash(std::int64_t qtyMicros, int priceBp) {
    __int128 num = static_cast<__int128>(qtyMicros) * static_cast<__int128>(priceBp);
    num = (num + 9999) / 10000;
    if (num > std::numeric_limits<std::int64_t>::max())
        return std::numeric_limits<std::int64_t>::max();
    return static_cast<std::int64_t>(num);
}

// helper: write json -> string
std::string jsonToString(const Json::Value &v) {
    Json::StreamWriterBuilder b;
    b["indentation"] = "";
    return Json::writeString(b, v);
}

struct MakerOrder {
    std::string id;
    std::string user_id;
    int price_bp{};
    std::int64_t qty_remaining_micros{};
};

MakerOrder rowToMaker(const Result &r, std::size_t i) {
    MakerOrder m;
    m.id = r[i]["id"].as<std::string>();
    m.user_id = r[i]["user_id"].as<std::string>();
    m.price_bp = r[i]["price_bp"].as<int>();
    m.qty_remaining_micros = r[i]["qty_remaining_micros"].as<std::int64_t>();
    return m;
}

} // namespace

// ---------------- OrderRepository ----------------

OrderRepository::OrderRepository(DbClientPtr db) : db_(std::move(db)) {}

void OrderRepository::createOrderWithReservation(const std::string &userId,
                                                const std::string &outcomeId,
                                                const std::string &side,
                                                int priceBp,
                                                std::int64_t qtyMicros,
                                                std::function<void(OrderRow)> onOk,
                                                std::function<void(const pm::ApiError &)> onBizErr,
                                                std::function<void(const DrogonDbException &)> onErr) const {
    struct Ctx {
        TransactionPtr tx;
        std::string userId;
        std::string outcomeId;
        std::string side;
        int priceBp{};
        std::int64_t qtyMicros{};
        std::function<void(OrderRow)> onOk;
        std::function<void(const pm::ApiError &)> onBizErr;
        std::function<void(const DrogonDbException &)> onErr;
        OrderRow created;
    };

    auto ctx = std::make_shared<Ctx>(Ctx{
        nullptr, userId, outcomeId, side, priceBp, qtyMicros,
        std::move(onOk), std::move(onBizErr), std::move(onErr), {}
    });

    db_->newTransactionAsync([ctx](const TransactionPtr &tx) {
        ctx->tx = tx;

        auto bizFail = [ctx](drogon::HttpStatusCode code, std::string msg) {
            ctx->tx->rollback();
            ctx->onBizErr({code, std::move(msg)});
        };

        // 1) validate outcome exists + market is OPEN
        ctx->tx->execSqlAsync(
            "SELECT m.status AS status "
            "FROM outcomes o JOIN markets m ON m.id = o.market_id "
            "WHERE o.id = $1::uuid",
            [ctx, bizFail](const Result &r) mutable {
                if (r.empty()) return bizFail(drogon::k404NotFound, "outcome not found");
                const auto st = r[0]["status"].as<std::string>();
                if (st != "OPEN") return bizFail(drogon::k409Conflict, "market is not OPEN");

                // 2) insert order
                static constexpr std::string_view kInsert =
                    "INSERT INTO orders (user_id, outcome_id, side, price_bp, qty_total_micros, qty_remaining_micros, status) "
                    "VALUES ($1::uuid, $2::uuid, $3, $4, $5, $5, 'OPEN') "
                    "RETURNING id::text AS id, user_id::text AS user_id, outcome_id::text AS outcome_id, "
                    "side, price_bp, qty_total_micros, qty_remaining_micros, status, "
                    "created_at::text AS created_at, updated_at::text AS updated_at";

                ctx->tx->execSqlAsync(std::string(kInsert),
                    [ctx, bizFail](const Result &ins) mutable {
                        if (ins.empty()) return bizFail(drogon::k500InternalServerError, "failed to create order");
                        ctx->created = rowToOrder(ins, 0);

                        const bool takerIsBuy = (ctx->side == "BUY");

                        // 3) reserve cash or shares + ledger
                        if (takerIsBuy) {
                            const std::int64_t reserve = calcReserveCash(ctx->qtyMicros, ctx->priceBp);

                            // ensure wallet exists
                            ctx->tx->execSqlAsync(
                                "INSERT INTO wallets(user_id) VALUES($1::uuid) ON CONFLICT DO NOTHING",
                                [ctx, bizFail, reserve](const Result &) mutable {
                                    // reserve funds
                                    ctx->tx->execSqlAsync(
                                        "UPDATE wallets "
                                        "SET available = available - $2, reserved = reserved + $2, updated_at = now() "
                                        "WHERE user_id = $1::uuid AND available >= $2",
                                        [ctx, bizFail, reserve](const Result &u) mutable {
                                            if (u.affectedRows() != 1)
                                                return bizFail(drogon::k409Conflict, "insufficient funds");

                                            // ledger (RESERVE)
                                            ctx->tx->execSqlAsync(
                                                "INSERT INTO cash_ledger(user_id, kind, delta_available, delta_reserved, ref_type, ref_id) "
                                                "VALUES($1::uuid, 'RESERVE', $2, $3, 'ORDER', $4::uuid)",
                                                [ctx, bizFail](const Result &) mutable {
                                                    // 4) MATCH after reservation
                                                    // -----------------------------------------
                                                    struct MatchCtx {
                                                        std::shared_ptr<Ctx> parent;
                                                        bool takerIsBuy{};
                                                        std::int64_t takerRemaining{};
                                                    };

                                                    auto mctx = std::make_shared<MatchCtx>();
                                                    mctx->parent = ctx;
                                                    mctx->takerIsBuy = (ctx->side == "BUY");
                                                    mctx->takerRemaining = ctx->created.qty_remaining_micros;

                                                    std::function<void()> step;
                                                    step = [mctx, &step, bizFail]() mutable {
                                                        auto &tx = mctx->parent->tx;
                                                        const auto &outcomeId = mctx->parent->outcomeId;

                                                        if (mctx->takerRemaining <= 0) {
                                                            // reload final order
                                                            tx->execSqlAsync(
                                                                "SELECT id::text AS id, user_id::text AS user_id, outcome_id::text AS outcome_id, "
                                                                "side, price_bp, qty_total_micros, qty_remaining_micros, status, "
                                                                "created_at::text AS created_at, updated_at::text AS updated_at "
                                                                "FROM orders WHERE id = $1::uuid",
                                                                [mctx](const Result &r) mutable {
                                                                    if (!r.empty()) mctx->parent->created = rowToOrder(r, 0);
                                                                    mctx->parent->onOk(std::move(mctx->parent->created));
                                                                },
                                                                mctx->parent->onErr,
                                                                mctx->parent->created.id);
                                                            return;
                                                        }

                                                        // pick best maker (opposite side) with price-crossing
                                                        const bool takerIsBuy = mctx->takerIsBuy;
                                                        const int takerPrice = mctx->parent->priceBp;

                                                        std::string sql;
                                                        if (takerIsBuy) {
                                                            sql =
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
                                                            sql =
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

                                                        tx->execSqlAsync(sql,
                                                            [mctx, &step, bizFail](const Result &r) mutable {
                                                                if (r.empty()) {
                                                                    // no crossing liquidity -> done
                                                                    auto &tx2 = mctx->parent->tx;
                                                                    tx2->execSqlAsync(
                                                                        "SELECT id::text AS id, user_id::text AS user_id, outcome_id::text AS outcome_id, "
                                                                        "side, price_bp, qty_total_micros, qty_remaining_micros, status, "
                                                                        "created_at::text AS created_at, updated_at::text AS updated_at "
                                                                        "FROM orders WHERE id = $1::uuid",
                                                                        [mctx](const Result &rr) mutable {
                                                                            if (!rr.empty()) mctx->parent->created = rowToOrder(rr, 0);
                                                                            mctx->parent->onOk(std::move(mctx->parent->created));
                                                                        },
                                                                        mctx->parent->onErr,
                                                                        mctx->parent->created.id);
                                                                    return;
                                                                }

                                                                const MakerOrder maker = rowToMaker(r, 0);

                                                                const std::int64_t qty =
                                                                    std::min<std::int64_t>(mctx->takerRemaining, maker.qty_remaining_micros);

                                                                // trade price = maker price
                                                                const int tradePrice = maker.price_bp;

                                                                const std::string &takerOrderId = mctx->parent->created.id;
                                                                const std::string &takerUserId = mctx->parent->userId;
                                                                const std::string &outcomeId = mctx->parent->outcomeId;

                                                                // Insert trade, get trade id
                                                                mctx->parent->tx->execSqlAsync(
                                                                    "INSERT INTO trades(outcome_id, maker_user_id, taker_user_id, maker_order_id, taker_order_id, price_bp, qty_micros) "
                                                                    "VALUES($1::uuid,$2::uuid,$3::uuid,$4::uuid,$5::uuid,$6,$7) "
                                                                    "RETURNING id::text AS id",
                                                                    [mctx, &step, bizFail, maker, qty, tradePrice, takerOrderId, takerUserId, outcomeId](const Result &tr) mutable {
                                                                        if (tr.empty()) return bizFail(drogon::k500InternalServerError, "failed to insert trade");
                                                                        const std::string tradeId = tr[0]["id"].as<std::string>();

                                                                        auto &tx = mctx->parent->tx;

                                                                        // Update maker order remaining/status
                                                                        tx->execSqlAsync(
                                                                            "UPDATE orders SET "
                                                                            " qty_remaining_micros = qty_remaining_micros - $2, "
                                                                            " status = CASE WHEN (qty_remaining_micros - $2)=0 THEN 'FILLED' ELSE 'PARTIALLY_FILLED' END, "
                                                                            " updated_at = now() "
                                                                            "WHERE id = $1::uuid AND qty_remaining_micros >= $2",
                                                                            [mctx, &step, bizFail, maker, qty, tradePrice, tradeId, takerOrderId, takerUserId, outcomeId](const Result &u1) mutable {
                                                                                if (u1.affectedRows() != 1)
                                                                                    return bizFail(drogon::k500InternalServerError, "maker order update failed");

                                                                                // Update taker order remaining/status
                                                                                mctx->parent->tx->execSqlAsync(
                                                                                    "UPDATE orders SET "
                                                                                    " qty_remaining_micros = qty_remaining_micros - $2, "
                                                                                    " status = CASE "
                                                                                    "   WHEN (qty_remaining_micros - $2)=0 THEN 'FILLED' "
                                                                                    "   WHEN (qty_remaining_micros - $2) < qty_total_micros THEN 'PARTIALLY_FILLED' "
                                                                                    "   ELSE status END, "
                                                                                    " updated_at = now() "
                                                                                    "WHERE id = $1::uuid AND qty_remaining_micros >= $2",
                                                                                    [mctx, &step, bizFail, maker, qty, tradePrice, tradeId, takerOrderId, takerUserId, outcomeId](const Result &u2) mutable {
                                                                                        if (u2.affectedRows() != 1)
                                                                                            return bizFail(drogon::k500InternalServerError, "taker order update failed");

                                                                                        // Apply balances/positions + ledgers
                                                                                        const bool takerIsBuy = mctx->takerIsBuy;

                                                                                        // Determine buyer/seller
                                                                                        std::string buyerUserId, sellerUserId;
                                                                                        int buyerOrderPriceBp{};
                                                                                        if (takerIsBuy) {
                                                                                            buyerUserId = takerUserId;
                                                                                            sellerUserId = maker.user_id;
                                                                                            buyerOrderPriceBp = mctx->parent->priceBp; // taker order price
                                                                                        } else {
                                                                                            buyerUserId = maker.user_id;
                                                                                            sellerUserId = takerUserId;
                                                                                            buyerOrderPriceBp = maker.price_bp; // maker BUY price (trade price)
                                                                                        }

                                                                                        const std::int64_t cost = calcReserveCash(qty, tradePrice);
                                                                                        const std::int64_t buyerReserveUsed = calcReserveCash(qty, buyerOrderPriceBp);
                                                                                        const std::int64_t refund = (buyerReserveUsed > cost) ? (buyerReserveUsed - cost) : 0;

                                                                                        // ensure rows exist
                                                                                        mctx->parent->tx->execSqlAsync(
                                                                                            "INSERT INTO wallets(user_id) VALUES($1::uuid) ON CONFLICT DO NOTHING",
                                                                                            [mctx, &step, bizFail, maker, qty, tradePrice, tradeId, outcomeId, buyerUserId, sellerUserId, cost, buyerReserveUsed, refund](const Result &) mutable {
                                                                                                mctx->parent->tx->execSqlAsync(
                                                                                                    "INSERT INTO wallets(user_id) VALUES($1::uuid) ON CONFLICT DO NOTHING",
                                                                                                    [mctx, &step, bizFail, maker, qty, tradePrice, tradeId, outcomeId, buyerUserId, sellerUserId, cost, buyerReserveUsed, refund](const Result &) mutable {

                                                                                                        // ensure positions for buyer/seller
                                                                                                        mctx->parent->tx->execSqlAsync(
                                                                                                            "INSERT INTO positions(user_id, outcome_id) VALUES($1::uuid, $2::uuid) ON CONFLICT DO NOTHING",
                                                                                                            [mctx, &step, bizFail, maker, qty, tradePrice, tradeId, outcomeId, buyerUserId, sellerUserId, cost, buyerReserveUsed, refund](const Result &) mutable {
                                                                                                                mctx->parent->tx->execSqlAsync(
                                                                                                                    "INSERT INTO positions(user_id, outcome_id) VALUES($1::uuid, $2::uuid) ON CONFLICT DO NOTHING",
                                                                                                                    [mctx, &step, bizFail, maker, qty, tradePrice, tradeId, outcomeId, buyerUserId, sellerUserId, cost, buyerReserveUsed, refund](const Result &) mutable {

                                                                                                                        auto &tx = mctx->parent->tx;

                                                                                                                        // Buyer: reserved -= buyerReserveUsed; available += refund
                                                                                                                        tx->execSqlAsync(
                                                                                                                            "UPDATE wallets "
                                                                                                                            "SET reserved = reserved - $2, available = available + $3, updated_at = now() "
                                                                                                                            "WHERE user_id = $1::uuid AND reserved >= $2",
                                                                                                                            [mctx, &step, bizFail, maker, qty, tradePrice, tradeId, outcomeId, buyerUserId, sellerUserId, cost, buyerReserveUsed, refund](const Result &wb) mutable {
                                                                                                                                if (wb.affectedRows() != 1)
                                                                                                                                    return bizFail(drogon::k500InternalServerError, "buyer wallet update failed");

                                                                                                                                // cash_ledger buyer: TRADE_DEBIT (cost)
                                                                                                                                mctx->parent->tx->execSqlAsync(
                                                                                                                                    "INSERT INTO cash_ledger(user_id, kind, delta_available, delta_reserved, ref_type, ref_id) "
                                                                                                                                    "VALUES($1::uuid, 'TRADE_DEBIT', 0, $2, 'TRADE', $3::uuid)",
                                                                                                                                    [mctx, &step, bizFail, maker, qty, tradePrice, tradeId, outcomeId, buyerUserId, sellerUserId, cost, buyerReserveUsed, refund](const Result &) mutable {

                                                                                                                                        // optional: refund (if BUY taker crossed lower ask)
                                                                                                                                        auto afterBuyer = [mctx, &step, bizFail, maker, qty, tradePrice, tradeId, outcomeId, buyerUserId, sellerUserId, cost]() mutable {
                                                                                                                                            // Buyer gets shares_available += qty
                                                                                                                                            mctx->parent->tx->execSqlAsync(
                                                                                                                                                "UPDATE positions SET shares_available = shares_available + $3, updated_at = now() "
                                                                                                                                                "WHERE user_id=$1::uuid AND outcome_id=$2::uuid",
                                                                                                                                                [mctx, &step, bizFail, maker, qty, tradePrice, tradeId, outcomeId, buyerUserId, sellerUserId, cost](const Result &pb) mutable {
                                                                                                                                                    if (pb.affectedRows() != 1)
                                                                                                                                                        return bizFail(drogon::k500InternalServerError, "buyer positions update failed");

                                                                                                                                                    mctx->parent->tx->execSqlAsync(
                                                                                                                                                        "INSERT INTO shares_ledger(user_id, outcome_id, kind, delta_available, delta_reserved, ref_type, ref_id) "
                                                                                                                                                        "VALUES($1::uuid, $2::uuid, 'TRADE_CREDIT', $3, 0, 'TRADE', $4::uuid)",
                                                                                                                                                        [mctx, &step, bizFail, maker, qty, tradePrice, tradeId, outcomeId, buyerUserId, sellerUserId, cost](const Result &) mutable {

                                                                                                                                                            // Seller: reserved shares -= qty; wallet available += cost
                                                                                                                                                            mctx->parent->tx->execSqlAsync(
                                                                                                                                                                "UPDATE positions SET shares_reserved = shares_reserved - $3, updated_at = now() "
                                                                                                                                                                "WHERE user_id=$1::uuid AND outcome_id=$2::uuid AND shares_reserved >= $3",
                                                                                                                                                                [mctx, &step, bizFail, maker, qty, tradePrice, tradeId, outcomeId, buyerUserId, sellerUserId, cost](const Result &ps) mutable {
                                                                                                                                                                    if (ps.affectedRows() != 1)
                                                                                                                                                                        return bizFail(drogon::k500InternalServerError, "seller positions update failed");

                                                                                                                                                                    mctx->parent->tx->execSqlAsync(
                                                                                                                                                                        "INSERT INTO shares_ledger(user_id, outcome_id, kind, delta_available, delta_reserved, ref_type, ref_id) "
                                                                                                                                                                        "VALUES($1::uuid, $2::uuid, 'TRADE_DEBIT', 0, $3, 'TRADE', $4::uuid)",
                                                                                                                                                                        [mctx, &step, bizFail, maker, qty, tradePrice, tradeId, outcomeId, buyerUserId, sellerUserId, cost](const Result &) mutable {

                                                                                                                                                                            mctx->parent->tx->execSqlAsync(
                                                                                                                                                                                "UPDATE wallets SET available = available + $2, updated_at = now() WHERE user_id = $1::uuid",
                                                                                                                                                                                [mctx, &step, bizFail, maker, qty, tradePrice, tradeId, outcomeId, buyerUserId, sellerUserId, cost](const Result &ws) mutable {
                                                                                                                                                                                    if (ws.affectedRows() != 1)
                                                                                                                                                                                        return bizFail(drogon::k500InternalServerError, "seller wallet update failed");

                                                                                                                                                                                    mctx->parent->tx->execSqlAsync(
                                                                                                                                                                                        "INSERT INTO cash_ledger(user_id, kind, delta_available, delta_reserved, ref_type, ref_id) "
                                                                                                                                                                                        "VALUES($1::uuid, 'TRADE_CREDIT', $2, 0, 'TRADE', $3::uuid)",
                                                                                                                                                                                        [mctx, &step, bizFail, maker, qty, tradePrice, tradeId, outcomeId, buyerUserId, sellerUserId, cost](const Result &) mutable {

                                                                                                                                                                                            // outbox event (for listener/ws)
                                                                                                                                                                                            Json::Value payload;
                                                                                                                                                                                            payload["trade_id"] = tradeId;
                                                                                                                                                                                            payload["outcome_id"] = outcomeId;
                                                                                                                                                                                            payload["price_bp"] = tradePrice;
                                                                                                                                                                                            payload["qty_micros"] = Json::Int64(qty);
                                                                                                                                                                                            payload["maker_order_id"] = maker.id;
                                                                                                                                                                                            payload["taker_order_id"] = mctx->parent->created.id;
                                                                                                                                                                                            payload["maker_user_id"] = maker.user_id;
                                                                                                                                                                                            payload["taker_user_id"] = mctx->parent->userId;

                                                                                                                                                                                            const std::string payloadStr = jsonToString(payload);

                                                                                                                                                                                            mctx->parent->tx->execSqlAsync(
                                                                                                                                                                                                "INSERT INTO outbox_events(event_type, aggregate_type, aggregate_id, payload) "
                                                                                                                                                                                                "VALUES('trade.created', 'outcome', $1::uuid, $2::jsonb)",
                                                                                                                                                                                                [mctx, &step, qty](const Result &) mutable {
                                                                                                                                                                                                    mctx->takerRemaining -= qty;
                                                                                                                                                                                                    step();
                                                                                                                                                                                                },
                                                                                                                                                                                                mctx->parent->onErr,
                                                                                                                                                                                                outcomeId, payloadStr);
                                                                                                                                                                                        },
                                                                                                                                                                                        mctx->parent->onErr,
                                                                                                                                                                                        sellerUserId, static_cast<std::int64_t>(cost), tradeId);
                                                                                                                                                                                },
                                                                                                                                                                                mctx->parent->onErr,
                                                                                                                                                                                sellerUserId, static_cast<std::int64_t>(cost));
                                                                                                                                                                        },
                                                                                                                                                                        mctx->parent->onErr,
                                                                                                                                                                        sellerUserId, outcomeId, static_cast<std::int64_t>(-qty), tradeId);
                                                                                                                                                                },
                                                                                                                                                                mctx->parent->onErr,
                                                                                                                                                                sellerUserId, outcomeId, static_cast<std::int64_t>(qty));
                                                                                                                                                        },
                                                                                                                                                        mctx->parent->onErr,
                                                                                                                                                        buyerUserId, outcomeId, static_cast<std::int64_t>(qty), tradeId);
                                                                                                                                                },
                                                                                                                                                mctx->parent->onErr,
                                                                                                                                                buyerUserId, outcomeId, static_cast<std::int64_t>(qty));
                                                                                                                                        };

                                                                                                                                        if (refund > 0) {
                                                                                                                                            mctx->parent->tx->execSqlAsync(
                                                                                                                                                "INSERT INTO cash_ledger(user_id, kind, delta_available, delta_reserved, ref_type, ref_id) "
                                                                                                                                                "VALUES($1::uuid, 'RELEASE', $2, $3, 'TRADE', $4::uuid)",
                                                                                                                                                [afterBuyer](const Result &) mutable { afterBuyer(); },
                                                                                                                                                mctx->parent->onErr,
                                                                                                                                                buyerUserId,
                                                                                                                                                static_cast<std::int64_t>(refund),
                                                                                                                                                static_cast<std::int64_t>(-refund),
                                                                                                                                                tradeId);
                                                                                                                                        } else {
                                                                                                                                            afterBuyer();
                                                                                                                                        }
                                                                                                                                    },
                                                                                                                                    mctx->parent->onErr,
                                                                                                                                    buyerUserId, static_cast<std::int64_t>(-cost), tradeId);
                                                                                                                            },
                                                                                                                            mctx->parent->onErr,
                                                                                                                            buyerUserId,
                                                                                                                            static_cast<std::int64_t>(buyerReserveUsed),
                                                                                                                            static_cast<std::int64_t>(refund));
                                                                                                                    },
                                                                                                                    mctx->parent->onErr,
                                                                                                                    sellerUserId, outcomeId);
                                                                                                            },
                                                                                                            mctx->parent->onErr,
                                                                                                            buyerUserId, outcomeId);
                                                                                                    },
                                                                                                    mctx->parent->onErr,
                                                                                                    sellerUserId);
                                                                                            },
                                                                                            mctx->parent->onErr,
                                                                                            buyerUserId);
                                                                                    },
                                                                                    mctx->parent->onErr,
                                                                                    takerOrderId, static_cast<std::int64_t>(qty));
                                                                            },
                                                                            mctx->parent->onErr,
                                                                            maker.id, static_cast<std::int64_t>(qty));
                                                                    },
                                                                    mctx->parent->onErr,
                                                                    outcomeId,
                                                                    maker.user_id,  // maker_user_id
                                                                    takerUserId,    // taker_user_id
                                                                    maker.id,
                                                                    takerOrderId,
                                                                    tradePrice,
                                                                    static_cast<std::int64_t>(qty));
                                                            },
                                                            mctx->parent->onErr,
                                                            outcomeId, takerPrice);
                                                    };

                                                    step();
                                                    // -----------------------------------------
                                                },
                                                ctx->onErr,
                                                ctx->userId,
                                                static_cast<std::int64_t>(-reserve),
                                                static_cast<std::int64_t>(reserve),
                                                ctx->created.id);
                                        },
                                        ctx->onErr,
                                        ctx->userId,
                                        static_cast<std::int64_t>(reserve));
                                },
                                ctx->onErr,
                                ctx->userId);
                        } else {
                            // SELL: reserve shares
                            ctx->tx->execSqlAsync(
                                "INSERT INTO positions(user_id, outcome_id) VALUES($1::uuid, $2::uuid) ON CONFLICT DO NOTHING",
                                [ctx, bizFail](const Result &) mutable {
                                    ctx->tx->execSqlAsync(
                                        "UPDATE positions "
                                        "SET shares_available = shares_available - $3, shares_reserved = shares_reserved + $3, updated_at = now() "
                                        "WHERE user_id = $1::uuid AND outcome_id = $2::uuid AND shares_available >= $3",
                                        [ctx, bizFail](const Result &u) mutable {
                                            if (u.affectedRows() != 1) return bizFail(drogon::k409Conflict, "insufficient shares");

                                            ctx->tx->execSqlAsync(
                                                "INSERT INTO shares_ledger(user_id, outcome_id, kind, delta_available, delta_reserved, ref_type, ref_id) "
                                                "VALUES($1::uuid, $2::uuid, 'RESERVE', $3, $4, 'ORDER', $5::uuid)",
                                                [ctx, bizFail](const Result &) mutable {
                                                    // MATCH after reservation (SELL taker)
                                                    // TODO: вынести matching в отдельную функцию и здесь вызвать, пока return
                                                    ctx->onOk(std::move(ctx->created));
                                                },
                                                ctx->onErr,
                                                ctx->userId,
                                                ctx->outcomeId,
                                                static_cast<std::int64_t>(-ctx->qtyMicros),
                                                static_cast<std::int64_t>(ctx->qtyMicros),
                                                ctx->created.id);
                                        },
                                        ctx->onErr,
                                        ctx->userId,
                                        ctx->outcomeId,
                                        static_cast<std::int64_t>(ctx->qtyMicros));
                                },
                                ctx->onErr,
                                ctx->userId,
                                ctx->outcomeId);
                        }
                    },
                    ctx->onErr,
                    ctx->userId,
                    ctx->outcomeId,
                    ctx->side,
                    ctx->priceBp,
                    static_cast<std::int64_t>(ctx->qtyMicros));
            },
            ctx->onErr,
            ctx->outcomeId);
    });
}

void OrderRepository::getOrderBook(const std::string &outcomeId,
                                  int depth,
                                  std::function<void(OrderBook)> onOk,
                                  std::function<void(const DrogonDbException &)> onErr) const {
    if (depth <= 0) depth = 50;
    if (depth > 200) depth = 200;

    struct Ctx {
        OrderBook book;
        std::function<void(OrderBook)> onOk;
        std::function<void(const DrogonDbException &)> onErr;
    };
    auto ctx = std::make_shared<Ctx>(Ctx{{}, std::move(onOk), std::move(onErr)});

    static constexpr std::string_view kSelectBuy =
        "SELECT id::text AS id, user_id::text AS user_id, outcome_id::text AS outcome_id, side, price_bp, "
        "qty_total_micros, qty_remaining_micros, status, created_at::text AS created_at, updated_at::text AS updated_at "
        "FROM orders "
        "WHERE outcome_id = $1::uuid AND side='BUY' AND status IN ('OPEN','PARTIALLY_FILLED') "
        "ORDER BY price_bp DESC, created_at, id "
        "LIMIT $2";

    static constexpr std::string_view kSelectSell =
        "SELECT id::text AS id, user_id::text AS user_id, outcome_id::text AS outcome_id, side, price_bp, "
        "qty_total_micros, qty_remaining_micros, status, created_at::text AS created_at, updated_at::text AS updated_at "
        "FROM orders "
        "WHERE outcome_id = $1::uuid AND side='SELL' AND status IN ('OPEN','PARTIALLY_FILLED') "
        "ORDER BY price_bp ASC, created_at, id "
        "LIMIT $2";

    db_->execSqlAsync(std::string(kSelectBuy),
        [this, ctx, outcomeId, depth](const Result &r) {
            ctx->book.buy.reserve(r.size());
            for (std::size_t i = 0; i < r.size(); ++i) ctx->book.buy.push_back(rowToOrder(r, i));

            db_->execSqlAsync(std::string(kSelectSell),
                [ctx](const Result &r2) {
                    ctx->book.sell.reserve(r2.size());
                    for (std::size_t i = 0; i < r2.size(); ++i) ctx->book.sell.push_back(rowToOrder(r2, i));
                    ctx->onOk(std::move(ctx->book));
                },
                ctx->onErr,
                outcomeId, depth);
        },
        ctx->onErr,
        outcomeId, depth);
}