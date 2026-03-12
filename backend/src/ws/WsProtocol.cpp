#include "pm/ws/WsProtocol.h"

#include <initializer_list>
#include <string>
#include <string_view>

namespace {
    Json::Value makeStringArray(std::initializer_list<const char *> values) {
        Json::Value arr(Json::arrayValue);
        for (const auto *value : values) {
            arr.append(value);
        }
        return arr;
    }

    Json::Value makeEnvelopeContract() {
        Json::Value payload(Json::objectValue);
        payload["event"] = "string";
        payload["topic"] = "string (optional for control-plane events)";
        payload["payload"] = "json value";
        payload["meta"] = Json::objectValue;
        payload["meta"]["server_time_ms"] = "uint64, always present";
        payload["meta"]["seq"] = "uint64, present on topic events and snapshots";
        payload["meta"]["snapshot"] = "boolean, present on *.snapshot";
        return payload;
    }

    Json::Value makeTopicSpec(std::string_view pattern,
                              std::string_view access,
                              std::initializer_list<const char *> liveEvents,
                              std::string_view snapshotEvent,
                              const Json::Value &snapshotOptions,
                              std::string_view note = {}) {
        Json::Value spec(Json::objectValue);
        spec["pattern"] = std::string(pattern);
        spec["access"] = std::string(access);
        spec["supports_snapshot"] = !snapshotEvent.empty();
        spec["supports_replay"] = true;
        spec["live_events"] = makeStringArray(liveEvents);
        spec["snapshot_event"] = snapshotEvent.empty() ? Json::Value(Json::nullValue)
                                                         : Json::Value(std::string(snapshotEvent));
        spec["subscribe_options"] = Json::objectValue;
        spec["subscribe_options"]["with_snapshot"] = "boolean";
        spec["subscribe_options"]["since_seq"] = "optional uint64";
        spec["subscribe_options"]["replay_limit"] = "1..256";
        spec["snapshot_options"] = snapshotOptions;
        if (!note.empty()) {
            spec["note"] = std::string(note);
        }
        return spec;
    }

    Json::Value makeTopicContracts() {
        Json::Value topics(Json::arrayValue);

        Json::Value marketsOptions(Json::objectValue);
        marketsOptions["limit"] = "1..200";
        marketsOptions["offset"] = ">=0";
        marketsOptions["status"] = "optional string";
        topics.append(makeTopicSpec(
            "markets",
            "public",
            {"markets.invalidate"},
            "markets.snapshot",
            marketsOptions,
            "list-level feed for market discovery"));

        Json::Value marketOptions(Json::objectValue);
        topics.append(makeTopicSpec(
            "market:<market_id>",
            "public",
            {"market.updated"},
            "market.snapshot",
            marketOptions,
            "detail feed for a single market"));

        Json::Value orderbookOptions(Json::objectValue);
        orderbookOptions["depth"] = "1..200";
        topics.append(makeTopicSpec(
            "orderbook:<outcome_id>",
            "public",
            {"orderbook.invalidate"},
            "orderbook.snapshot",
            orderbookOptions,
            "use snapshot or REST to re-fetch the aggregated orderbook"));

        Json::Value tradesOptions(Json::objectValue);
        tradesOptions["limit"] = "1..200";
        tradesOptions["offset"] = ">=0";
        topics.append(makeTopicSpec(
            "trades:<outcome_id>",
            "public",
            {"trade.created"},
            "trades.snapshot",
            tradesOptions));

        Json::Value userOptions(Json::objectValue);
        userOptions["positions_limit"] = "1..200";
        userOptions["positions_offset"] = ">=0";
        userOptions["orders_limit"] = "1..200";
        userOptions["orders_offset"] = ">=0";
        topics.append(makeTopicSpec(
            "user:<user_id|me>",
            "private",
            {"user.wallet.updated", "user.position.updated", "user.trade.created", "order.changed"},
            "user.snapshot",
            userOptions,
            "user:me is accepted as a client alias and normalized to user:<uuid>"));

        return topics;
    }

    Json::Value makeControlEvents() {
        Json::Value control(Json::arrayValue);
        for (const auto *event : {
                 "system.welcome",
                 "system.authenticated",
                 "system.auth.skipped",
                 "system.auth.failed",
                 "system.subscribed",
                 "system.unsubscribed",
                 "system.topics",
                 "system.replayed",
                 "system.describe",
                 "system.pong",
                 "system.error",
             }) {
            control.append(event);
        }
        return control;
    }

    Json::Value makeActions() {
        Json::Value actions(Json::arrayValue);
        for (const auto *action : {
                 "subscribe",
                 "unsubscribe",
                 "list_topics",
                 "ping",
                 "snapshot",
                 "replay",
                 "describe",
             }) {
            actions.append(action);
        }
        return actions;
    }

    Json::Value findTopicContract(std::string_view topic) {
        const auto contracts = makeTopicContracts();
        if (topic == "markets") {
            return contracts[0u];
        }
        if (topic.rfind("market:", 0) == 0) {
            return contracts[1u];
        }
        if (topic.rfind("orderbook:", 0) == 0) {
            return contracts[2u];
        }
        if (topic.rfind("trades:", 0) == 0) {
            return contracts[3u];
        }
        if (topic.rfind("user:", 0) == 0) {
            return contracts[4u];
        }
        return Json::Value(Json::nullValue);
    }
} // namespace

namespace pm::ws {
    Json::Value makeWelcomePayload() {
        Json::Value payload(Json::objectValue);
        payload["message"] = "websocket connected";
        payload["protocol_version"] = std::string(protocolVersion());
        payload["actions"] = makeActions();
        payload["topics"] = makeTopicContracts();
        payload["control_events"] = makeControlEvents();
        payload["envelope"] = makeEnvelopeContract();
        payload["max_message_size"] = Json::UInt64(4096);
        return payload;
    }

    Json::Value makeDescribePayload() {
        Json::Value payload(Json::objectValue);
        payload["protocol_version"] = std::string(protocolVersion());
        payload["actions"] = makeActions();
        payload["control_events"] = makeControlEvents();
        payload["topics"] = makeTopicContracts();
        payload["envelope"] = makeEnvelopeContract();
        return payload;
    }

    Json::Value makeDescribePayloadForTopic(std::string_view requestedTopic,
                                            std::string_view normalizedTopic) {
        Json::Value payload(Json::objectValue);
        payload["protocol_version"] = std::string(protocolVersion());
        payload["requested_topic"] = std::string(requestedTopic);
        payload["normalized_topic"] = std::string(normalizedTopic);
        payload["contract"] = findTopicContract(normalizedTopic);
        return payload;
    }
} // namespace pm::ws
