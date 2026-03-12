#include "pm/controllers/PortfolioController.h"

#include "pm/repositories/PortfolioRepository.h"
#include "pm/services/PortfolioService.h"
#include "pm/util/ApiError.h"
#include "pm/util/AuthGuard.h"
#include "pm/util/JsonSerializers.h"

#include <charconv>
#include <memory>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

using drogon::HttpResponse;

namespace {

drogon::orm::DbClientPtr getDbOrNull(std::string &err) {
    try {
        return drogon::app().getDbClient("default");
    } catch (const std::exception &e) {
        err = e.what();
        return nullptr;
    }
}

bool parseInt(std::string_view s, int &out) {
    if (s.empty()) {
        return false;
    }
    int v{};
    auto [p, ec] = std::from_chars(s.data(), s.data() + s.size(), v);
    if (ec != std::errc{} || p != s.data() + s.size()) {
        return false;
    }
    out = v;
    return true;
}
}  // namespace

void PortfolioController::listPortfolio(
    const drogon::HttpRequestPtr &req,
    std::function<void(const drogon::HttpResponsePtr &)> &&cb) const {
    auto cbp =
        std::make_shared<std::function<void(const drogon::HttpResponsePtr &)>>(std::move(cb));

    int limit = 50;
    int offset = 0;
    {
        int v{};
        if (parseInt(req->getParameter("limit"), v)) {
            limit = v;
        }
        if (parseInt(req->getParameter("offset"), v)) {
            offset = v;
        }
    }

    pm::auth::requireAuthenticatedUser(
        req,
        [cbp, limit, offset](pm::auth::Principal principal) {
            std::string dbErr;
            auto db = getDbOrNull(dbErr);
            if (!db) {
                return (*cbp)(pm::jsonError(
                    {drogon::k500InternalServerError, "db client not available: " + dbErr}));
            }

            PortfolioService svc{PortfolioRepository{db}};
            svc.listPositions(
                principal.user_id,
                limit,
                offset,
                [cbp](std::vector<PortfolioPositionRow> rows) {
                    Json::Value arr(Json::arrayValue);
                    for (const auto &row : rows) {
                        arr.append(pm::json::toJson(row));
                    }
                    auto resp = HttpResponse::newHttpJsonResponse(arr);
                    resp->setStatusCode(drogon::k200OK);
                    (*cbp)(resp);
                },
                [cbp](const drogon::orm::DrogonDbException &e) {
                    (*cbp)(pm::jsonError({drogon::k503ServiceUnavailable, e.base().what()}));
                });
        },
        [cbp](const pm::ApiError &e) { (*cbp)(pm::jsonError(e)); },
        [cbp](const drogon::orm::DrogonDbException &e) {
            (*cbp)(pm::jsonError({drogon::k503ServiceUnavailable, e.base().what()}));
        });
}

void PortfolioController::listLedger(
    const drogon::HttpRequestPtr &req,
    std::function<void(const drogon::HttpResponsePtr &)> &&cb) const {
    auto cbp =
        std::make_shared<std::function<void(const drogon::HttpResponsePtr &)>>(std::move(cb));

    int limit = 50;
    int offset = 0;
    {
        int v{};
        if (parseInt(req->getParameter("limit"), v)) {
            limit = v;
        }
        if (parseInt(req->getParameter("offset"), v)) {
            offset = v;
        }
    }

    pm::auth::requireAuthenticatedUser(
        req,
        [cbp, limit, offset](pm::auth::Principal principal) {
            std::string dbErr;
            auto db = getDbOrNull(dbErr);
            if (!db) {
                return (*cbp)(pm::jsonError(
                    {drogon::k500InternalServerError, "db client not available: " + dbErr}));
            }

            PortfolioService svc{PortfolioRepository{db}};
            svc.listLedger(
                principal.user_id,
                limit,
                offset,
                [cbp](std::vector<PortfolioLedgerEntryRow> rows) {
                    Json::Value arr(Json::arrayValue);
                    for (const auto &row : rows) {
                        arr.append(pm::json::toJson(row));
                    }
                    auto resp = HttpResponse::newHttpJsonResponse(arr);
                    resp->setStatusCode(drogon::k200OK);
                    (*cbp)(resp);
                },
                [cbp](const drogon::orm::DrogonDbException &e) {
                    (*cbp)(pm::jsonError({drogon::k503ServiceUnavailable, e.base().what()}));
                });
        },
        [cbp](const pm::ApiError &e) { (*cbp)(pm::jsonError(e)); },
        [cbp](const drogon::orm::DrogonDbException &e) {
            (*cbp)(pm::jsonError({drogon::k503ServiceUnavailable, e.base().what()}));
        });
}
