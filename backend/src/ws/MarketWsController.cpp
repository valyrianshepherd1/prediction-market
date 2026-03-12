#include "pm/ws/MarketWsController.h"

#include "pm/util/ApiError.h"
#include "pm/util/AuthGuard.h"
#include "pm/util/JsonSerializers.h"
#include "pm/ws/TopicSnapshotPublisher.h"
#include "pm/ws/WsCommand.h"
#include "pm/ws/WsEnvelope.h"
#include "pm/ws/WsHub.h"
#include "pm/ws/WsProtocol.h"

#include <drogon/drogon.h>
#include <json/json.h>

#include <cstddef>
#include <string>
#include <utility>

namespace {
    constexpr std::size_t kMaxClientMessageSize = 4096;

    void sendSystemEvent(const drogon::WebSocketConnectionPtr &connection,
                         const std::string &event,
                         const Json::Value &payload = Json::Value(Json::objectValue),
                         const std::string &topic = {}) {
        const auto envelope = pm::ws::makeEnvelope(event, payload, topic, pm::ws::makeMeta());
        connection->send(pm::json::stringify(envelope));
    }

    void sendSystemError(const drogon::WebSocketConnectionPtr &connection,
                         const pm::ApiError &error,
                         const std::string &topic = {}) {
        Json::Value payload(Json::objectValue);
        payload["code"] = static_cast<int>(error.code);
        payload["message"] = error.message;
        sendSystemEvent(connection, "system.error", payload, topic);
    }

    Json::Value subscriptionsPayload(const drogon::WebSocketConnectionPtr &connection) {
        Json::Value payload(Json::objectValue);
        payload["topics"] = Json::arrayValue;
        for (const auto &topic: pm::ws::WsHub::instance().subscriptions(connection)) {
            payload["topics"].append(topic);
        }
        return payload;
    }

    drogon::orm::DbClientPtr getDbOrNull(std::string &err) {
        try {
            return drogon::app().getDbClient("default");
        } catch (const std::exception &e) {
            err = e.what();
            return nullptr;
        }
    }

    void sendReplayResult(const drogon::WebSocketConnectionPtr &connection,
                          const std::string &controlTopic,
                          std::uint64_t sinceSeq,
                          const pm::ws::WsHub::ReplayResult &result) {
        Json::Value payload(Json::objectValue);
        payload["since_seq"] = Json::UInt64(sinceSeq);
        payload["current_seq"] = Json::UInt64(result.current_seq);
        payload["replayed_count"] = Json::UInt64(result.replayed_count);
        payload["gap_detected"] = result.gap_detected;
        payload["truncated"] = result.truncated;
        if (result.replayed_count > 0) {
            payload["last_replayed_seq"] = Json::UInt64(result.last_replayed_seq);
        } else {
            payload["last_replayed_seq"] = Json::nullValue;
        }
        sendSystemEvent(connection, "system.replayed", payload, controlTopic);
    }

    bool resolveTopicOrSendError(const drogon::WebSocketConnectionPtr &connection,
                                 const std::string &requestedTopic,
                                 std::string &normalizedTopic) {
        std::string error;
        if (pm::ws::WsHub::instance().resolveTopic(connection, requestedTopic, normalizedTopic, error)) {
            return true;
        }

        sendSystemError(connection, {drogon::k400BadRequest, error}, requestedTopic);
        return false;
    }
} // namespace

void MarketWsController::handleNewConnection(const drogon::HttpRequestPtr &request,
                                             const drogon::WebSocketConnectionPtr &connection) {
    pm::ws::WsHub::instance().registerConnection(connection);
    sendSystemEvent(connection, "system.welcome", pm::ws::makeWelcomePayload());

    const auto authHeader = request->getHeader("Authorization");
    if (authHeader.empty()) {
        Json::Value hint(Json::objectValue);
        hint["message"] = "public topics are available immediately; private user:* topics require Authorization: Bearer <access_token> in the websocket handshake request";
        sendSystemEvent(connection, "system.auth.skipped", hint);
        return;
    }

    pm::auth::requireAuthenticatedUser(
        request,
        [connection](pm::auth::Principal principal) {
            Json::Value payload(Json::objectValue);
            payload["user_id"] = principal.user_id;
            payload["role"] = principal.role;
            pm::ws::WsHub::instance().setPrincipal(connection, std::move(principal));
            sendSystemEvent(connection, "system.authenticated", payload);
        },
        [connection](const pm::ApiError &error) {
            Json::Value payload(Json::objectValue);
            payload["code"] = static_cast<int>(error.code);
            payload["message"] = error.message;
            sendSystemEvent(connection, "system.auth.failed", payload);
        },
        [connection](const drogon::orm::DrogonDbException &error) {
            sendSystemError(connection, {drogon::k500InternalServerError, error.base().what()});
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
        sendSystemError(connection, {drogon::k400BadRequest, "only text JSON messages are supported"});
        return;
    }

    auto parsed = pm::ws::parseClientCommand(message, kMaxClientMessageSize);
    if (!parsed) {
        sendSystemError(connection, parsed.error());
        return;
    }

    const auto &command = parsed.value();
    switch (command.action) {
        case pm::ws::ClientAction::Ping:
            sendSystemEvent(connection, "system.pong");
            return;

        case pm::ws::ClientAction::ListTopics:
            sendSystemEvent(connection, "system.topics", subscriptionsPayload(connection));
            return;

        case pm::ws::ClientAction::Describe: {
            if (command.topic.empty()) {
                sendSystemEvent(connection, "system.describe", pm::ws::makeDescribePayload());
                return;
            }

            std::string normalizedTopic;
            if (!resolveTopicOrSendError(connection, command.topic, normalizedTopic)) {
                return;
            }
            sendSystemEvent(
                connection,
                "system.describe",
                pm::ws::makeDescribePayloadForTopic(command.topic, normalizedTopic),
                command.topic);
            return;
        }

        case pm::ws::ClientAction::Subscribe: {
            std::string error;
            if (!pm::ws::WsHub::instance().subscribe(connection, command.topic, error)) {
                sendSystemError(connection, {drogon::k400BadRequest, error}, command.topic);
                return;
            }

            std::string normalizedTopic;
            if (!resolveTopicOrSendError(connection, command.topic, normalizedTopic)) {
                return;
            }

            sendSystemEvent(connection, "system.subscribed", subscriptionsPayload(connection), command.topic);

            if (command.since_seq.has_value()) {
                pm::ws::WsHub::ReplayResult replayResult;
                if (!pm::ws::WsHub::instance().replaySince(
                        connection,
                        normalizedTopic,
                        *command.since_seq,
                        command.replay_limit,
                        replayResult,
                        error)) {
                    sendSystemError(connection, {drogon::k400BadRequest, error}, normalizedTopic);
                    return;
                }

                sendReplayResult(connection, command.topic, *command.since_seq, replayResult);
                if (command.with_snapshot && replayResult.gap_detected) {
                    std::string dbErr;
                    auto db = getDbOrNull(dbErr);
                    if (!db) {
                        sendSystemError(connection, {drogon::k500InternalServerError, dbErr}, normalizedTopic);
                        return;
                    }
                    pm::ws::sendTopicSnapshot(connection, db, normalizedTopic, command.root);
                }
                return;
            }

            if (command.with_snapshot) {
                std::string dbErr;
                auto db = getDbOrNull(dbErr);
                if (!db) {
                    sendSystemError(connection, {drogon::k500InternalServerError, dbErr}, normalizedTopic);
                    return;
                }
                pm::ws::sendTopicSnapshot(connection, db, normalizedTopic, command.root);
            }
            return;
        }

        case pm::ws::ClientAction::Snapshot: {
            std::string normalizedTopic;
            if (!resolveTopicOrSendError(connection, command.topic, normalizedTopic)) {
                return;
            }

            std::string dbErr;
            auto db = getDbOrNull(dbErr);
            if (!db) {
                sendSystemError(connection, {drogon::k500InternalServerError, dbErr}, normalizedTopic);
                return;
            }

            pm::ws::sendTopicSnapshot(connection, db, normalizedTopic, command.root);
            return;
        }

        case pm::ws::ClientAction::Replay: {
            std::string normalizedTopic;
            if (!resolveTopicOrSendError(connection, command.topic, normalizedTopic)) {
                return;
            }

            std::string error;
            pm::ws::WsHub::ReplayResult replayResult;
            if (!pm::ws::WsHub::instance().replaySince(
                    connection,
                    normalizedTopic,
                    *command.since_seq,
                    command.replay_limit,
                    replayResult,
                    error)) {
                sendSystemError(connection, {drogon::k400BadRequest, error}, normalizedTopic);
                return;
            }

            sendReplayResult(connection, command.topic, *command.since_seq, replayResult);
            return;
        }

        case pm::ws::ClientAction::Unsubscribe:
            pm::ws::WsHub::instance().unsubscribe(connection, command.topic);
            sendSystemEvent(connection, "system.unsubscribed", subscriptionsPayload(connection), command.topic);
            return;
    }
}

void MarketWsController::handleConnectionClosed(const drogon::WebSocketConnectionPtr &connection) {
    pm::ws::WsHub::instance().unregisterConnection(connection);
}
