#pragma once

#include <drogon/HttpController.h>

class MarketController : public drogon::HttpController<MarketController> {
public:
    METHOD_LIST_BEGIN
        ADD_METHOD_TO(MarketController::listMarkets, "/markets", drogon::Get);
        ADD_METHOD_TO(MarketController::getMarket, "/markets/{1}", drogon::Get);
        ADD_METHOD_TO(MarketController::createMarket, "/admin/markets", drogon::Post);
    METHOD_LIST_END

    void listMarkets(const drogon::HttpRequestPtr &req,
                     std::function<void(const drogon::HttpResponsePtr &)> &&cb) const;

    void getMarket(const drogon::HttpRequestPtr &req,
                   std::function<void(const drogon::HttpResponsePtr &)> &&cb,
                   std::string id) const;

    void createMarket(const drogon::HttpRequestPtr &req,
                      std::function<void(const drogon::HttpResponsePtr &)> &&cb) const;

private:
    static bool isAdmin(const drogon::HttpRequestPtr &req);
};


/*
 1.b 1
 2.b 0
 3.b 0
 4.a 0
 5.a 0
 6.b 1
 7.a 0
 8.b 1
 3 overall
 */