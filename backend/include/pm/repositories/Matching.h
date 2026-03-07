#pragma once

#include <drogon/orm/DbClient.h>
#include <drogon/HttpTypes.h>

#include <functional>
#include <string>

using TransactionPtr = std::shared_ptr<drogon::orm::Transaction>;

namespace pm {
    void matchTakerOrderInTx(
        const TransactionPtr &tx,
        const std::string &takerOrderId,
        const std::string &takerUserId,
        const std::string &outcomeId,
        const std::string &takerSide,   // BUY | SELL
        int takerPriceBp,
        int maxTrades,
        std::function<void()> onDone,
        std::function<void(const drogon::orm::DrogonDbException &)> onErr,
        std::function<void(drogon::HttpStatusCode, std::string)> onBizErr);
} // namespace pm