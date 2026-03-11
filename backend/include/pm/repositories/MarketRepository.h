#pragma once

#include <drogon/orm/DbClient.h>

#include <functional>
#include <optional>
#include <string>
#include <vector>

struct OutcomeRow {
    std::string id;
    std::string market_id;
    std::string title;
    int outcome_index{};
};

struct MarketRow {
    std::string id;
    std::string question;
    std::string status;
    std::optional<std::string> resolved_outcome_id;
    std::string created_at;
};

class MarketRepository {
public:
    explicit MarketRepository(drogon::orm::DbClientPtr db);

    void listMarkets(std::optional<std::string> status,
                     int limit,
                     int offset,
                     std::function<void(std::vector<MarketRow>)> onOk,
                     std::function<void(const drogon::orm::DrogonDbException &)> onErr) const;

    void getMarketById(const std::string &id,
                       std::function<void(std::optional<MarketRow>)> onOk,
                       std::function<void(const drogon::orm::DrogonDbException &)> onErr) const;

    void createMarket(const std::string &question,
                      std::function<void(MarketRow)> onOk,
                      std::function<void(const drogon::orm::DrogonDbException &)> onErr) const;

    void listOutcomesByMarketId(
        const std::string &marketId,
        std::function<void(std::vector<OutcomeRow>)> onOk,
        std::function<void(const drogon::orm::DrogonDbException &)> onErr) const;

    void createMarketWithOutcomes(
        const std::string &question,
        const std::vector<std::string> &outcomeTitles,
        std::function<void(MarketRow, std::vector<OutcomeRow>)> onOk,
        std::function<void(const drogon::orm::DrogonDbException &)> onErr) const;

    void updateMarket(const std::string &marketId,
                      std::optional<std::string> question,
                      std::optional<std::string> status,
                      std::function<void(std::optional<MarketRow>)> onOk,
                      std::function<void(const drogon::orm::DrogonDbException &)> onErr) const;

    void resolveMarket(const std::string &marketId,
                       const std::string &winningOutcomeId,
                       const std::string &resolvedByUserId,
                       std::function<void(MarketRow)> onOk,
                       std::function<void(const drogon::orm::DrogonDbException &)> onErr) const;

    void archiveMarket(const std::string &marketId,
                       std::function<void(MarketRow)> onOk,
                       std::function<void(const drogon::orm::DrogonDbException &)> onErr) const ;

private:
    drogon::orm::DbClientPtr db_;
};
