#pragma once

#include <drogon/HttpController.h>
#include <functional>
#include <string>

class MarketController : public drogon::HttpController<MarketController> {
public:
    METHOD_LIST_BEGIN
        ADD_METHOD_TO(MarketController::listMarkets, "/markets", drogon::Get);
        ADD_METHOD_TO(MarketController::getMarket, "/markets/{1}", drogon::Get);
        ADD_METHOD_TO(MarketController::listOutcomes, "/markets/{1}/outcomes", drogon::Get);
        ADD_METHOD_TO(MarketController::createMarket, "/admin/markets", drogon::Post);
        ADD_METHOD_TO(MarketController::updateMarket, "/admin/markets/{1}", drogon::Patch);
        ADD_METHOD_TO(MarketController::closeMarket, "/admin/markets/{1}/close", drogon::Post);
        ADD_METHOD_TO(MarketController::resolveMarket, "/admin/markets/{1}/resolve", drogon::Post);
    METHOD_LIST_END

    void listMarkets(const drogon::HttpRequestPtr &req,
                     std::function<void(const drogon::HttpResponsePtr &)> &&cb) const;

    void getMarket(const drogon::HttpRequestPtr &req,
                   std::function<void(const drogon::HttpResponsePtr &)> &&cb,
                   std::string id) const;

    void listOutcomes(const drogon::HttpRequestPtr &req,
                      std::function<void(const drogon::HttpResponsePtr &)> &&cb,
                      std::string id) const;

    void createMarket(const drogon::HttpRequestPtr &req,
                      std::function<void(const drogon::HttpResponsePtr &)> &&cb) const;

    void updateMarket(const drogon::HttpRequestPtr &req,
                      std::function<void(const drogon::HttpResponsePtr &)> &&cb,
                      std::string id) const;

    void closeMarket(const drogon::HttpRequestPtr &req,
                     std::function<void(const drogon::HttpResponsePtr &)> &&cb,
                     std::string id) const;

    void resolveMarket(const drogon::HttpRequestPtr &req,
                       std::function<void(const drogon::HttpResponsePtr &)> &&cb,
                       std::string id) const;

private:
    static bool isAdmin(const drogon::HttpRequestPtr &req);
};
