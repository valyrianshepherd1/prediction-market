#include "pm/ws/OrderRealtimePublisher.h"

#include "pm/util/JsonSerializers.h"
#include "pm/ws/WsHub.h"

#include <drogon/orm/DbClient.h>

#include <json/json.h>

#include <cstdint>
#include <string>
#include <string_view>

using drogon::orm::DbClientPtr;
using drogon::orm::DrogonDbException;
using drogon::orm::Result;

namespace {

    void publishOrderbookInvalidate(const OrderRow &order, std::string_view reason) {
        Json::Value payload;
        payload["reason"] = std::string(reason);
        payload["outcome_id"] = order.outcome_id;
        payload["order_id"] = order.id;

        pm::ws::WsHub::instance().publish(
            "orderbook:" + order.outcome_id,
            "orderbook.invalidate",
            payload);
    }

    void publishOrderChanged(const OrderRow &order, std::string_view reason) {
        Json::Value payload = pm::json::toJson(order);
        payload["reason"] = std::string(reason);

        pm::ws::WsHub::instance().publish(
            "user:" + order.user_id,
            "order.changed",
            payload);
    }

    void publishPositionSnapshot(const DbClientPtr &db,
                                 const std::string &userId,
                                 const std::string &outcomeId) {
        if (!db || userId.empty() || outcomeId.empty()) {
            return;
        }

        static constexpr std::string_view kSql =
            "SELECT user_id::text AS user_id, "
            "outcome_id::text AS outcome_id, "
            "shares_available, shares_reserved, updated_at::text AS updated_at "
            "FROM positions "
            "WHERE user_id = $1::uuid AND outcome_id = $2::uuid";

        db->execSqlAsync(
            std::string(kSql),
            [userId](const Result &r) {
                if (r.empty()) {
                    return;
                }

                Json::Value payload;
                payload["user_id"] = r[0]["user_id"].as<std::string>();
                payload["outcome_id"] = r[0]["outcome_id"].as<std::string>();
                payload["shares_available"] = Json::Int64(r[0]["shares_available"].as<std::int64_t>());
                payload["shares_reserved"] = Json::Int64(r[0]["shares_reserved"].as<std::int64_t>());
                payload["shares_total"] = Json::Int64(
                    r[0]["shares_available"].as<std::int64_t>() +
                    r[0]["shares_reserved"].as<std::int64_t>());
                payload["updated_at"] = r[0]["updated_at"].as<std::string>();

                pm::ws::WsHub::instance().publish(
                    "user:" + userId,
                    "user.position.updated",
                    payload);
            },
            [](const DrogonDbException &) {
                // best effort: realtime notification failure must not break request flow
            },
            userId,
            outcomeId);
    }

} // namespace

namespace pm::ws {

    bool OrderRealtimePublisher::supports(RealtimePublishKind kind) const noexcept {
        return kind == RealtimePublishKind::OrderLifecycle;
    }

    void OrderRealtimePublisher::publish(const RealtimePublishRequest &request) const {
        const auto *orderRequest = std::get_if<OrderLifecycleRequest>(&request);
        if (!orderRequest) {
            return;
        }

        publishOrderbookInvalidate(orderRequest->order, orderRequest->reason);
        publishOrderChanged(orderRequest->order, orderRequest->reason);

        if (!orderRequest->db) {
            return;
        }

        if (orderRequest->order.side == "BUY") {
            publishRealtime(WalletSnapshotRequest{orderRequest->db, orderRequest->order.user_id});
        } else if (orderRequest->order.side == "SELL") {
            publishPositionSnapshot(orderRequest->db,
                                    orderRequest->order.user_id,
                                    orderRequest->order.outcome_id);
        }
    }

} // namespace pm::ws
