#include "pm/controllers/OrderController.h"

#include "pm/repositories/OrderRepository.h"
#include "pm/repositories/TradeRepository.h"
#include "pm/services/OrderService.h"
#include "pm/services/TradeService.h"
#include "pm/util/ApiError.h"
#include "pm/util/AuthGuard.h"
#include "pm/ws/RealtimePublisher.h"

#include <drogon/drogon.h>
#include <memory>
#include <optional>
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

    HttpResponsePtr jsonError(drogon::HttpStatusCode code, std::string msg) {
        Json::Value j;
        j["error"] = std::move(msg);
        auto resp = HttpResponse::newHttpJsonResponse(j);
        resp->setStatusCode(code);
        return resp;
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

    Json::Value orderBookToJson(const OrderBook &b) {
        Json::Value j;
        j["buy"] = Json::arrayValue;
        j["sell"] = Json::arrayValue;
        for (const auto &o: b.buy) j["buy"].append(orderToJson(o));
        for (const auto &o: b.sell) j["sell"].append(orderToJson(o));
        return j;
    }
} // namespace

void OrderController::createOrder(const drogon::HttpRequestPtr &req,
                                  std::function<void(const HttpResponsePtr &)> &&cb) const {
    auto cbp = std::make_shared<std::function<void(const HttpResponsePtr &)> >(std::move(cb));

    pm::auth::requireAuthenticatedUser(
        req,
        [cbp, req](pm::auth::Principal principal) {
            auto json = req->getJsonObject();
            if (!json) return (*cbp)(jsonError(drogon::k400BadRequest, "JSON body is required"));

            const auto outcomeId = (*json).get("outcome_id", "").asString();
            const auto side = (*json).get("side", "").asString();
            const int priceBp = (*json).get("price_bp", 0).asInt();
            const auto qtyMicros = (*json)["qty_micros"].asInt64();

            if (outcomeId.empty() || side.empty() || qtyMicros <= 0)
                return (*cbp)(jsonError(drogon::k400BadRequest, "outcome_id, side, qty_micros are required"));

            std::string err;
            auto db = getDbOrNull(err);
            if (!db) return (*cbp)(jsonError(drogon::k500InternalServerError, err));

            OrderService svc{OrderRepository{db}};
            svc.createOrder(
                principal.user_id, outcomeId, side, priceBp, qtyMicros,
                [cbp, db](OrderRow created) {
                    pm::ws::publishOrderLifecycle(db, created, "order_created");

                    TradeService tradeSvc{TradeRepository{db}};
                    tradeSvc.listTradesForOrder(
                        created.id,
                        [db](std::vector<TradeRow> trades) {
                            for (const auto &trade: trades) {
                                pm::ws::publishTradeExecution(db, trade);
                            }
                        },
                        [](const drogon::orm::DrogonDbException &) {
                            // Best effort: the order is already persisted.
                        });

                    auto resp = HttpResponse::newHttpJsonResponse(orderToJson(created));
                    resp->setStatusCode(drogon::k201Created);
                    (*cbp)(resp);
                },
                [cbp](const pm::ApiError &e) {
                    auto [code, msg] = e;
                    (*cbp)(jsonError(code, msg));
                },
                [cbp](const drogon::orm::DrogonDbException &e) {
                    (*cbp)(jsonError(drogon::k503ServiceUnavailable, e.base().what()));
                });
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
    if (auto q = req->getParameter("depth"); !q.empty()) depth = std::atoi(q.c_str());

    std::string err;
    auto db = getDbOrNull(err);
    if (!db) return (*cbp)(jsonError(drogon::k500InternalServerError, err));

    OrderService svc{OrderRepository{db}};
    svc.getOrderBook(
        outcomeId, depth,
        [cbp](OrderBook b) {
            (*cbp)(HttpResponse::newHttpJsonResponse(orderBookToJson(b)));
        },
        [cbp](const drogon::orm::DrogonDbException &e) {
            (*cbp)(jsonError(drogon::k503ServiceUnavailable, e.base().what()));
        });
}

void OrderController::getOrder(const drogon::HttpRequestPtr &req,
                               std::function<void(const HttpResponsePtr &)> &&cb,
                               std::string orderId) const {
    auto cbp = std::make_shared<std::function<void(const HttpResponsePtr &)> >(std::move(cb));

    pm::auth::requireAuthenticatedUser(
        req,
        [cbp, orderId](pm::auth::Principal principal) {
            std::string err;
            auto db = getDbOrNull(err);
            if (!db) return (*cbp)(jsonError(drogon::k500InternalServerError, err));

            OrderService svc{OrderRepository{db}};
            svc.getOrderForUser(
                principal.user_id, orderId,
                [cbp](OrderRow o) {
                    (*cbp)(HttpResponse::newHttpJsonResponse(orderToJson(o)));
                },
                [cbp](const pm::ApiError &e) {
                    auto [code, msg] = e;
                    (*cbp)(jsonError(code, msg));
                },
                [cbp](const drogon::orm::DrogonDbException &e) {
                    (*cbp)(jsonError(drogon::k503ServiceUnavailable, e.base().what()));
                });
        },
        [cbp](const pm::ApiError &e) { (*cbp)(pm::jsonError(e)); },
        [cbp](const drogon::orm::DrogonDbException &e) {
            (*cbp)(pm::jsonError({drogon::k503ServiceUnavailable, e.base().what()}));
        });
}

void OrderController::listMyOrders(const drogon::HttpRequestPtr &req,
                                   std::function<void(const HttpResponsePtr &)> &&cb) const {
    auto cbp = std::make_shared<std::function<void(const HttpResponsePtr &)> >(std::move(cb));

    pm::auth::requireAuthenticatedUser(
        req,
        [cbp, req](pm::auth::Principal principal) {
            int limit = 50;
            int offset = 0;
            if (auto q = req->getParameter("limit"); !q.empty()) limit = std::atoi(q.c_str());
            if (auto q = req->getParameter("offset"); !q.empty()) offset = std::atoi(q.c_str());

            std::optional<std::string> status;
            if (auto s = req->getParameter("status"); !s.empty()) status = s;

            std::string err;
            auto db = getDbOrNull(err);
            if (!db) return (*cbp)(jsonError(drogon::k500InternalServerError, err));

            OrderService svc{OrderRepository{db}};
            svc.listOrdersForUser(
                principal.user_id, status, limit, offset,
                [cbp](std::vector<OrderRow> orders) {
                    Json::Value arr = Json::arrayValue;
                    for (const auto &o: orders) arr.append(orderToJson(o));
                    (*cbp)(HttpResponse::newHttpJsonResponse(arr));
                },
                [cbp](const drogon::orm::DrogonDbException &e) {
                    (*cbp)(jsonError(drogon::k503ServiceUnavailable, e.base().what()));
                });
        },
        [cbp](const pm::ApiError &e) { (*cbp)(pm::jsonError(e)); },
        [cbp](const drogon::orm::DrogonDbException &e) {
            (*cbp)(pm::jsonError({drogon::k503ServiceUnavailable, e.base().what()}));
        });
}

void OrderController::cancelOrder(const drogon::HttpRequestPtr &req,
                                  std::function<void(const HttpResponsePtr &)> &&cb,
                                  std::string orderId) const {
    auto cbp = std::make_shared<std::function<void(const HttpResponsePtr &)> >(std::move(cb));

    pm::auth::requireAuthenticatedUser(
        req,
        [cbp, orderId](pm::auth::Principal principal) {
            std::string err;
            auto db = getDbOrNull(err);
            if (!db) return (*cbp)(jsonError(drogon::k500InternalServerError, err));

            OrderService svc{OrderRepository{db}};
            svc.cancelOrderForUser(
                principal.user_id, orderId,
                [cbp, db](OrderRow o) {
                    pm::ws::publishOrderLifecycle(db, o, "order_canceled");
                    (*cbp)(HttpResponse::newHttpJsonResponse(orderToJson(o)));
                },
                [cbp](const pm::ApiError &e) {
                    auto [code, msg] = e;
                    (*cbp)(jsonError(code, msg));
                },
                [cbp](const drogon::orm::DrogonDbException &e) {
                    (*cbp)(jsonError(drogon::k503ServiceUnavailable, e.base().what()));
                });
        },
        [cbp](const pm::ApiError &e) { (*cbp)(pm::jsonError(e)); },
        [cbp](const drogon::orm::DrogonDbException &e) {
            (*cbp)(pm::jsonError({drogon::k503ServiceUnavailable, e.base().what()}));
        });
}
