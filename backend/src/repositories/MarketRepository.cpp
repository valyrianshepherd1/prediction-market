#include "pm/repositories/MarketRepository.h"

#include <cstdint>
#include <memory>
#include <utility>

using drogon::orm::DrogonDbException;
using drogon::orm::Result;
using TransactionPtr = std::shared_ptr<drogon::orm::Transaction>;
MarketRepository::MarketRepository(drogon::orm::DbClientPtr db) : db_(std::move(db)) {}

static MarketRow rowToMarket(const Result &r, size_t i = 0) {
    MarketRow m;
    const auto &row = r[i];
    m.id = row["id"].as<std::string>();
    m.question = row["question"].as<std::string>();
    m.status = row["status"].as<std::string>();

    // если колонки нет в БД — будет ошибка на SQL уровне; предполагаем, что она уже есть
    const auto resolved = row["resolved_outcome_id"];
    if (resolved.isNull()) m.resolved_outcome_id = std::nullopt;
    else m.resolved_outcome_id = resolved.as<std::string>();

    m.created_at = row["created_at"].as<std::string>();
    return m;
}

static OutcomeRow rowToOutcome(const Result &r, size_t i = 0) {
    OutcomeRow o;
    const auto &row = r[i];
    o.id = row["id"].as<std::string>();
    o.market_id = row["market_id"].as<std::string>();
    o.title = row["title"].as<std::string>();
    o.outcome_index = row["outcome_index"].as<int>();
    return o;
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
                " m.id::text AS id, "
                " m.question, "
                " m.status, "
                " mr.winning_outcome_id::text AS resolved_outcome_id, "
                " m.created_at::text AS created_at "
                "FROM markets m "
                "LEFT JOIN market_resolutions mr ON mr.market_id = m.id "
                "WHERE m.status = $1::text "
                "ORDER BY m.created_at DESC "
                "LIMIT $2::bigint OFFSET $3::bigint;";

        db_->execSqlAsync(
            sql,
            [onOk = std::move(onOk)](const Result &r) mutable {
                std::vector<MarketRow> out;
                out.reserve(r.size());
                for (size_t i = 0; i < r.size(); ++i) out.push_back(rowToMarket(r, i));
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
            " m.id::text AS id, "
            " m.question, "
            " m.status, "
            " mr.winning_outcome_id::text AS resolved_outcome_id, "
            " m.created_at::text AS created_at "
            "FROM markets m "
            "LEFT JOIN market_resolutions mr ON mr.market_id = m.id "
            "ORDER BY m.created_at DESC "
            "LIMIT $1::bigint OFFSET $2::bigint;";

    db_->execSqlAsync(
        sql,
        [onOk = std::move(onOk)](const Result &r) mutable {
            std::vector<MarketRow> out;
            out.reserve(r.size());
            for (size_t i = 0; i < r.size(); ++i) out.push_back(rowToMarket(r, i));
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
            "INSERT INTO markets (question, status) "
            "VALUES ($1, 'OPEN') "
            "RETURNING "
            " id::text AS id, "
            " question, "
            " status, "
            " NULL::text AS resolved_outcome_id, "
            " created_at::text AS created_at;";

    db_->execSqlAsync(
        sql,
        [onOk = std::move(onOk)](const Result &r) mutable {
            if (r.empty()) return onOk(std::nullopt);
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
            " id::text AS id, "
            " question, "
            " status, "
            " NULL::text AS resolved_outcome_id, "
            " created_at::text AS created_at;";

    db_->execSqlAsync(
        sql,
        [onOk = std::move(onOk)](const Result &r) mutable { onOk(rowToMarket(r, 0)); },
        std::move(onErr),
        question);
}

void MarketRepository::listOutcomesByMarketId(const std::string &marketId,
                                             std::function<void(std::vector<OutcomeRow>)> onOk,
                                             std::function<void(const DrogonDbException &)> onErr) const {
    static const std::string sql =
        "SELECT "
        " id::text AS id, "
        " market_id::text AS market_id, "
        " title, "
        " outcome_index "
        "FROM outcomes "
        "WHERE market_id::text = $1 "
        "ORDER BY outcome_index ASC;";

    db_->execSqlAsync(
        sql,
        [onOk = std::move(onOk)](const Result &r) mutable {
            std::vector<OutcomeRow> out;
            out.reserve(r.size());
            for (size_t i = 0; i < r.size(); ++i) out.push_back(rowToOutcome(r, i));
            onOk(std::move(out));
        },
        std::move(onErr),
        marketId);
}

void MarketRepository::createMarketWithOutcomes(
    const std::string &question,
    const std::vector<std::string> &outcomeTitles,
    std::function<void(MarketRow, std::vector<OutcomeRow>)> onOk,
    std::function<void(const DrogonDbException &)> onErr) const {

    struct State {
        TransactionPtr tx;
        std::function<void(MarketRow, std::vector<OutcomeRow>)> onOk;
        std::function<void(const DrogonDbException &)> onErr;
        MarketRow market;
        std::vector<std::string> titles;
        std::vector<OutcomeRow> outcomes;
    };

    auto st = std::make_shared<State>();
    st->onOk = std::move(onOk);
    st->onErr = std::move(onErr);
    st->titles = outcomeTitles;

    db_->newTransactionAsync([st, question](const TransactionPtr &tx) {
        st->tx = tx;

        static const std::string insertMarketSql =
            "INSERT INTO markets (question, status) "
            "VALUES ($1, 'OPEN') "
            "RETURNING "
            " id::text AS id, "
            " question, "
            " status, "
            " NULL::text AS resolved_outcome_id, "
            " created_at::text AS created_at;";

        tx->execSqlAsync(
            insertMarketSql,
            [st](const Result &r) mutable {
                st->market = rowToMarket(r, 0);

                if (st->titles.empty()) {
                    st->onOk(std::move(st->market), {});
                    return;
                }

                auto insertNext = std::make_shared<std::function<void(size_t)>>();
                *insertNext = [st, insertNext](size_t i) mutable {
                    if (i >= st->titles.size()) {
                        st->onOk(std::move(st->market), std::move(st->outcomes));
                        return;
                    }

                    static const std::string insertOutcomeSql =
                        "INSERT INTO outcomes (market_id, title, outcome_index) "
                        "VALUES ($1::uuid, $2::text, $3::int) "
                        "RETURNING "
                        " id::text AS id, "
                        " market_id::text AS market_id, "
                        " title, "
                        " outcome_index;";

                    st->tx->execSqlAsync(
                        insertOutcomeSql,
                        [st, insertNext, i](const Result &r2) mutable {
                            st->outcomes.push_back(rowToOutcome(r2, 0));
                            (*insertNext)(i + 1);
                        },
                        [st](const DrogonDbException &e) mutable { st->onErr(e); },
                        st->market.id,          // кастуем внутри SQL: $1::uuid
                        st->titles[i],
                        static_cast<int>(i));
                };

                (*insertNext)(0);
            },
            [st](const DrogonDbException &e) mutable { st->onErr(e); },
            question);
    });
}