#include "pm/controllers/WalletController.h"

#include "pm/repositories/WalletRepository.h"
#include "pm/services/WalletService.h"
#include "pm/util/ApiError.h"
#include "pm/util/AuthGuard.h"
#include "pm/util/JsonSerializers.h"
#include "pm/ws/RealtimePublisher.h"

#include <cstdlib>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

using drogon::HttpResponse;
using drogon::HttpResponsePtr;

namespace {
using ResponseCallback = std::function<void(const drogon::HttpResponsePtr &)>;


drogon::orm::DbClientPtr getDbOrNull(std::string &err) {
    try {
        return drogon::app().getDbClient("default");
    } catch (const std::exception &e) {
        err = e.what();
        return nullptr;
    }
}
}  // namespace

bool WalletController::isAdmin(const drogon::HttpRequestPtr &) {
    return false;
}

pm::ApiError WalletController::adminAuthError() {
    return {drogon::k401Unauthorized, "admin bearer token missing or invalid"};
}

void WalletController::getMyWallet(
    const drogon::HttpRequestPtr &req,
    std::function<void(const drogon::HttpResponsePtr &)> &&cb) const {
    auto cbp = std::make_shared<ResponseCallback>(std::move(cb));

    pm::auth::requireAuthenticatedUser(
        req,
        [cbp](pm::auth::Principal principal) {
            std::string dbErr;
            auto db = getDbOrNull(dbErr);
            if (!db) {
                return (*cbp)(pm::jsonError(
                    {drogon::k500InternalServerError, "db client not available: " + dbErr}));
            }

            WalletService svc{WalletRepository{db}};
            svc.getWallet(
                principal.user_id,
                [cbp](std::optional<WalletRow> w) {
                    if (!w) {
                        return (*cbp)(pm::jsonError(
                            {drogon::k404NotFound, "wallet not found"}));
                    }
                    auto resp = HttpResponse::newHttpJsonResponse(pm::json::toJson(*w));
                    resp->setStatusCode(drogon::k200OK);
                    (*cbp)(resp);
                },
                [cbp](const drogon::orm::DrogonDbException &e) {
                    (*cbp)(pm::jsonError(
                        {drogon::k503ServiceUnavailable, e.base().what()}));
                });
        },
        [cbp](const pm::ApiError &e) { (*cbp)(pm::jsonError(e)); },
        [cbp](const drogon::orm::DrogonDbException &e) {
            (*cbp)(pm::jsonError({drogon::k503ServiceUnavailable, e.base().what()}));
        });
}

void WalletController::getWallet(
    const drogon::HttpRequestPtr &req,
    std::function<void(const drogon::HttpResponsePtr &)> &&cb,
    std::string userId) const {
    auto cbp = std::make_shared<ResponseCallback>(std::move(cb));

    pm::auth::requireAdminUser(
        req,
        [cbp, userId = std::move(userId)](pm::auth::Principal) {
            std::string dbErr;
            auto db = getDbOrNull(dbErr);
            if (!db) {
                return (*cbp)(pm::jsonError(
                    {drogon::k500InternalServerError, "db client not available: " + dbErr}));
            }

            WalletService svc{WalletRepository{db}};
            svc.getWallet(
                userId,
                [cbp](std::optional<WalletRow> w) {
                    if (!w) {
                        return (*cbp)(pm::jsonError(
                            {drogon::k404NotFound, "wallet not found"}));
                    }
                    auto resp = HttpResponse::newHttpJsonResponse(pm::json::toJson(*w));
                    resp->setStatusCode(drogon::k200OK);
                    (*cbp)(resp);
                },
                [cbp](const drogon::orm::DrogonDbException &e) {
                    (*cbp)(pm::jsonError(
                        {drogon::k503ServiceUnavailable, e.base().what()}));
                });
        },
        [cbp](const pm::ApiError &e) { (*cbp)(pm::jsonError(e)); },
        [cbp](const drogon::orm::DrogonDbException &e) {
            (*cbp)(pm::jsonError({drogon::k503ServiceUnavailable, e.base().what()}));
        });
}

void WalletController::adminDeposit(
    const drogon::HttpRequestPtr &req,
    std::function<void(const drogon::HttpResponsePtr &)> &&cb) const {
    auto cbp = std::make_shared<ResponseCallback>(std::move(cb));

    pm::auth::requireAdminUser(
        req,
        [cbp, req](pm::auth::Principal) {
            const auto json = req->getJsonObject();
            if (!json) {
                return (*cbp)(pm::jsonError(
                    {drogon::k400BadRequest, "expected JSON body"}));
            }

            const auto userId = (*json).get("user_id", "").asString();
            const auto amount = (*json).get("amount", 0).asInt64();
            if (userId.empty()) {
                return (*cbp)(pm::jsonError(
                    {drogon::k400BadRequest, "user_id is required"}));
            }

            std::string dbErr;
            auto db = getDbOrNull(dbErr);
            if (!db) {
                return (*cbp)(pm::jsonError(
                    {drogon::k500InternalServerError, "db client not available: " + dbErr}));
            }

            WalletService svc{WalletRepository{db}};
            svc.deposit(
                userId,
                amount,
                [cbp, db](WalletRow w) {
                    pm::ws::publishRealtime(pm::ws::WalletSnapshotRequest{db, w.user_id});

                    auto resp = HttpResponse::newHttpJsonResponse(pm::json::toJson(w));
                    resp->setStatusCode(drogon::k200OK);
                    (*cbp)(resp);
                },
                [cbp](const pm::ApiError &e) { (*cbp)(pm::jsonError(e)); },
                [cbp](const drogon::orm::DrogonDbException &e) {
                    (*cbp)(pm::jsonError(
                        {drogon::k503ServiceUnavailable, e.base().what()}));
                });
        },
        [cbp](const pm::ApiError &e) { (*cbp)(pm::jsonError(e)); },
        [cbp](const drogon::orm::DrogonDbException &e) {
            (*cbp)(pm::jsonError({drogon::k503ServiceUnavailable, e.base().what()}));
        });
}
