#include "pm/repositories/MarketRepository.h"

#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

using drogon::orm::DrogonDbException;
using drogon::orm::Result;
using TransactionPtr = std::shared_ptr<drogon::orm::Transaction>;

namespace {

struct WinnerRow {
    std::string user_id;
    std::int64_t payout_micros{};
};

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

static OutcomeRow rowToOutcome(const Result &r, size_t i = 0) {
    OutcomeRow o;
    const auto &row = r[i];
    o.id = row["id"].as<std::string>();
    o.market_id = row["market_id"].as<std::string>();
    o.title = row["title"].as<std::string>();
    o.outcome_index = row["outcome_index"].as<int>();
    return o;
}

static std::vector<WinnerRow> rowsToWinners(const Result &r) {
    std::vector<WinnerRow> out;
    out.reserve(r.size());
    for (size_t i = 0; i < r.size(); ++i) {
        WinnerRow w;
        const auto &row = r[i];
        w.user_id = row["user_id"].as<std::string>();
        w.payout_micros = row["payout_micros"].as<std::int64_t>();
        out.push_back(std::move(w));
    }
    return out;
}

}  // namespace

MarketRepository::MarketRepository(drogon::orm::DbClientPtr db) : db_(std::move(db)) {}

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
                for (size_t i = 0; i < r.size(); ++i) {
                    out.push_back(rowToMarket(r, i));
                }
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
            for (size_t i = 0; i < r.size(); ++i) {
                out.push_back(rowToMarket(r, i));
            }
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
        " m.id::text AS id, "
        " m.question, "
        " m.status, "
        " mr.winning_outcome_id::text AS resolved_outcome_id, "
        " m.created_at::text AS created_at "
        "FROM markets m "
        "LEFT JOIN market_resolutions mr ON mr.market_id = m.id "
        "WHERE m.id::text = $1 "
        "LIMIT 1;";

    db_->execSqlAsync(
        sql,
        [onOk = std::move(onOk)](const Result &r) mutable {
            if (r.empty()) {
                return onOk(std::nullopt);
            }
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

void MarketRepository::listOutcomesByMarketId(
    const std::string &marketId,
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
            for (size_t i = 0; i < r.size(); ++i) {
                out.push_back(rowToOutcome(r, i));
            }
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
                        st->market.id,
                        st->titles[i],
                        static_cast<int>(i));
                };

                (*insertNext)(0);
            },
            [st](const DrogonDbException &e) mutable { st->onErr(e); },
            question);
    });
}

void MarketRepository::resolveMarket(
    const std::string &marketId,
    const std::string &winningOutcomeId,
    const std::string &resolvedByUserId,
    std::function<void(MarketRow)> onOk,
    std::function<void(const DrogonDbException &)> onErr) const {
    struct State {
        TransactionPtr tx;
        std::function<void(MarketRow)> onOk;
        std::function<void(const DrogonDbException &)> onErr;
        std::string marketId;
        std::string winningOutcomeId;
        MarketRow updatedMarket;
        std::vector<WinnerRow> winners;
    };

    auto st = std::make_shared<State>();
    st->onOk = std::move(onOk);
    st->onErr = std::move(onErr);
    st->marketId = marketId;
    st->winningOutcomeId = winningOutcomeId;

    db_->newTransactionAsync([st, resolvedByUserId](const TransactionPtr &tx) {
        st->tx = tx;

        static const std::string lockSql =
            "SELECT id "
            "FROM markets "
            "WHERE id = $1::uuid "
            "FOR UPDATE;";

        tx->execSqlAsync(
            lockSql,
            [st, resolvedByUserId](const Result &) mutable {
                static const std::string insResolutionSql =
                    "INSERT INTO market_resolutions (market_id, winning_outcome_id, resolved_by_user_id) "
                    "VALUES ($1::uuid, $2::uuid, $3::uuid);";

                st->tx->execSqlAsync(
                    insResolutionSql,
                    [st](const Result &) mutable {
                        static const std::string updateMarketSql =
                            "UPDATE markets "
                            "SET status = 'RESOLVED', "
                            "    closed_at = COALESCE(closed_at, now()) "
                            "WHERE id = $1::uuid "
                            "RETURNING "
                            " id::text AS id, "
                            " question, "
                            " status, "
                            " $2::text AS resolved_outcome_id, "
                            " created_at::text AS created_at;";

                        st->tx->execSqlAsync(
                            updateMarketSql,
                            [st](const Result &r3) mutable {
                                st->updatedMarket = rowToMarket(r3, 0);

                                static const std::string winnersSql =
                                    "SELECT "
                                    " p.user_id::text AS user_id, "
                                    " (p.shares_available + p.shares_reserved) AS payout_micros "
                                    "FROM positions p "
                                    "WHERE p.outcome_id = $1::uuid "
                                    "  AND (p.shares_available + p.shares_reserved) > 0 "
                                    "FOR UPDATE;";

                                st->tx->execSqlAsync(
                                    winnersSql,
                                    [st](const Result &r4) mutable {
                                        st->winners = rowsToWinners(r4);

                                        auto applyNext = std::make_shared<std::function<void(size_t)>>();
                                        *applyNext = [st, applyNext](size_t i) mutable {
                                            if (i >= st->winners.size()) {
                                                st->onOk(std::move(st->updatedMarket));
                                                return;
                                            }

                                            const WinnerRow winner = st->winners[i];
                                            if (winner.payout_micros <= 0) {
                                                (*applyNext)(i + 1);
                                                return;
                                            }

                                            static const std::string ensureWalletSql =
                                                "INSERT INTO wallets (user_id, available, reserved) "
                                                "VALUES ($1::uuid, 0, 0) "
                                                "ON CONFLICT (user_id) DO NOTHING;";

                                            st->tx->execSqlAsync(
                                                ensureWalletSql,
                                                [st, applyNext, i, winner](const Result &) mutable {
                                                    static const std::string walletSql =
                                                        "UPDATE wallets "
                                                        "SET available = available + $2::bigint, "
                                                        "    updated_at = now() "
                                                        "WHERE user_id = $1::uuid;";

                                                    st->tx->execSqlAsync(
                                                        walletSql,
                                                        [st, applyNext, i, winner](const Result &) mutable {
                                                            static const std::string ledgerSql =
                                                                "INSERT INTO cash_ledger "
                                                                "(user_id, kind, delta_available, delta_reserved, ref_type, ref_id) "
                                                                "VALUES ($1::uuid, 'SETTLEMENT', $2::bigint, 0, 'MARKET', $3::uuid) "
                                                                "RETURNING id::text AS id;";

                                                            st->tx->execSqlAsync(
                                                                ledgerSql,
                                                                [st, applyNext, i, winner](const Result &r7) mutable {
                                                                    const std::string cashLedgerId =
                                                                        r7[0]["id"].as<std::string>();

                                                                    static const std::string settlementSql =
                                                                        "INSERT INTO settlements "
                                                                        "(market_id, user_id, winning_outcome_id, payout_micros, cash_ledger_id) "
                                                                        "VALUES ($1::uuid, $2::uuid, $3::uuid, $4::bigint, $5::uuid);";

                                                                    st->tx->execSqlAsync(
                                                                        settlementSql,
                                                                        [applyNext, i](const Result &) mutable {
                                                                            (*applyNext)(i + 1);
                                                                        },
                                                                        [st](const DrogonDbException &e) mutable {
                                                                            st->onErr(e);
                                                                        },
                                                                        st->marketId,
                                                                        winner.user_id,
                                                                        st->winningOutcomeId,
                                                                        winner.payout_micros,
                                                                        cashLedgerId);
                                                                },
                                                                [st](const DrogonDbException &e) mutable {
                                                                    st->onErr(e);
                                                                },
                                                                winner.user_id,
                                                                winner.payout_micros,
                                                                st->marketId);
                                                        },
                                                        [st](const DrogonDbException &e) mutable {
                                                            st->onErr(e);
                                                        },
                                                        winner.user_id,
                                                        winner.payout_micros);
                                                },
                                                [st](const DrogonDbException &e) mutable {
                                                    st->onErr(e);
                                                },
                                                winner.user_id);
                                        };

                                        (*applyNext)(0);
                                    },
                                    [st](const DrogonDbException &e) mutable { st->onErr(e); },
                                    st->winningOutcomeId);
                            },
                            [st](const DrogonDbException &e) mutable { st->onErr(e); },
                            st->marketId,
                            st->winningOutcomeId);
                    },
                    [st](const DrogonDbException &e) mutable { st->onErr(e); },
                    st->marketId,
                    st->winningOutcomeId,
                    resolvedByUserId);
            },
            [st](const DrogonDbException &e) mutable { st->onErr(e); },
            st->marketId);
    });
}
