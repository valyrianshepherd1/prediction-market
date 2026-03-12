#include "pm/repositories/OrderRepository.h"
#include "pm/repositories/Matching.h"

#include <drogon/HttpTypes.h>

#include <algorithm>
#include <cstdint>
#include <limits>
#include <memory>
#include <string>
#include <string_view>
#include <type_traits>

using drogon::orm::DbClientPtr;
using drogon::orm::DrogonDbException;
using drogon::orm::Result;

using TransactionPtr = std::shared_ptr<drogon::orm::Transaction>;

namespace {

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
    if (qtyMicros <= 0 || priceBp <= 0) {
        return 0;
    }

    constexpr std::int64_t denom = 10000;
    const auto maxValue = (std::numeric_limits<std::int64_t>::max)();

    const std::int64_t wholeUnits = qtyMicros / denom;
    const std::int64_t remainder = qtyMicros % denom;

    if (wholeUnits > maxValue / static_cast<std::int64_t>(priceBp)) {
        return maxValue;
    }

    const std::int64_t wholePart = wholeUnits * static_cast<std::int64_t>(priceBp);
    const std::int64_t fractionalPart =
        (remainder * static_cast<std::int64_t>(priceBp) + denom - 1) / denom;

    if (wholePart > maxValue - fractionalPart) {
        return maxValue;
    }

    return wholePart + fractionalPart;
}

// Compatibility wrapper:
// - If pm::matchTakerOrderInTx expects TransactionPtr -> we pass ctx->tx.
// - If it expects const drogon::orm::Transaction& -> we pass *ctx->tx.
// This fixes the exact clangd error you saw.
template <class T>
inline constexpr bool always_false_v = false;

template <class Tx>
void matchTakerCompat(const Tx &tx,
                      const std::string &takerOrderId,
                      const std::string &takerUserId,
                      const std::string &outcomeId,
                      const std::string &takerSide,
                      int takerPriceBp,
                      int maxTrades,
                      std::function<void()> onDone,
                      std::function<void(const DrogonDbException &)> onErr,
                      std::function<void(drogon::HttpStatusCode, std::string)> onBizErr) {
    if constexpr (requires {
                      pm::matchTakerOrderInTx(tx, takerOrderId, takerUserId, outcomeId, takerSide, takerPriceBp,
                                              maxTrades, onDone, onErr, onBizErr);
                  }) {
        pm::matchTakerOrderInTx(tx, takerOrderId, takerUserId, outcomeId, takerSide, takerPriceBp, maxTrades,
                                std::move(onDone), std::move(onErr), std::move(onBizErr));
    } else if constexpr (requires {
                             pm::matchTakerOrderInTx(*tx, takerOrderId, takerUserId, outcomeId, takerSide, takerPriceBp,
                                                     maxTrades, onDone, onErr, onBizErr);
                         }) {
        pm::matchTakerOrderInTx(*tx, takerOrderId, takerUserId, outcomeId, takerSide, takerPriceBp, maxTrades,
                                std::move(onDone), std::move(onErr), std::move(onBizErr));
    } else {
        static_assert(always_false_v<Tx>,
                      "pm::matchTakerOrderInTx signature mismatch. Expected TransactionPtr or Transaction& as first arg.");
    }
}

} // namespace

OrderRepository::OrderRepository(DbClientPtr db) : db_(std::move(db)) {}

void OrderRepository::createOrderWithReservation(const std::string &userId,
                                                 const std::string &outcomeId,
                                                 const std::string &side,
                                                 int priceBp,
                                                 std::int64_t qtyMicros,
                                                 std::function<void(OrderRow)> onOk,
                                                 std::function<void(const pm::ApiError &)> onBizErr,
                                                 std::function<void(const DrogonDbException &)> onErr) const {
    struct Completion {
        std::function<void(OrderRow)> onOk;
        std::function<void(const pm::ApiError &)> onBizErr;
        std::function<void(const DrogonDbException &)> onErr;
        OrderRow created;
    };

    struct Ctx {
        TransactionPtr tx;
        std::shared_ptr<Completion> completion;
        std::string userId;
        std::string outcomeId;
        std::string side;
        int priceBp{};
        std::int64_t qtyMicros{};
    };

    auto completion = std::make_shared<Completion>(
        Completion{std::move(onOk), std::move(onBizErr), std::move(onErr), {}});

    auto ctx = std::make_shared<Ctx>(
        Ctx{nullptr, completion, userId, outcomeId, side, priceBp, qtyMicros});

    db_->newTransactionAsync([ctx](const TransactionPtr &tx) {
        if (!tx) {
            return ctx->completion->onBizErr(
                {drogon::k503ServiceUnavailable, "db transaction unavailable"});
        }

        ctx->tx = tx;
        ctx->tx->setCommitCallback([completion = ctx->completion](bool committed) {
            if (committed) {
                completion->onOk(completion->created);
            } else {
                completion->onBizErr(
                    {drogon::k503ServiceUnavailable, "transaction commit failed"});
            }
        });

        auto dbFail = [ctx](const DrogonDbException &e) mutable {
            if (ctx->tx) ctx->tx->rollback();
            ctx->completion->onErr(e);
        };

        auto bizFail = [ctx](drogon::HttpStatusCode code, std::string msg) mutable {
            if (ctx->tx) ctx->tx->rollback();
            ctx->completion->onBizErr({code, std::move(msg)});
        };

        if (!(ctx->side == "BUY" || ctx->side == "SELL"))
            return bizFail(drogon::k400BadRequest, "side must be BUY or SELL");
        if (ctx->priceBp < 0 || ctx->priceBp > 10000)
            return bizFail(drogon::k400BadRequest, "price_bp must be in [0,10000]");
        if (ctx->qtyMicros <= 0)
            return bizFail(drogon::k400BadRequest, "qty_micros must be > 0");

        // 1) validate outcome exists + market is OPEN
        ctx->tx->execSqlAsync(
            "SELECT m.status AS status "
            "FROM outcomes o JOIN markets m ON m.id = o.market_id "
            "WHERE o.id = $1::uuid",
            [ctx, bizFail, dbFail](const Result &r) mutable {
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

                ctx->tx->execSqlAsync(
                    std::string(kInsert),
                    [ctx, bizFail, dbFail](const Result &ins) mutable {
                        if (ins.empty()) return bizFail(drogon::k500InternalServerError, "failed to create order");
                        ctx->completion->created = rowToOrder(ins, 0);

                        const bool takerIsBuy = (ctx->side == "BUY");

                        auto afterReserve = [ctx, dbFail]() mutable {
                            // 4) MATCH after reservation (both BUY and SELL)
                            matchTakerCompat(
                                ctx->tx,
                                ctx->completion->created.id,
                                ctx->userId,
                                ctx->outcomeId,
                                ctx->side,
                                ctx->priceBp,
                                100, // maxTrades per request
                                [ctx, dbFail]() mutable {
                                    // reload final order state
                                    ctx->tx->execSqlAsync(
                                        "SELECT id::text AS id, user_id::text AS user_id, outcome_id::text AS outcome_id, "
                                        "side, price_bp, qty_total_micros, qty_remaining_micros, status, "
                                        "created_at::text AS created_at, updated_at::text AS updated_at "
                                        "FROM orders WHERE id = $1::uuid",
                                        [ctx](const Result &r) mutable {
                                            if (!r.empty()) ctx->completion->created = rowToOrder(r, 0);
                                            ctx->tx.reset();
                                        },
                                        dbFail,
                                        ctx->completion->created.id);
                                },
                                dbFail,
                                [ctx](drogon::HttpStatusCode code, std::string msg) mutable {
                                    if (ctx->tx) ctx->tx->rollback();
                                    ctx->completion->onBizErr({code, std::move(msg)});
                                });
                        };

                        // 3) reserve cash or shares + ledger
                        if (takerIsBuy) {
                            const std::int64_t reserve = calcReserveCash(ctx->qtyMicros, ctx->priceBp);

                            // ensure wallet exists
                            ctx->tx->execSqlAsync(
                                "INSERT INTO wallets(user_id) VALUES($1::uuid) ON CONFLICT DO NOTHING",
                                [ctx, bizFail, dbFail, reserve, afterReserve](const Result &) mutable {
                                    // reserve funds
                                    ctx->tx->execSqlAsync(
                                        "UPDATE wallets "
                                        "SET available = available - $2, reserved = reserved + $2, updated_at = now() "
                                        "WHERE user_id = $1::uuid AND available >= $2",
                                        [ctx, bizFail, dbFail, reserve, afterReserve](const Result &u) mutable {
                                            if (u.affectedRows() != 1)
                                                return bizFail(drogon::k409Conflict, "insufficient funds");

                                            // ledger (RESERVE)
                                            ctx->tx->execSqlAsync(
                                                "INSERT INTO cash_ledger(user_id, kind, delta_available, delta_reserved, ref_type, ref_id) "
                                                "VALUES($1::uuid, 'RESERVE', $2, $3, 'ORDER', $4::uuid)",
                                                [afterReserve](const Result &) mutable { afterReserve(); },
                                                dbFail,
                                                ctx->userId,
                                                static_cast<long long>(-reserve),
                                                static_cast<long long>(reserve),
                                                ctx->completion->created.id);
                                        },
                                        dbFail,
                                        ctx->userId,
                                        static_cast<long long>(reserve));
                                },
                                dbFail,
                                ctx->userId);
                        } else {
                            // SELL: reserve shares
                            ctx->tx->execSqlAsync(
                                "INSERT INTO positions(user_id, outcome_id) VALUES($1::uuid, $2::uuid) ON CONFLICT DO NOTHING",
                                [ctx, bizFail, dbFail, afterReserve](const Result &) mutable {
                                    ctx->tx->execSqlAsync(
                                        "UPDATE positions "
                                        "SET shares_available = shares_available - $3, shares_reserved = shares_reserved + $3, updated_at = now() "
                                        "WHERE user_id = $1::uuid AND outcome_id = $2::uuid AND shares_available >= $3",
                                        [ctx, bizFail, dbFail, afterReserve](const Result &u) mutable {
                                            if (u.affectedRows() != 1) return bizFail(drogon::k409Conflict, "insufficient shares");

                                            ctx->tx->execSqlAsync(
                                                "INSERT INTO shares_ledger(user_id, outcome_id, kind, delta_available, delta_reserved, ref_type, ref_id) "
                                                "VALUES($1::uuid, $2::uuid, 'RESERVE', $3, $4, 'ORDER', $5::uuid)",
                                                [afterReserve](const Result &) mutable { afterReserve(); },
                                                dbFail,
                                                ctx->userId,
                                                ctx->outcomeId,
                                                static_cast<long long>(-ctx->qtyMicros),
                                                static_cast<long long>(ctx->qtyMicros),
                                                ctx->completion->created.id);
                                        },
                                        dbFail,
                                        ctx->userId,
                                        ctx->outcomeId,
                                        static_cast<long long>(ctx->qtyMicros));
                                },
                                dbFail,
                                ctx->userId,
                                ctx->outcomeId);
                        }
                    },
                    dbFail,
                    ctx->userId,
                    ctx->outcomeId,
                    ctx->side,
                    ctx->priceBp,
                    static_cast<long long>(ctx->qtyMicros));
            },
            dbFail,
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
        "LIMIT $2::int";

    static constexpr std::string_view kSelectSell =
        "SELECT id::text AS id, user_id::text AS user_id, outcome_id::text AS outcome_id, side, price_bp, "
        "qty_total_micros, qty_remaining_micros, status, created_at::text AS created_at, updated_at::text AS updated_at "
        "FROM orders "
        "WHERE outcome_id = $1::uuid AND side='SELL' AND status IN ('OPEN','PARTIALLY_FILLED') "
        "ORDER BY price_bp ASC, created_at, id "
        "LIMIT $2::int";

    auto db = db_;

    db->execSqlAsync(std::string(kSelectBuy),
        [db, ctx, outcomeId, depth](const Result &r) {
            ctx->book.buy.reserve(r.size());
            for (std::size_t i = 0; i < r.size(); ++i) {
                ctx->book.buy.push_back(rowToOrder(r, i));
            }

            db->execSqlAsync(std::string(kSelectSell),
                [ctx](const Result &r2) {
                    ctx->book.sell.reserve(r2.size());
                    for (std::size_t i = 0; i < r2.size(); ++i) {
                        ctx->book.sell.push_back(rowToOrder(r2, i));
                    }
                    ctx->onOk(std::move(ctx->book));
                },
                ctx->onErr,
                outcomeId, depth);
        },
        ctx->onErr,
        outcomeId, depth);
}

void OrderRepository::getOrderForUser(const std::string &userId,
                                      const std::string &orderId,
                                      std::function<void(OrderRow)> onOk,
                                      std::function<void(const pm::ApiError &)> onBizErr,
                                      std::function<void(const DrogonDbException &)> onErr) const {
    static constexpr std::string_view kSql =
        "SELECT id::text AS id, user_id::text AS user_id, outcome_id::text AS outcome_id, "
        "side, price_bp, qty_total_micros, qty_remaining_micros, status, "
        "created_at::text AS created_at, updated_at::text AS updated_at "
        "FROM orders "
        "WHERE id = $1::uuid AND user_id = $2::uuid";

    db_->execSqlAsync(
        std::string(kSql),
        [onOk = std::move(onOk), onBizErr = std::move(onBizErr)](const Result &r) mutable {
            if (r.empty()) {
                onBizErr({drogon::k404NotFound, "order not found"});
                return;
            }
            onOk(rowToOrder(r, 0));
        },
        std::move(onErr),
        orderId, userId);
}

void OrderRepository::listOrdersForUser(const std::string &userId,
                                        const std::optional<std::string> &status,
                                        int limit,
                                        int offset,
                                        std::function<void(std::vector<OrderRow>)> onOk,
                                        std::function<void(const DrogonDbException &)> onErr) const {
    if (limit <= 0) limit = 50;
    if (limit > 200) limit = 200;
    if (offset < 0) offset = 0;

    static constexpr std::string_view kSqlNoStatus =
        "SELECT id::text AS id, user_id::text AS user_id, outcome_id::text AS outcome_id, "
        "side, price_bp, qty_total_micros, qty_remaining_micros, status, "
        "created_at::text AS created_at, updated_at::text AS updated_at "
        "FROM orders "
        "WHERE user_id = $1::uuid "
        "ORDER BY created_at DESC, id "
        "LIMIT $2::int OFFSET $3::int";

    static constexpr std::string_view kSqlWithStatus =
        "SELECT id::text AS id, user_id::text AS user_id, outcome_id::text AS outcome_id, "
        "side, price_bp, qty_total_micros, qty_remaining_micros, status, "
        "created_at::text AS created_at, updated_at::text AS updated_at "
        "FROM orders "
        "WHERE user_id = $1::uuid AND status = $2 "
        "ORDER BY created_at DESC, id "
        "LIMIT $3::int OFFSET $4::int";

    auto mapRows = [onOk = std::move(onOk)](const Result &r) mutable {
        std::vector<OrderRow> out;
        out.reserve(r.size());
        for (std::size_t i = 0; i < r.size(); ++i) out.push_back(rowToOrder(r, i));
        onOk(std::move(out));
    };

    if (status && !status->empty()) {
        db_->execSqlAsync(std::string(kSqlWithStatus), std::move(mapRows), std::move(onErr),
                          userId, *status, limit, offset);
    } else {
        db_->execSqlAsync(std::string(kSqlNoStatus), std::move(mapRows), std::move(onErr),
                          userId, limit, offset);
    }
}

void OrderRepository::cancelOrderWithRelease(const std::string &userId,
                                             const std::string &orderId,
                                             std::function<void(OrderRow)> onOk,
                                             std::function<void(const pm::ApiError &)> onBizErr,
                                             std::function<void(const DrogonDbException &)> onErr) const {
    struct Ctx {
        TransactionPtr tx;
        std::string userId;
        std::string orderId;
        std::function<void(OrderRow)> onOk;
        std::function<void(const pm::ApiError &)> onBizErr;
        std::function<void(const DrogonDbException &)> onErr;

        OrderRow order;
        std::int64_t releaseAmount{0}; // cash for BUY or shares for SELL
    };

    auto ctx = std::make_shared<Ctx>(Ctx{
        nullptr,
        userId,
        orderId,
        std::move(onOk),
        std::move(onBizErr),
        std::move(onErr),
        {},
        0
    });

    db_->newTransactionAsync([ctx](const TransactionPtr &tx) {
        ctx->tx = tx;

        auto bizFail = [ctx](drogon::HttpStatusCode code, std::string msg) mutable {
            ctx->tx->rollback();
            ctx->onBizErr({code, std::move(msg)});
        };

        auto txFail = [ctx](const DrogonDbException &e) mutable {
            ctx->tx->rollback();
            ctx->onErr(e);
        };

        static constexpr std::string_view kSelectForUpdate =
            "SELECT id::text AS id, user_id::text AS user_id, outcome_id::text AS outcome_id, "
            "side, price_bp, qty_total_micros, qty_remaining_micros, status, "
            "created_at::text AS created_at, updated_at::text AS updated_at "
            "FROM orders "
            "WHERE id = $1::uuid AND user_id = $2::uuid "
            "FOR UPDATE";

        ctx->tx->execSqlAsync(
            std::string(kSelectForUpdate),
            [ctx, bizFail, txFail](const Result &r) mutable {
                if (r.empty()) return bizFail(drogon::k404NotFound, "order not found");
                ctx->order = rowToOrder(r, 0);

                if (!(ctx->order.status == "OPEN" || ctx->order.status == "PARTIALLY_FILLED")) {
                    return bizFail(drogon::k409Conflict, "order is not cancelable");
                }

                // how much to release
                if (ctx->order.qty_remaining_micros <= 0) {
                    ctx->releaseAmount = 0;
                } else if (ctx->order.side == "BUY") {
                    ctx->releaseAmount = calcReserveCash(ctx->order.qty_remaining_micros, ctx->order.price_bp);
                } else {
                    ctx->releaseAmount = ctx->order.qty_remaining_micros; // shares
                }

                static constexpr std::string_view kCancelReturning =
                    "UPDATE orders "
                    "SET status='CANCELED', updated_at=now() "
                    "WHERE id=$1::uuid AND user_id=$2::uuid "
                    "RETURNING id::text AS id, user_id::text AS user_id, outcome_id::text AS outcome_id, "
                    "side, price_bp, qty_total_micros, qty_remaining_micros, status, "
                    "created_at::text AS created_at, updated_at::text AS updated_at";

                ctx->tx->execSqlAsync(
                    std::string(kCancelReturning),
                    [ctx, bizFail, txFail](const Result &u) mutable {
                        if (u.empty()) return bizFail(drogon::k500InternalServerError, "failed to cancel order");
                        ctx->order = rowToOrder(u, 0);

                        // Nothing to release (avoid ledger CHECK delta!=0)
                        if (ctx->releaseAmount <= 0) {
                            ctx->onOk(std::move(ctx->order));
                            return;
                        }

                        if (ctx->order.side == "BUY") {
                            // wallets: reserved -> available
                            ctx->tx->execSqlAsync(
                                "UPDATE wallets "
                                "SET available = available + $2, reserved = reserved - $2, updated_at = now() "
                                "WHERE user_id = $1::uuid AND reserved >= $2",
                                [ctx, bizFail, txFail](const Result &w) mutable {
                                    if (w.affectedRows() != 1)
                                        return bizFail(drogon::k500InternalServerError, "wallet reserve release failed");

                                    ctx->tx->execSqlAsync(
                                        "INSERT INTO cash_ledger(user_id, kind, delta_available, delta_reserved, ref_type, ref_id) "
                                        "VALUES($1::uuid, 'RELEASE', $2, $3, 'ORDER', $4::uuid)",
                                        [ctx](const Result &) mutable {
                                            ctx->onOk(std::move(ctx->order));
                                        },
                                        txFail,
                                        ctx->userId,
                                        static_cast<std::int64_t>(ctx->releaseAmount),
                                        static_cast<std::int64_t>(-ctx->releaseAmount),
                                        ctx->orderId);
                                },
                                txFail,
                                ctx->userId,
                                static_cast<std::int64_t>(ctx->releaseAmount));
                        } else {
                            // positions: reserved -> available
                            ctx->tx->execSqlAsync(
                                "UPDATE positions "
                                "SET shares_available = shares_available + $3, shares_reserved = shares_reserved - $3, updated_at = now() "
                                "WHERE user_id = $1::uuid AND outcome_id = $2::uuid AND shares_reserved >= $3",
                                [ctx, bizFail, txFail](const Result &p) mutable {
                                    if (p.affectedRows() != 1)
                                        return bizFail(drogon::k500InternalServerError, "shares reserve release failed");

                                    ctx->tx->execSqlAsync(
                                        "INSERT INTO shares_ledger(user_id, outcome_id, kind, delta_available, delta_reserved, ref_type, ref_id) "
                                        "VALUES($1::uuid, $2::uuid, 'RELEASE', $3, $4, 'ORDER', $5::uuid)",
                                        [ctx](const Result &) mutable {
                                            ctx->onOk(std::move(ctx->order));
                                        },
                                        txFail,
                                        ctx->userId,
                                        ctx->order.outcome_id,
                                        static_cast<std::int64_t>(ctx->releaseAmount),
                                        static_cast<std::int64_t>(-ctx->releaseAmount),
                                        ctx->orderId);
                                },
                                txFail,
                                ctx->userId,
                                ctx->order.outcome_id,
                                static_cast<std::int64_t>(ctx->releaseAmount));
                        }
                    },
                    txFail,
                    ctx->orderId,
                    ctx->userId);
            },
            txFail,
            ctx->orderId,
            ctx->userId);
    });
}