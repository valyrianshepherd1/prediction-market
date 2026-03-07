#include "pm/controllers/WalletController.h"

#include "pm/repositories/WalletRepository.h"
#include "pm/services/WalletService.h"
#include "pm/util/ApiError.h"

#include <drogon/drogon.h>
#include <memory>

using drogon::HttpResponse;
using drogon::HttpResponsePtr;

namespace {
    pm::ApiError requireAdmin(const drogon::HttpRequestPtr &req) {
        const char *expected = std::getenv("PM_ADMIN_TOKEN");
        if (!expected || std::string_view(expected).empty()) {
            return {drogon::k401Unauthorized, "admin token is not configured"};
        }
        const auto token = req->getHeader("X-Admin-Token");
        if (token.empty() || token != expected) {
            return {drogon::k401Unauthorized, "admin token missing or invalid"};
        }
        return {drogon::k200OK, ""};
    }

    drogon::orm::DbClientPtr getDbOrNull(std::string &err) {
        try {
            return drogon::app().getDbClient("default");
        } catch (const std::exception &e) {
            err = e.what();
            return nullptr;
        }
    }

    Json::Value walletToJson(const WalletRow &w) {
        Json::Value j;
        j["user_id"] = w.user_id;
        j["available"] = Json::Int64(w.available);
        j["reserved"] = Json::Int64(w.reserved);
        j["updated_at"] = w.updated_at;
        return j;
    }
} // namespace

bool WalletController::isAdmin(const drogon::HttpRequestPtr &req) {
    return requireAdmin(req).code == drogon::k200OK;
}

pm::ApiError WalletController::adminAuthError() {
    return {drogon::k401Unauthorized, "admin token missing or invalid"};
}

void WalletController::getWallet(const drogon::HttpRequestPtr &,
                                 std::function<void(const HttpResponsePtr &)> &&cb,
                                 std::string userId) const {
    auto cbp = std::make_shared<std::function<void(const HttpResponsePtr &)> >(std::move(cb));

    std::string dbErr;
    auto db = getDbOrNull(dbErr);
    if (!db) {
        return (*cbp)(pm::jsonError({drogon::k500InternalServerError, "db client not available: " + dbErr}));
    }

    WalletService svc{WalletRepository{db}};
    svc.getWallet(
        userId,
        [cbp](std::optional<WalletRow> w) {
            if (!w) return (*cbp)(pm::jsonError({drogon::k404NotFound, "wallet not found"}));
            auto resp = HttpResponse::newHttpJsonResponse(walletToJson(*w));
            resp->setStatusCode(drogon::k200OK);
            (*cbp)(resp);
        },
        [cbp](const drogon::orm::DrogonDbException &e) {
            (*cbp)(pm::jsonError({drogon::k503ServiceUnavailable, e.base().what()}));
        });
}

void WalletController::adminDeposit(const drogon::HttpRequestPtr &req,
                                    std::function<void(const HttpResponsePtr &)> &&cb) const {
    auto cbp = std::make_shared<std::function<void(const HttpResponsePtr &)> >(std::move(cb));

    if (!isAdmin(req)) return (*cbp)(pm::jsonError(adminAuthError()));

    const auto json = req->getJsonObject();
    if (!json) return (*cbp)(pm::jsonError({drogon::k400BadRequest, "expected JSON body"}));

    const auto userId = (*json).get("user_id", "").asString();
    const auto amount = (*json).get("amount", 0).asInt64();

    if (userId.empty()) return (*cbp)(pm::jsonError({drogon::k400BadRequest, "user_id is required"}));

    std::string dbErr;
    auto db = getDbOrNull(dbErr);
    if (!db) {
        return (*cbp)(pm::jsonError({drogon::k500InternalServerError, "db client not available: " + dbErr}));
    }

    WalletService svc{WalletRepository{db}};
    svc.deposit(
        userId,
        amount,
        [cbp](WalletRow w) {
            auto resp = HttpResponse::newHttpJsonResponse(walletToJson(w));
            resp->setStatusCode(drogon::k200OK);
            (*cbp)(resp);
        },
        [cbp](const pm::ApiError &e) {
            (*cbp)(pm::jsonError(e));
        },
        [cbp](const drogon::orm::DrogonDbException &e) {
            (*cbp)(pm::jsonError({drogon::k503ServiceUnavailable, e.base().what()}));
        });
}
