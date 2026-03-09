#pragma once

#include <drogon/HttpController.h>
#include <functional>
#include <string>

#include "pm/util/ApiError.h"

class WalletController : public drogon::HttpController<WalletController> {
public:
    METHOD_LIST_BEGIN
        ADD_METHOD_TO(WalletController::getMyWallet, "/wallet", drogon::Get);
        ADD_METHOD_TO(WalletController::getWallet, "/wallets/{1}", drogon::Get);
        ADD_METHOD_TO(WalletController::adminDeposit, "/admin/deposit", drogon::Post);
    METHOD_LIST_END

    void getMyWallet(const drogon::HttpRequestPtr &req,
                     std::function<void(const drogon::HttpResponsePtr &)> &&cb) const;

    void getWallet(const drogon::HttpRequestPtr &req,
                   std::function<void(const drogon::HttpResponsePtr &)> &&cb,
                   std::string userId) const;

    void adminDeposit(const drogon::HttpRequestPtr &req,
                      std::function<void(const drogon::HttpResponsePtr &)> &&cb) const;

private:
    static pm::ApiError adminAuthError();

    static bool isAdmin(const drogon::HttpRequestPtr &req);
};
