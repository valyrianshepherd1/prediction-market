#include "pm/repositories/OrderRepository.h"

#include <drogon/orm/DbClient.h>
#include <limits>
#include <memory>

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
        o.qty_total_micros = r[i]["qty_total_micros"].as<long long>();
        o.qty_remaining_micros = r[i]["qty_remaining_micros"].as<long long>();
        o.status = r[i]["status"].as<std::string>();
        o.created_at = r[i]["created_at"].as<std::string>();
        o.updated_at = r[i]["updated_at"].as<std::string>();
        return o;
    }

    // ceil(qty * price_bp / 10000)
    std::int64_t calcReserveCash(std::int64_t qtyMicros, int priceBp) {
        __int128 num = static_cast<__int128>(qtyMicros) * static_cast<__int128>(priceBp);
        num = (num + 9999) / 10000;
        if (num > std::numeric_limits<std::int64_t>::max()) return std::numeric_limits<std::int64_t>::max();
        return static_cast<std::int64_t>(num);
    }
} // namespace

OrderRepository::OrderRepository(DbClientPtr db) : db_(std::move(db)) {
}

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
            ctx->tx->rollback(); // important: Transaction commits on destruction
            ctx->onBizErr({code, std::move(msg)});
        };

        // 1) validate outcome exists + market is OPEN
        tx->execSqlAsync(
            "SELECT m.status AS status "
            "FROM outcomes o JOIN markets m ON m.id = o.market_id "
            "WHERE o.id = $1::uuid",
            [ctx, bizFail](const Result &r) mutable {
                if (r.empty()) return bizFail(drogon::k404NotFound, "outcome not found");
                const auto st = r[0]["status"].as<std::string>();
                if (st != "OPEN") return bizFail(drogon::k409Conflict, "market is not OPEN");

                // 2) insert order (within tx)
                static constexpr std::string_view kInsert =
                        "INSERT INTO orders (user_id, outcome_id, side, price_bp, qty_total_micros, qty_remaining_micros, status) "
                        "VALUES ($1::uuid, $2::uuid, $3, $4, $5, $5, 'OPEN') "
                        "RETURNING id::text AS id, user_id::text AS user_id, outcome_id::text AS outcome_id, "
                        "side, price_bp, qty_total_micros, qty_remaining_micros, status, "
                        "created_at::text AS created_at, updated_at::text AS updated_at";

                ctx->tx->execSqlAsync(
                    std::string(kInsert),
                    [ctx, bizFail](const Result &ins) mutable {
                        if (ins.empty()) return bizFail(drogon::k500InternalServerError, "failed to create order");
                        ctx->created = rowToOrder(ins, 0);

                        // 3) reserve cash or shares + ledger
                        const bool isBuy = (ctx->side == "BUY");
                        if (isBuy) {
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
                                            if (u.affectedRows() != 1) {
                                                return bizFail(drogon::k409Conflict, "insufficient funds");
                                            }
                                            // ledger
                                            ctx->tx->execSqlAsync(
                                                "INSERT INTO cash_ledger(user_id, kind, delta_available, delta_reserved, ref_type, ref_id) "
                                                "VALUES($1::uuid, 'RESERVE', $2, $3, 'ORDER', $4::uuid)",
                                                [ctx](const Result &) mutable {
                                                    ctx->onOk(std::move(ctx->created));
                                                },
                                                ctx->onErr,
                                                ctx->userId,
                                                static_cast<long long>(-reserve),
                                                static_cast<long long>(reserve),
                                                ctx->created.id);
                                        },
                                        ctx->onErr,
                                        ctx->userId,
                                        static_cast<long long>(reserve));
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
                                            if (u.affectedRows() != 1) {
                                                return bizFail(drogon::k409Conflict, "insufficient shares");
                                            }
                                            ctx->tx->execSqlAsync(
                                                "INSERT INTO shares_ledger(user_id, outcome_id, kind, delta_available, delta_reserved, ref_type, ref_id) "
                                                "VALUES($1::uuid, $2::uuid, 'RESERVE', $3, $4, 'ORDER', $5::uuid)",
                                                [ctx](const Result &) mutable {
                                                    ctx->onOk(std::move(ctx->created));
                                                },
                                                ctx->onErr,
                                                ctx->userId,
                                                ctx->outcomeId,
                                                static_cast<long long>(-ctx->qtyMicros),
                                                static_cast<long long>(ctx->qtyMicros),
                                                ctx->created.id);
                                        },
                                        ctx->onErr,
                                        ctx->userId,
                                        ctx->outcomeId,
                                        static_cast<long long>(ctx->qtyMicros));
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
                    static_cast<long long>(ctx->qtyMicros));
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

    db_->execSqlAsync(
        std::string(kSelectBuy),
        [this, ctx, outcomeId, depth](const Result &r) {
            ctx->book.buy.reserve(r.size());
            for (std::size_t i = 0; i < r.size(); ++i) ctx->book.buy.push_back(rowToOrder(r, i));

            db_->execSqlAsync(
                std::string(kSelectSell),
                [ctx](const Result &r2) {
                    ctx->book.sell.reserve(r2.size());
                    for (std::size_t i = 0; i < r2.size(); ++i) ctx->book.sell.push_back(rowToOrder(r2, i));
                    ctx->onOk(std::move(ctx->book));
                },
                ctx->onErr,
                outcomeId,
                depth);
        },
        ctx->onErr,
        outcomeId,
        depth);
}
