#include "pm/controllers/OrderController.h"

#include "pm/repositories/OrderRepository.h"
#include "pm/services/OrderService.h"
#include "pm/util/ApiError.h"

#include <drogon/drogon.h>
#include <memory>
#include <string_view>

using drogon::HttpResponse;
using drogon::HttpResponsePtr;

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
        if (outUserId.empty()) return {drogon::k401Unauthorized, "X-User-Id header is required (temporary auth)"};
        return {drogon::k200OK, ""};
    }

    Json::Value orderToJson(const OrderRow &o) {
        Json::Value j;
        j["id"] = o.id;
        j["user_id"] = o.user_id;
        j["outcome_id"] = o.outcome_id;
        j["side"] = o.side;
        j["price_bp"] = o.price_bp;
        j["qty_total_micros"] = Json::Int64(o.qty_total_micros);
        j["qty_remaining_micros"] = Json::Int64(o.qty_remaining_micros);
        j["status"] = o.status;
        j["created_at"] = o.created_at;
        j["updated_at"] = o.updated_at;
        return j;
    }

    bool isSideValid(std::string_view s) {
        return s == "BUY" || s == "SELL";
    }
} // namespace

void OrderController::createOrder(const drogon::HttpRequestPtr &req,
                                  std::function<void(const HttpResponsePtr &)> &&cb) const {
    auto cbp = std::make_shared<std::function<void(const HttpResponsePtr &)> >(std::move(cb));

    std::string userId;
    if (auto e = requireUserId(req, userId); e.code != drogon::k200OK) {
        return (*cbp)(pm::jsonError(e));
    }

    const auto json = req->getJsonObject();
    if (!json) return (*cbp)(pm::jsonError({drogon::k400BadRequest, "expected JSON body"}));

    const auto outcomeId = (*json).get("outcome_id", "").asString();
    const auto side = (*json).get("side", "").asString();
    const int priceBp = (*json).get("price_bp", -1).asInt();
    const auto qtyMicros = (*json).get("qty_micros", 0).asInt64();

    if (outcomeId.empty()) return (*cbp)(pm::jsonError({drogon::k400BadRequest, "outcome_id is required"}));
    if (!isSideValid(side)) return (*cbp)(pm::jsonError({drogon::k400BadRequest, "side must be BUY|SELL"}));
    if (priceBp < 0 || priceBp > 10000) return (*cbp)(pm::jsonError({
        drogon::k400BadRequest, "price_bp must be 0..10000"
    }));
    if (qtyMicros <= 0) return (*cbp)(pm::jsonError({drogon::k400BadRequest, "qty_micros must be > 0"}));

    std::string dbErr;
    auto db = getDbOrNull(dbErr);
    if (!db) {
        return (*cbp)(pm::jsonError({drogon::k500InternalServerError, "db client not available: " + dbErr}));
    }

    OrderService svc{OrderRepository{db}};
    svc.createOrder(
        userId, outcomeId, side, priceBp, qtyMicros,
        [cbp](OrderRow created) {
            auto resp = HttpResponse::newHttpJsonResponse(orderToJson(created));
            resp->setStatusCode(drogon::k201Created);
            (*cbp)(resp);
        },
        [cbp](const pm::ApiError &e) {
            (*cbp)(pm::jsonError(e));
        },
        [cbp](const drogon::orm::DrogonDbException &e) {
            (*cbp)(pm::jsonError({drogon::k503ServiceUnavailable, e.base().what()}));
        });
}

void OrderController::getOrderBook(const drogon::HttpRequestPtr &req,
                                   std::function<void(const HttpResponsePtr &)> &&cb,
                                   std::string outcomeId) const {
    auto cbp = std::make_shared<std::function<void(const HttpResponsePtr &)> >(std::move(cb));

    int depth = 50;
    if (auto s = req->getParameter("depth"); !s.empty()) {
        try { depth = std::stoi(s); } catch (...) { depth = 50; }
    }

    std::string dbErr;
    auto db = getDbOrNull(dbErr);
    if (!db) {
        return (*cbp)(pm::jsonError({drogon::k500InternalServerError, "db client not available: " + dbErr}));
    }

    OrderService svc{OrderRepository{db}};
    svc.getOrderBook(
        outcomeId, depth,
        [cbp, outcomeId](OrderBook book) {
            Json::Value j;
            j["outcome_id"] = outcomeId;

            Json::Value buys(Json::arrayValue);
            for (const auto &o: book.buy) buys.append(orderToJson(o));

            Json::Value sells(Json::arrayValue);
            for (const auto &o: book.sell) sells.append(orderToJson(o));

            j["buy"] = std::move(buys);
            j["sell"] = std::move(sells);

            auto resp = HttpResponse::newHttpJsonResponse(j);
            resp->setStatusCode(drogon::k200OK);
            (*cbp)(resp);
        },
        [cbp](const drogon::orm::DrogonDbException &e) {
            (*cbp)(pm::jsonError({drogon::k503ServiceUnavailable, e.base().what()}));
        });
}
