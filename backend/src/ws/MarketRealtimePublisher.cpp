#include "pm/ws/MarketRealtimePublisher.h"

#include "pm/util/JsonSerializers.h"
#include "pm/ws/WsHub.h"

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

} // namespace

namespace pm::ws {

    bool MarketRealtimePublisher::supports(RealtimePublishKind kind) const noexcept {
        return kind == RealtimePublishKind::MarketLifecycle;
    }

    void MarketRealtimePublisher::publish(const RealtimePublishRequest &request) const {
        const auto *marketRequest = std::get_if<MarketLifecycleRequest>(&request);
        if (!marketRequest) {
            return;
        }

        Json::Value payload = makeMarketPayload(marketRequest->market, marketRequest->reason);
        if (marketRequest->outcomes.has_value()) {
            payload["outcomes"] = Json::arrayValue;
            for (const auto &outcome: *marketRequest->outcomes) {
                payload["outcomes"].append(pm::json::toJson(outcome));
            }
        }

        WsHub::instance().publish(
            "market:" + marketRequest->market.id,
            "market.updated",
            payload);

        publishMarketsInvalidate(marketRequest->market, marketRequest->reason);
    }

} // namespace pm::ws
