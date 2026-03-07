#pragma once

#include <drogon/HttpController.h>

class TradeController : public drogon::HttpController<TradeController> {
public:
    METHOD_LIST_BEGIN
        ADD_METHOD_TO(TradeController::listByOutcome, "/outcomes/{1}/trades", drogon::Get);
        ADD_METHOD_TO(TradeController::listMyTrades,  "/me/trades",           drogon::Get);
    METHOD_LIST_END

    void listByOutcome(const drogon::HttpRequestPtr &req,
                       std::function<void(const drogon::HttpResponsePtr &)> &&cb,
                       std::string outcomeId) const;

    void listMyTrades(const drogon::HttpRequestPtr &req,
                      std::function<void(const drogon::HttpResponsePtr &)> &&cb) const;
};