#pragma once

#include <drogon/orm/DbClient.h>
#include <functional>
#include <optional>
#include <string>
#include <vector>

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
                     std::function<void(const drogon::orm::DrogonDbException &)>
                     onErr) const;

    void getMarketById(
        const std::string &id,
        std::function<void(std::optional<MarketRow>)> onOk,
        std::function<void(const drogon::orm::DrogonDbException &)>
        onErr) const;

    void createMarket(
        const std::string &question,
        std::function<void(MarketRow)> onOk,
        std::function<void(const drogon::orm::DrogonDbException &)>
        onErr) const;

private:
    drogon::orm::DbClientPtr db_;
};
