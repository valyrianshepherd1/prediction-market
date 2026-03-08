#include "pm/repositories/PortfolioRepository.h"

using drogon::orm::DbClientPtr;
using drogon::orm::Result;

namespace {

PortfolioPositionRow rowToPosition(const Result &r, std::size_t i) {
    PortfolioPositionRow p;
    p.user_id = r[i]["user_id"].as<std::string>();
    p.outcome_id = r[i]["outcome_id"].as<std::string>();
    p.market_id = r[i]["market_id"].as<std::string>();
    p.market_question = r[i]["market_question"].as<std::string>();
    p.outcome_title = r[i]["outcome_title"].as<std::string>();
    p.outcome_index = r[i]["outcome_index"].as<int>();
    p.shares_available = r[i]["shares_available"].as<std::int64_t>();
    p.shares_reserved = r[i]["shares_reserved"].as<std::int64_t>();
    p.updated_at = r[i]["updated_at"].as<std::string>();
    return p;
}

PortfolioLedgerEntryRow rowToLedgerEntry(const Result &r, std::size_t i) {
    PortfolioLedgerEntryRow e;
    e.id = r[i]["id"].as<std::string>();
    e.ledger_type = r[i]["ledger_type"].as<std::string>();
    e.kind = r[i]["kind"].as<std::string>();
    if (!r[i]["outcome_id"].isNull()) {
        e.outcome_id = r[i]["outcome_id"].as<std::string>();
    }
    e.delta_available = r[i]["delta_available"].as<std::int64_t>();
    e.delta_reserved = r[i]["delta_reserved"].as<std::int64_t>();
    e.ref_type = r[i]["ref_type"].as<std::string>();
    e.ref_id = r[i]["ref_id"].as<std::string>();
    e.created_at = r[i]["created_at"].as<std::string>();
    return e;
}

}  // namespace

PortfolioRepository::PortfolioRepository(DbClientPtr db) : db_(std::move(db)) {}

void PortfolioRepository::listPositionsForUser(
    const std::string &userId,
    int limit,
    int offset,
    std::function<void(std::vector<PortfolioPositionRow>)> onOk,
    std::function<void(const drogon::orm::DrogonDbException &)> onErr) const {
    if (limit <= 0) {
        limit = 50;
    }
    if (limit > 200) {
        limit = 200;
    }
    if (offset < 0) {
        offset = 0;
    }

    static constexpr std::string_view kSql =
        "SELECT "
        " p.user_id::text AS user_id, "
        " p.outcome_id::text AS outcome_id, "
        " o.market_id::text AS market_id, "
        " m.question AS market_question, "
        " o.title AS outcome_title, "
        " o.outcome_index, "
        " p.shares_available, "
        " p.shares_reserved, "
        " p.updated_at::text AS updated_at "
        "FROM positions p "
        "JOIN outcomes o ON o.id = p.outcome_id "
        "JOIN markets m ON m.id = o.market_id "
        "WHERE p.user_id = $1::uuid "
        "  AND (p.shares_available <> 0 OR p.shares_reserved <> 0) "
        "ORDER BY m.created_at DESC, o.outcome_index ASC "
        "LIMIT $2::int OFFSET $3::int";

    db_->execSqlAsync(
        std::string(kSql),
        [onOk = std::move(onOk)](const Result &r) mutable {
            std::vector<PortfolioPositionRow> out;
            out.reserve(r.size());
            for (std::size_t i = 0; i < r.size(); ++i) {
                out.push_back(rowToPosition(r, i));
            }
            onOk(std::move(out));
        },
        std::move(onErr),
        userId,
        limit,
        offset);
}

void PortfolioRepository::listLedgerForUser(
    const std::string &userId,
    int limit,
    int offset,
    std::function<void(std::vector<PortfolioLedgerEntryRow>)> onOk,
    std::function<void(const drogon::orm::DrogonDbException &)> onErr) const {
    if (limit <= 0) {
        limit = 50;
    }
    if (limit > 200) {
        limit = 200;
    }
    if (offset < 0) {
        offset = 0;
    }

    static constexpr std::string_view kSql =
        "SELECT id, ledger_type, kind, outcome_id, delta_available, delta_reserved, ref_type, ref_id, created_at "
        "FROM ("
        "  SELECT "
        "    id::text AS id, "
        "    'CASH'::text AS ledger_type, "
        "    NULL::text AS outcome_id, "
        "    kind, "
        "    delta_available, "
        "    delta_reserved, "
        "    ref_type, "
        "    ref_id::text AS ref_id, "
        "    created_at::text AS created_at, "
        "    created_at AS sort_ts "
        "  FROM cash_ledger "
        "  WHERE user_id = $1::uuid "
        "  UNION ALL "
        "  SELECT "
        "    id::text AS id, "
        "    'SHARES'::text AS ledger_type, "
        "    outcome_id::text AS outcome_id, "
        "    kind, "
        "    delta_available, "
        "    delta_reserved, "
        "    ref_type, "
        "    ref_id::text AS ref_id, "
        "    created_at::text AS created_at, "
        "    created_at AS sort_ts "
        "  FROM shares_ledger "
        "  WHERE user_id = $1::uuid "
        ") t "
        "ORDER BY sort_ts DESC "
        "LIMIT $2::int OFFSET $3::int";

    db_->execSqlAsync(
        std::string(kSql),
        [onOk = std::move(onOk)](const Result &r) mutable {
            std::vector<PortfolioLedgerEntryRow> out;
            out.reserve(r.size());
            for (std::size_t i = 0; i < r.size(); ++i) {
                out.push_back(rowToLedgerEntry(r, i));
            }
            onOk(std::move(out));
        },
        std::move(onErr),
        userId,
        limit,
        offset);
}
