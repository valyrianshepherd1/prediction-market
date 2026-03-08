#pragma once

#include <drogon/HttpController.h>

#include <functional>

class PortfolioController : public drogon::HttpController<PortfolioController> {
public:
    METHOD_LIST_BEGIN
        ADD_METHOD_TO(PortfolioController::listPortfolio, "/portfolio", drogon::Get);
        ADD_METHOD_TO(PortfolioController::listLedger, "/ledger", drogon::Get);
    METHOD_LIST_END

    void listPortfolio(const drogon::HttpRequestPtr &req,
                       std::function<void(const drogon::HttpResponsePtr &)> &&cb) const;

    void listLedger(const drogon::HttpRequestPtr &req,
                    std::function<void(const drogon::HttpResponsePtr &)> &&cb) const;
};
