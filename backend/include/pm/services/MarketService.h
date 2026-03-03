#pragma once

#include <pm/repositories/MarketRepository.h>

#include <functional>
#include <optional>
#include <string>
#include <vector>

class MarketService {
public:
    explicit MarketService(MarketRepository repo);

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
    MarketRepository repo_;
};
