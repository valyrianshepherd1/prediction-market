#include "pm/controllers/TradeController.h"

#include "pm/repositories/TradeRepository.h"
#include "pm/services/TradeService.h"
#include "pm/util/ApiError.h"

#include <charconv>
#include <string_view>

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

    bool parseIntParam(std::string_view s, int &out) {
        if (s.empty()) return false;
        int v{};
        auto *b = s.data();
        auto *e = s.data() + s.size();
        auto [p, ec] = std::from_chars(b, e, v);
        if (ec != std::errc{} || p != e) return false;
        out = v;
        return true;
    }

    Json::Value tradeToJson(const TradeRow &t) {
        Json::Value j;
        j["id"] = t.id;
        j["outcome_id"] = t.outcome_id;
        j["maker_user_id"] = t.maker_user_id;
        j["taker_user_id"] = t.taker_user_id;
        j["maker_order_id"] = t.maker_order_id;
        j["taker_order_id"] = t.taker_order_id;
        j["price_bp"] = t.price_bp;
        j["qty_micros"] = Json::Int64(t.qty_micros);
        j["created_at"] = t.created_at;
        return j;
    }
} // namespace

void TradeController::listTradesByOutcome(const drogon::HttpRequestPtr &req,
                                          std::function<void(const drogon::HttpResponsePtr &)> &&cb,
                                          std::string outcomeId) const {
    auto cbp = std::make_shared<std::function<void(const drogon::HttpResponsePtr &)> >(
        std::move(cb));

    int limit = 50, offset = 0;
    {
        int v{};
        if (parseIntParam(req->getParameter("limit"), v)) limit = v;
        if (parseIntParam(req->getParameter("offset"), v)) offset = v;
    }

    std::string dbErr;
    auto db = getDbOrNull(dbErr);
    if (!db) return (*cbp)(pm::jsonError({drogon::k500InternalServerError, "db client not available: " + dbErr}));

    TradeService svc{TradeRepository{db}};
    svc.listTradesByOutcome(outcomeId, limit, offset,
                            [cbp](std::vector<TradeRow> rows) {
                                Json::Value arr(Json::arrayValue);
                                for (const auto &r: rows) arr.append(tradeToJson(r));
                                auto resp = HttpResponse::newHttpJsonResponse(arr);
                                resp->setStatusCode(drogon::k200OK);
                                (*cbp)(resp);
                            },
                            [cbp](const drogon::orm::DrogonDbException &e) {
                                (*cbp)(pm::jsonError({drogon::k503ServiceUnavailable, e.base().what()}));
                            });
}

void TradeController::listTradesByUser(const drogon::HttpRequestPtr &req,
                                       std::function<void(const drogon::HttpResponsePtr &)> &&cb,
                                       std::string userId) const {
    auto cbp = std::make_shared<std::function<void(const drogon::HttpResponsePtr &)> >(
        std::move(cb));

    int limit = 50, offset = 0;
    {
        int v{};
        if (parseIntParam(req->getParameter("limit"), v)) limit = v;
        if (parseIntParam(req->getParameter("offset"), v)) offset = v;
    }

    std::string dbErr;
    auto db = getDbOrNull(dbErr);
    if (!db) return (*cbp)(pm::jsonError({drogon::k500InternalServerError, "db client not available: " + dbErr}));

    TradeService svc{TradeRepository{db}};
    svc.listTradesByUser(userId, limit, offset,
                         [cbp](std::vector<TradeRow> rows) {
                             Json::Value arr(Json::arrayValue);
                             for (const auto &r: rows) arr.append(tradeToJson(r));
                             auto resp = HttpResponse::newHttpJsonResponse(arr);
                             resp->setStatusCode(drogon::k200OK);
                             (*cbp)(resp);
                         },
                         [cbp](const drogon::orm::DrogonDbException &e) {
                             (*cbp)(pm::jsonError({drogon::k503ServiceUnavailable, e.base().what()}));
                         });
}
