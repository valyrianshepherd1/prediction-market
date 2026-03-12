#include "pm/ws/MarketRealtimePublisher.h"

#include "pm/ws/WsHub.h"

#include "pm/util/JsonSerializers.h"

#include <json/json.h>

#include <string>
#include <string_view>
#include <vector>

namespace {

    [[nodiscard]] std::string normalizeMarketReason(std::string_view reason) {
        if (reason == "market_created" ||
            reason == "market_updated" ||
            reason == "market_closed" ||
            reason == "market_resolved" ||
            reason == "market_archived") {
            return std::string(reason);
        }

        if (reason == "created") {
            return "market_created";
        }
        if (reason == "updated") {
            return "market_updated";
        }
        if (reason == "closed") {
            return "market_closed";
        }
        if (reason == "resolved") {
            return "market_resolved";
        }
        if (reason == "archived") {
            return "market_archived";
        }

        return std::string(reason);
    }

    Json::Value makeMarketPayload(const MarketRow &market,
                                  std::string_view reason) {
        Json::Value payload = pm::json::toJson(market);
        payload["reason"] = normalizeMarketReason(reason);
        return payload;
    }

    void publishMarketsInvalidate(const MarketRow &market,
                                  std::string_view reason) {
        Json::Value payload;
        payload["reason"] = normalizeMarketReason(reason);
        payload["market_id"] = market.id;
        payload["status"] = market.status;

        if (market.resolved_outcome_id) {
            payload["resolved_outcome_id"] = *market.resolved_outcome_id;
        } else {
            payload["resolved_outcome_id"] = Json::Value(Json::nullValue);
        }

        pm::ws::WsHub::instance().publish(
            "markets",
            "markets.invalidate",
            payload);
    }
}

namespace pm::ws {
    void publishMarketLifecycle(const MarketRow &market,
                                std::string_view reason) {
        WsHub::instance().publish(
            "market:" + market.id,
            "market.updated",
            makeMarketPayload(market, reason));

        publishMarketsInvalidate(market, reason);
    }

    void publishMarketLifecycle(const MarketRow &market,
                                const std::vector<OutcomeRow> &outcomes,
                                std::string_view reason) {
        Json::Value payload = makeMarketPayload(market, reason);
        payload["outcomes"] = Json::arrayValue;
        for (const auto &outcome: outcomes) {
            payload["outcomes"].append(pm::json::toJson(outcome));
        }

        WsHub::instance().publish(
            "market:" + market.id,
            "market.updated",
            payload);

        publishMarketsInvalidate(market, reason);
    }
} // namespace pm::ws
