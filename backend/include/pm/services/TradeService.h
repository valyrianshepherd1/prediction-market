#pragma once

#include "pm/repositories/TradeRepository.h"

class TradeService {
public:
    explicit TradeService(TradeRepository repo) : repo_(std::move(repo)) {
    }

    inline void listTradesByOutcome(const std::string &outcomeId,
                             int limit,
                             int offset,
                             std::function<void(std::vector<TradeRow>)> onOk,
                             std::function<void(const drogon::orm::DrogonDbException &)> onErr) const {
        repo_.listTradesByOutcome(outcomeId, limit, offset, std::move(onOk), std::move(onErr));
    }

    inline void listTradesByUser(const std::string &userId,
                          int limit,
                          int offset,
                          std::function<void(std::vector<TradeRow>)> onOk,
                          std::function<void(const drogon::orm::DrogonDbException &)> onErr) const {
        repo_.listTradesByUser(userId, limit, offset, std::move(onOk), std::move(onErr));
    }

private:
    TradeRepository repo_;
};
