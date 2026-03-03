#include <pm/repositories/MarketRepository.h>

#include <cstdint>
#include <drogon/drogon.h>

using drogon::orm::DrogonDbException;
using drogon::orm::Result;

MarketRepository::MarketRepository(drogon::orm::DbClientPtr db) : db_(std::move(db)) {
}

static MarketRow rowToMarket(const Result &r, size_t i = 0) {
    MarketRow m;
    const auto &row = r[i];

    m.id = row["id"].as<std::string>();
    m.question = row["question"].as<std::string>();
    m.status = row["status"].as<std::string>();

    const auto resolved = row["resolved_outcome_id"];
    if (resolved.isNull()) {
        m.resolved_outcome_id = std::nullopt;
    } else {
        m.resolved_outcome_id = resolved.as<std::string>();
    }

    m.created_at = row["created_at"].as<std::string>();
    return m;
}

void MarketRepository::listMarkets(std::optional<std::string> status,
                                   int limit,
                                   int offset,
                                   std::function<void(std::vector<MarketRow>)> onOk,
                                   std::function<void(const DrogonDbException &)> onErr) const {
    const std::int64_t lim = static_cast<std::int64_t>(limit);
    const std::int64_t off = static_cast<std::int64_t>(offset);

    if (status.has_value()) {
        static const std::string sql =
                "SELECT "
                "  id::text AS id, "
                "  question, "
                "  status, "
                "  resolved_outcome_id::text AS resolved_outcome_id, "
                "  created_at::text AS created_at "
                "FROM markets "
                "WHERE status = $1::text "
                "ORDER BY created_at DESC "
                "LIMIT $2::bigint OFFSET $3::bigint;";

        db_->execSqlAsync(
            sql,
            [onOk = std::move(onOk)](const Result &r) mutable {
                std::vector<MarketRow> out;
                out.reserve(r.size());
                for (size_t i = 0; i < r.size(); ++i)
                    out.push_back(rowToMarket(r, i));
                onOk(std::move(out));
            },
            std::move(onErr),
            *status,
            lim,
            off);
        return;
    }

    static const std::string sql =
            "SELECT "
            "  id::text AS id, "
            "  question, "
            "  status, "
            "  resolved_outcome_id::text AS resolved_outcome_id, "
            "  created_at::text AS created_at "
            "FROM markets "
            "ORDER BY created_at DESC "
            "LIMIT $1::bigint OFFSET $2::bigint;";

    db_->execSqlAsync(
        sql,
        [onOk = std::move(onOk)](const Result &r) mutable {
            std::vector<MarketRow> out;
            out.reserve(r.size());
            for (size_t i = 0; i < r.size(); ++i)
                out.push_back(rowToMarket(r, i));
            onOk(std::move(out));
        },
        std::move(onErr),
        lim,
        off);
}

void MarketRepository::getMarketById(const std::string &id,
                                     std::function<void(std::optional<MarketRow>)> onOk,
                                     std::function<void(const DrogonDbException &)> onErr) const {
    static const std::string sql =
            "SELECT "
            "  id::text AS id, "
            "  question, "
            "  status, "
            "  resolved_outcome_id::text AS resolved_outcome_id, "
            "  created_at::text AS created_at "
            "FROM markets "
            "WHERE id::text = $1 "
            "LIMIT 1;";

    db_->execSqlAsync(
        sql,
        [onOk = std::move(onOk)](const Result &r) mutable {
            if (r.empty())
                return onOk(std::nullopt);
            onOk(rowToMarket(r, 0));
        },
        std::move(onErr),
        id);
}

void MarketRepository::createMarket(const std::string &question,
                                    std::function<void(MarketRow)> onOk,
                                    std::function<void(const DrogonDbException &)> onErr) const {
    static const std::string sql =
            "INSERT INTO markets (question, status) "
            "VALUES ($1, 'OPEN') "
            "RETURNING "
            "  id::text AS id, "
            "  question, "
            "  status, "
            "  resolved_outcome_id::text AS resolved_outcome_id, "
            "  created_at::text AS created_at;";

    db_->execSqlAsync(
        sql,
        [onOk = std::move(onOk)](const Result &r) mutable {
            onOk(rowToMarket(r, 0));
        },
        std::move(onErr),
        question);
}
