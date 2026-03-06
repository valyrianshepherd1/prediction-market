#pragma once

#include <drogon/HttpController.h>

class TradeController : public drogon::HttpController<TradeController> {
public:
    METHOD_LIST_BEGIN
        ADD_METHOD_TO(TradeController::listTradesByOutcome, "/outcomes/{1}/trades", drogon::Get);
        ADD_METHOD_TO(TradeController::listTradesByUser, "/users/{1}/trades", drogon::Get);
    METHOD_LIST_END

    void listTradesByOutcome(const drogon::HttpRequestPtr &req,
                             std::function<void(const drogon::HttpResponsePtr &)> &&cb,
                             std::string outcomeId) const;

    void listTradesByUser(const drogon::HttpRequestPtr &req,
                          std::function<void(const drogon::HttpResponsePtr &)> &&cb,
                          std::string userId) const;
};
