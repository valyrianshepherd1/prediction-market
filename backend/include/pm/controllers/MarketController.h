#pragma once

#include <drogon/HttpController.h>
#include <string>

class MarketController : public drogon::HttpController<MarketController> {
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(MarketController::listMarkets, "/markets", drogon::Get);
    ADD_METHOD_TO(MarketController::getMarket, "/markets/{1}", drogon::Get);
    ADD_METHOD_TO(MarketController::listOutcomes, "/markets/{1}/outcomes", drogon::Get);
    ADD_METHOD_TO(MarketController::createMarket, "/admin/markets", drogon::Post);
    METHOD_LIST_END

    void listMarkets(const drogon::HttpRequestPtr &req, std::function<void(const drogon::HttpResponsePtr &)> &&cb) const;
    void getMarket(const drogon::HttpRequestPtr &req, std::function<void(const drogon::HttpResponsePtr &)> &&cb, std::string id) const;
    void listOutcomes(const drogon::HttpRequestPtr &req, std::function<void(const drogon::HttpResponsePtr &)> &&cb, std::string id) const;
    void createMarket(const drogon::HttpRequestPtr &req, std::function<void(const drogon::HttpResponsePtr &)> &&cb) const;

private:
    static bool isAdmin(const drogon::HttpRequestPtr &req);
};