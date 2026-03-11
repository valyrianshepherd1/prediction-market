#include "pm/ws/MarketWsController.h"

#include "pm/util/ApiError.h"
#include "pm/util/AuthGuard.h"
#include "pm/ws/TopicSnapshotPublisher.h"
#include "pm/ws/WsEnvelope.h"
#include "pm/ws/WsHub.h"

#include <drogon/drogon.h>
#include <json/json.h>

#include <array>
#include <memory>
#include <optional>
#include <string>
#include <utility>

namespace {
    constexpr std::size_t kMaxClientMessageSize = 4096;

    void sendSystemEvent(const drogon::WebSocketConnectionPtr &connection,
                         const std::string &event,
                         const Json::Value &payload = Json::Value(Json::objectValue),
                         const std::string &topic = {}) {
        if (!connection || !connection->connected()) {
            return;
        }

        connection->send(pm::ws::stringifyJson(
            pm::ws::makeEnvelope(event, payload, topic, pm::ws::makeMeta())));
    }

    Json::Value subscriptionsPayload(const drogon::WebSocketConnectionPtr &connection) {
        Json::Value payload;
        payload["topics"] = Json::arrayValue;
        for (const auto &topic: pm::ws::WsHub::instance().subscriptions(connection)) {
            payload["topics"].append(topic);
        }
        return payload;
    }

    std::optional<Json::Value> parseObjectMessage(const std::string &message,
                                                  std::string &error) {
        Json::CharReaderBuilder builder;
        std::string errs;
        Json::Value root;
        std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
        if (!reader->parse(message.data(), message.data() + message.size(), &root, &errs)) {
            error = errs.empty() ? "invalid JSON" : errs;
            return std::nullopt;
        }

        if (!root.isObject()) {
            error = "message must be a JSON object";
            return std::nullopt;
        }

        return root;
    }

    drogon::orm::DbClientPtr getDbOrNull(std::string &err) {
        try {
            return drogon::app().getDbClient("default");
        } catch (const std::exception &e) {
            err = e.what();
            return nullptr;
        }
    }
} // namespace

void MarketWsController::handleNewConnection(const drogon::HttpRequestPtr &request,
                                             const drogon::WebSocketConnectionPtr &connection) {
    pm::ws::WsHub::instance().registerConnection(connection);

    Json::Value welcomePayload;
    welcomePayload["message"] = "websocket connected";
    welcomePayload["actions"] = Json::arrayValue;
    for (const auto action: std::array{"subscribe", "unsubscribe", "list_topics", "ping", "snapshot"}) {
        welcomePayload["actions"].append(action);
    }
    welcomePayload["topics"] = Json::arrayValue;
    welcomePayload["topics"].append("orderbook:<outcome_id>");
    welcomePayload["topics"].append("trades:<outcome_id>");
    welcomePayload["topics"].append("user:<user_id|me>");
    welcomePayload["subscribe_options"] = Json::objectValue;
    welcomePayload["subscribe_options"]["with_snapshot"] = true;
    welcomePayload["message_meta"] = Json::objectValue;
    welcomePayload["message_meta"]["server_time_ms"] = "always";
    welcomePayload["message_meta"]["seq"] = "for topic events and snapshots";
    welcomePayload["message_meta"]["snapshot"] = "true for snapshot envelopes";
    welcomePayload["max_message_size"] = static_cast<Json::UInt64>(kMaxClientMessageSize);
    sendSystemEvent(connection, "system.welcome", welcomePayload);

    const auto authHeader = request->getHeader("Authorization");
    if (authHeader.empty()) {
        Json::Value hint;
        hint["message"] = "public topics are available immediately; private user:* topics require Authorization: Bearer <access_token> in the websocket handshake request";
        sendSystemEvent(connection, "system.auth.skipped", hint);
        return;
    }

    pm::auth::requireAuthenticatedUser(
        request,
        [connection](pm::auth::Principal principal) {
            Json::Value payload;
            payload["user_id"] = principal.user_id;
            payload["role"] = principal.role;
            pm::ws::WsHub::instance().setPrincipal(connection, std::move(principal));
            sendSystemEvent(connection, "system.authenticated", payload);
        },
        [connection](const pm::ApiError &error) {
            Json::Value payload;
            payload["code"] = static_cast<int>(error.code);
            payload["message"] = error.message;
            sendSystemEvent(connection, "system.auth.failed", payload);
        },
        [connection](const drogon::orm::DrogonDbException &error) {
            Json::Value payload;
            payload["message"] = error.base().what();
            sendSystemEvent(connection, "system.error", payload);
        });
}

void MarketWsController::handleNewMessage(const drogon::WebSocketConnectionPtr &connection,
                                          std::string &&message,
                                          const drogon::WebSocketMessageType &type) {
    if (type == drogon::WebSocketMessageType::Ping) {
        sendSystemEvent(connection, "system.pong");
        return;
    }

    if (type != drogon::WebSocketMessageType::Text) {
        Json::Value payload;
        payload["message"] = "only text JSON messages are supported";
        sendSystemEvent(connection, "system.error", payload);
        return;
    }

    if (message.size() > kMaxClientMessageSize) {
        Json::Value payload;
        payload["message"] = "message too large";
        payload["max_message_size"] = static_cast<Json::UInt64>(kMaxClientMessageSize);
        sendSystemEvent(connection, "system.error", payload);
        return;
    }

    std::string parseError;
    auto root = parseObjectMessage(message, parseError);
    if (!root) {
        Json::Value payload;
        payload["message"] = parseError;
        sendSystemEvent(connection, "system.error", payload);
        return;
    }

    const auto action = (*root).get("action", "").asString();
    if (action == "ping") {
        sendSystemEvent(connection, "system.pong");
        return;
    }

    if (action == "list_topics") {
        sendSystemEvent(connection, "system.topics", subscriptionsPayload(connection));
        return;
    }

    const auto topic = (*root).get("topic", "").asString();
    if (topic.empty()) {
        Json::Value payload;
        payload["message"] = "topic is required";
        sendSystemEvent(connection, "system.error", payload);
        return;
    }

    if (action == "subscribe") {
        std::string error;
        if (!pm::ws::WsHub::instance().subscribe(connection, topic, error)) {
            Json::Value payload;
            payload["message"] = error;
            sendSystemEvent(connection, "system.error", payload, topic);
            return;
        }

        sendSystemEvent(connection, "system.subscribed", subscriptionsPayload(connection), topic);

        if ((*root).get("with_snapshot", false).asBool()) {
            std::string normalizedTopic;
            if (!pm::ws::WsHub::instance().resolveTopic(connection, topic, normalizedTopic, error)) {
                Json::Value payload;
                payload["message"] = error;
                sendSystemEvent(connection, "system.error", payload, topic);
                return;
            }

            std::string dbErr;
            auto db = getDbOrNull(dbErr);
            if (!db) {
                Json::Value payload;
                payload["message"] = dbErr;
                sendSystemEvent(connection, "system.error", payload, normalizedTopic);
                return;
            }

            pm::ws::sendTopicSnapshot(connection, db, normalizedTopic, *root);
        }
        return;
    }

    if (action == "snapshot") {
        std::string normalizedTopic;
        std::string error;
        if (!pm::ws::WsHub::instance().resolveTopic(connection, topic, normalizedTopic, error)) {
            Json::Value payload;
            payload["message"] = error;
            sendSystemEvent(connection, "system.error", payload, topic);
            return;
        }

        std::string dbErr;
        auto db = getDbOrNull(dbErr);
        if (!db) {
            Json::Value payload;
            payload["message"] = dbErr;
            sendSystemEvent(connection, "system.error", payload, normalizedTopic);
            return;
        }

        pm::ws::sendTopicSnapshot(connection, db, normalizedTopic, *root);
        return;
    }

    if (action == "unsubscribe") {
        pm::ws::WsHub::instance().unsubscribe(connection, topic);
        sendSystemEvent(connection, "system.unsubscribed", subscriptionsPayload(connection), topic);
        return;
    }

    Json::Value payload;
    payload["message"] = "unknown action";
    payload["action"] = action;
    sendSystemEvent(connection, "system.error", payload);
}

void MarketWsController::handleConnectionClosed(const drogon::WebSocketConnectionPtr &connection) {
    pm::ws::WsHub::instance().unregisterConnection(connection);
}
