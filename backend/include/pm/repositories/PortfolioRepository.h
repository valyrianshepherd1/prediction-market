#pragma once

#include <drogon/orm/DbClient.h>

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

struct PortfolioPositionRow {
    std::string user_id;
    std::string outcome_id;
    std::string market_id;
    std::string market_question;
    std::string outcome_title;
    int outcome_index{};
    std::int64_t shares_available{};
    std::int64_t shares_reserved{};
    std::string updated_at;
};

struct PortfolioLedgerEntryRow {
    std::string id;
    std::string ledger_type;
    std::string kind;
    std::string outcome_id;
    std::int64_t delta_available{};
    std::int64_t delta_reserved{};
    std::string ref_type;
    std::string ref_id;
    std::string created_at;
};

class PortfolioRepository {
public:
    explicit PortfolioRepository(drogon::orm::DbClientPtr db);

    void listPositionsForUser(
        const std::string &userId,
        int limit,
        int offset,
        std::function<void(std::vector<PortfolioPositionRow>)> onOk,
        std::function<void(const drogon::orm::DrogonDbException &)> onErr) const;

    void listLedgerForUser(
        const std::string &userId,
        int limit,
        int offset,
        std::function<void(std::vector<PortfolioLedgerEntryRow>)> onOk,
        std::function<void(const drogon::orm::DrogonDbException &)> onErr) const;

private:
    drogon::orm::DbClientPtr db_;
};
