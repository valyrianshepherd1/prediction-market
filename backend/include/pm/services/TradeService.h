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
        repo_.listByOutcome(outcomeId, limit, offset, std::move(onOk), std::move(onErr));
    }

    inline void listTradesByUser(const std::string &userId,
                                 int limit,
                                 int offset,
                                 std::function<void(std::vector<TradeRow>)> onOk,
                                 std::function<void(const drogon::orm::DrogonDbException &)> onErr) const {
        repo_.listByUser(userId, limit, offset, std::move(onOk), std::move(onErr));
    }

    inline void listTradesForOrder(const std::string &orderId,
                                   std::function<void(std::vector<TradeRow>)> onOk,
                                   std::function<void(const drogon::orm::DrogonDbException &)> onErr) const {
        repo_.listByOrder(orderId, std::move(onOk), std::move(onErr));
    }

private:
    TradeRepository repo_;
};
