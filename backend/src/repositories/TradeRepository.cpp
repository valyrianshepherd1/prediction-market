#include "pm/repositories/TradeRepository.h"

using drogon::orm::DbClientPtr;
using drogon::orm::DrogonDbException;
using drogon::orm::Result;

namespace {

TradeRow rowToTrade(const Result &r, std::size_t i) {
    TradeRow t;
    t.id = r[i]["id"].as<std::string>();
    t.outcome_id = r[i]["outcome_id"].as<std::string>();
    t.maker_user_id = r[i]["maker_user_id"].as<std::string>();
    t.taker_user_id = r[i]["taker_user_id"].as<std::string>();
    t.maker_order_id = r[i]["maker_order_id"].as<std::string>();
    t.taker_order_id = r[i]["taker_order_id"].as<std::string>();
    t.price_bp = r[i]["price_bp"].as<int>();
    t.qty_micros = r[i]["qty_micros"].as<std::int64_t>();
    t.created_at = r[i]["created_at"].as<std::string>();
    return t;
}

} // namespace

TradeRepository::TradeRepository(DbClientPtr db) : db_(std::move(db)) {}

void TradeRepository::listByOutcome(const std::string &outcomeId, int limit, int offset,
                                    std::function<void(std::vector<TradeRow>)> onOk,
                                    std::function<void(const DrogonDbException &)> onErr) const {
    if (limit <= 0) limit = 50;
    if (limit > 200) limit = 200;
    if (offset < 0) offset = 0;

    static constexpr std::string_view kSql =
        "SELECT id::text AS id, outcome_id::text AS outcome_id, "
        "maker_user_id::text AS maker_user_id, taker_user_id::text AS taker_user_id, "
        "maker_order_id::text AS maker_order_id, taker_order_id::text AS taker_order_id, "
        "price_bp, qty_micros, created_at::text AS created_at "
        "FROM trades WHERE outcome_id=$1::uuid "
        "ORDER BY created_at DESC "
        "LIMIT $2::int OFFSET $3::int";

    db_->execSqlAsync(std::string(kSql),
        [onOk = std::move(onOk)](const Result &r) mutable {
            std::vector<TradeRow> out;
            out.reserve(r.size());
            for (std::size_t i = 0; i < r.size(); ++i) out.push_back(rowToTrade(r, i));
            onOk(std::move(out));
        },
        std::move(onErr),
        outcomeId, limit, offset);
}

void TradeRepository::listByUser(const std::string &userId, int limit, int offset,
                                 std::function<void(std::vector<TradeRow>)> onOk,
                                 std::function<void(const DrogonDbException &)> onErr) const {
    if (limit <= 0) limit = 50;
    if (limit > 200) limit = 200;
    if (offset < 0) offset = 0;

    static constexpr std::string_view kSql =
        "SELECT id::text AS id, outcome_id::text AS outcome_id, "
        "maker_user_id::text AS maker_user_id, taker_user_id::text AS taker_user_id, "
        "maker_order_id::text AS maker_order_id, taker_order_id::text AS taker_order_id, "
        "price_bp, qty_micros, created_at::text AS created_at "
        "FROM trades WHERE maker_user_id=$1::uuid OR taker_user_id=$1::uuid "
        "ORDER BY created_at DESC "
        "LIMIT $2::int OFFSET $3::int";

    db_->execSqlAsync(std::string(kSql),
        [onOk = std::move(onOk)](const Result &r) mutable {
            std::vector<TradeRow> out;
            out.reserve(r.size());
            for (std::size_t i = 0; i < r.size(); ++i) out.push_back(rowToTrade(r, i));
            onOk(std::move(out));
        },
        std::move(onErr),
        userId, limit, offset);
}

void TradeRepository::listByOrder(const std::string &orderId,
                                  std::function<void(std::vector<TradeRow>)> onOk,
                                  std::function<void(const DrogonDbException &)> onErr) const {
    static constexpr std::string_view kSql =
        "SELECT id::text AS id, outcome_id::text AS outcome_id, "
        "maker_user_id::text AS maker_user_id, taker_user_id::text AS taker_user_id, "
        "maker_order_id::text AS maker_order_id, taker_order_id::text AS taker_order_id, "
        "price_bp, qty_micros, created_at::text AS created_at "
        "FROM trades WHERE maker_order_id=$1::uuid OR taker_order_id=$1::uuid "
        "ORDER BY created_at ASC, id ASC";

    db_->execSqlAsync(std::string(kSql),
        [onOk = std::move(onOk)](const Result &r) mutable {
            std::vector<TradeRow> out;
            out.reserve(r.size());
            for (std::size_t i = 0; i < r.size(); ++i) out.push_back(rowToTrade(r, i));
            onOk(std::move(out));
        },
        std::move(onErr),
        orderId);
}
