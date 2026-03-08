#include "pm/controllers/PortfolioController.h"

#include "pm/repositories/PortfolioRepository.h"
#include "pm/services/PortfolioService.h"
#include "pm/util/ApiError.h"

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

pm::ApiError requireUserId(const drogon::HttpRequestPtr &req, std::string &outUserId) {
    outUserId = req->getHeader("X-User-Id");
    if (outUserId.empty()) {
        return {drogon::k401Unauthorized, "X-User-Id header is required (temporary auth)"};
    }
    return {drogon::k200OK, ""};
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

Json::Value positionToJson(const PortfolioPositionRow &p) {
    Json::Value j;
    j["user_id"] = p.user_id;
    j["outcome_id"] = p.outcome_id;
    j["market_id"] = p.market_id;
    j["market_question"] = p.market_question;
    j["outcome_title"] = p.outcome_title;
    j["outcome_index"] = p.outcome_index;
    j["shares_available"] = Json::Int64(p.shares_available);
    j["shares_reserved"] = Json::Int64(p.shares_reserved);
    j["shares_total"] = Json::Int64(p.shares_available + p.shares_reserved);
    j["updated_at"] = p.updated_at;
    return j;
}

Json::Value ledgerToJson(const PortfolioLedgerEntryRow &e) {
    Json::Value j;
    j["id"] = e.id;
    j["ledger_type"] = e.ledger_type;
    j["kind"] = e.kind;
    if (!e.outcome_id.empty()) {
        j["outcome_id"] = e.outcome_id;
    } else {
        j["outcome_id"] = Json::nullValue;
    }
    j["delta_available"] = Json::Int64(e.delta_available);
    j["delta_reserved"] = Json::Int64(e.delta_reserved);
    j["ref_type"] = e.ref_type;
    j["ref_id"] = e.ref_id;
    j["created_at"] = e.created_at;
    return j;
}

}  // namespace

void PortfolioController::listPortfolio(
    const drogon::HttpRequestPtr &req,
    std::function<void(const drogon::HttpResponsePtr &)> &&cb) const {
    auto cbp =
        std::make_shared<std::function<void(const drogon::HttpResponsePtr &)>>(std::move(cb));

    std::string userId;
    if (auto e = requireUserId(req, userId); !e) {
        return (*cbp)(pm::jsonError(e));
    }

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

    std::string dbErr;
    auto db = getDbOrNull(dbErr);
    if (!db) {
        return (*cbp)(pm::jsonError(
            {drogon::k500InternalServerError, "db client not available: " + dbErr}));
    }

    PortfolioService svc{PortfolioRepository{db}};
    svc.listPositions(
        userId,
        limit,
        offset,
        [cbp](std::vector<PortfolioPositionRow> rows) {
            Json::Value arr(Json::arrayValue);
            for (const auto &row : rows) {
                arr.append(positionToJson(row));
            }
            auto resp = HttpResponse::newHttpJsonResponse(arr);
            resp->setStatusCode(drogon::k200OK);
            (*cbp)(resp);
        },
        [cbp](const drogon::orm::DrogonDbException &e) {
            (*cbp)(pm::jsonError({drogon::k503ServiceUnavailable, e.base().what()}));
        });
}

void PortfolioController::listLedger(
    const drogon::HttpRequestPtr &req,
    std::function<void(const drogon::HttpResponsePtr &)> &&cb) const {
    auto cbp =
        std::make_shared<std::function<void(const drogon::HttpResponsePtr &)>>(std::move(cb));

    std::string userId;
    if (auto e = requireUserId(req, userId); !e) {
        return (*cbp)(pm::jsonError(e));
    }

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

    std::string dbErr;
    auto db = getDbOrNull(dbErr);
    if (!db) {
        return (*cbp)(pm::jsonError(
            {drogon::k500InternalServerError, "db client not available: " + dbErr}));
    }

    PortfolioService svc{PortfolioRepository{db}};
    svc.listLedger(
        userId,
        limit,
        offset,
        [cbp](std::vector<PortfolioLedgerEntryRow> rows) {
            Json::Value arr(Json::arrayValue);
            for (const auto &row : rows) {
                arr.append(ledgerToJson(row));
            }
            auto resp = HttpResponse::newHttpJsonResponse(arr);
            resp->setStatusCode(drogon::k200OK);
            (*cbp)(resp);
        },
        [cbp](const drogon::orm::DrogonDbException &e) {
            (*cbp)(pm::jsonError({drogon::k503ServiceUnavailable, e.base().what()}));
        });
}
