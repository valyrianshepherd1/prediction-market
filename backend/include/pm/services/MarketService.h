#pragma once

#include "pm/repositories/MarketRepository.h"

#include <functional>
#include <optional>
#include <string>
#include <utility>
#include <vector>

class MarketService {
public:
    explicit MarketService(MarketRepository repo);

    void listMarkets(std::optional<std::string> status,
                     int limit,
                     int offset,
                     std::function<void(std::vector<MarketRow>)> onOk,
                     std::function<void(const drogon::orm::DrogonDbException &)> onErr) const;

    void getMarketById(const std::string &id,
                       std::function<void(std::optional<MarketRow>)> onOk,
                       std::function<void(const drogon::orm::DrogonDbException &)> onErr) const;

    void getMarketWithOutcomesById(
        const std::string &id,
        std::function<void(std::optional<std::pair<MarketRow, std::vector<OutcomeRow>>>)> onOk,
        std::function<void(const drogon::orm::DrogonDbException &)> onErr) const;

    void createMarket(const std::string &question,
                      std::function<void(MarketRow)> onOk,
                      std::function<void(const drogon::orm::DrogonDbException &)> onErr) const;

    void createMarketWithOutcomes(
        const std::string &question,
        const std::vector<std::string> &outcomeTitles,
        std::function<void(MarketRow, std::vector<OutcomeRow>)> onOk,
        std::function<void(const drogon::orm::DrogonDbException &)> onErr) const;

    void listOutcomesByMarketId(
        const std::string &marketId,
        std::function<void(std::vector<OutcomeRow>)> onOk,
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

private:
    MarketRepository repo_;
};
