#include "pm/ws/WsCommand.h"

#include <algorithm>
#include <memory>
#include <string>
#include <utility>

namespace {
    constexpr std::size_t kDefaultReplayLimit = 100;
    constexpr std::size_t kMaxReplayLimit = 256;

    pm::ApiError badRequest(std::string message) {
        return {drogon::k400BadRequest, std::move(message)};
    }

    pm::expected<Json::Value, pm::ApiError> parseJsonObject(std::string_view message) {
        Json::CharReaderBuilder builder;
        std::string errs;
        Json::Value root;
        auto reader = std::unique_ptr<Json::CharReader>(builder.newCharReader());
        if (!reader->parse(message.data(), message.data() + message.size(), &root, &errs)) {
            return pm::unexpected<pm::ApiError>(pm::ApiError{
                drogon::k400BadRequest,
                errs.empty() ? "invalid JSON" : errs,
            });
        }

        if (!root.isObject()) {
            return pm::unexpected<pm::ApiError>(badRequest("message must be a JSON object"));
        }

        return root;
    }

    pm::expected<std::optional<std::uint64_t>, pm::ApiError> parseSinceSeq(const Json::Value &root) {
        if (!root.isMember("since_seq")) {
            return std::optional<std::uint64_t>{};
        }

        const auto &field = root["since_seq"];
        if (field.isUInt64()) {
            return std::optional<std::uint64_t>{field.asUInt64()};
        }

        if (field.isInt64()) {
            const auto value = field.asInt64();
            if (value < 0) {
                return pm::unexpected<pm::ApiError>(badRequest("since_seq must be >= 0"));
            }
            return std::optional<std::uint64_t>{static_cast<std::uint64_t>(value)};
        }

        if (field.isIntegral()) {
            return std::optional<std::uint64_t>{static_cast<std::uint64_t>(field.asLargestUInt())};
        }

        return pm::unexpected<pm::ApiError>(badRequest("since_seq must be an integer"));
    }

    pm::expected<bool, pm::ApiError> parseWithSnapshot(const Json::Value &root) {
        if (!root.isMember("with_snapshot")) {
            return false;
        }
        if (!root["with_snapshot"].isBool()) {
            return pm::unexpected<pm::ApiError>(badRequest("with_snapshot must be a boolean"));
        }
        return root["with_snapshot"].asBool();
    }

    pm::expected<std::size_t, pm::ApiError> parseReplayLimit(const Json::Value &root) {
        if (!root.isMember("replay_limit")) {
            return kDefaultReplayLimit;
        }

        const auto &field = root["replay_limit"];
        if (!field.isIntegral()) {
            return pm::unexpected<pm::ApiError>(badRequest("replay_limit must be an integer"));
        }

        const auto value = field.asInt64();
        if (value <= 0) {
            return pm::unexpected<pm::ApiError>(badRequest("replay_limit must be > 0"));
        }

        return std::min<std::size_t>(static_cast<std::size_t>(value), kMaxReplayLimit);
    }

    pm::expected<pm::ws::ClientAction, pm::ApiError> parseAction(const Json::Value &root) {
        const auto action = root.get("action", "").asString();
        if (action == "subscribe") {
            return pm::ws::ClientAction::Subscribe;
        }
        if (action == "unsubscribe") {
            return pm::ws::ClientAction::Unsubscribe;
        }
        if (action == "list_topics") {
            return pm::ws::ClientAction::ListTopics;
        }
        if (action == "ping") {
            return pm::ws::ClientAction::Ping;
        }
        if (action == "snapshot") {
            return pm::ws::ClientAction::Snapshot;
        }
        if (action == "replay") {
            return pm::ws::ClientAction::Replay;
        }
        if (action == "describe") {
            return pm::ws::ClientAction::Describe;
        }
        if (action.empty()) {
            return pm::unexpected<pm::ApiError>(badRequest("action is required"));
        }
        return pm::unexpected<pm::ApiError>(badRequest("unknown action"));
    }
} // namespace

namespace pm::ws {
    const char *toString(ClientAction action) noexcept {
        switch (action) {
            case ClientAction::Subscribe:
                return "subscribe";
            case ClientAction::Unsubscribe:
                return "unsubscribe";
            case ClientAction::ListTopics:
                return "list_topics";
            case ClientAction::Ping:
                return "ping";
            case ClientAction::Snapshot:
                return "snapshot";
            case ClientAction::Replay:
                return "replay";
            case ClientAction::Describe:
                return "describe";
        }
        return "unknown";
    }

    bool actionRequiresTopic(ClientAction action) noexcept {
        switch (action) {
            case ClientAction::Subscribe:
            case ClientAction::Unsubscribe:
            case ClientAction::Snapshot:
            case ClientAction::Replay:
                return true;
            case ClientAction::Describe:
            case ClientAction::ListTopics:
            case ClientAction::Ping:
                return false;
        }
        return false;
    }

    pm::expected<ClientCommand, pm::ApiError>
    parseClientCommand(std::string_view message,
                       std::size_t maxMessageSize) {
        if (message.size() > maxMessageSize) {
            return pm::unexpected<pm::ApiError>(pm::ApiError{
                drogon::k400BadRequest,
                "message too large",
            });
        }

        auto root = parseJsonObject(message);
        if (!root) {
            return pm::unexpected<pm::ApiError>(root.error());
        }

        auto action = parseAction(root.value());
        if (!action) {
            return pm::unexpected<pm::ApiError>(action.error());
        }

        auto withSnapshot = parseWithSnapshot(root.value());
        if (!withSnapshot) {
            return pm::unexpected<pm::ApiError>(withSnapshot.error());
        }

        auto sinceSeq = parseSinceSeq(root.value());
        if (!sinceSeq) {
            return pm::unexpected<pm::ApiError>(sinceSeq.error());
        }

        auto replayLimit = parseReplayLimit(root.value());
        if (!replayLimit) {
            return pm::unexpected<pm::ApiError>(replayLimit.error());
        }

        ClientCommand command;
        command.action = action.value();
        command.root = root.value();
        command.topic = root.value().get("topic", "").asString();
        command.with_snapshot = withSnapshot.value();
        command.since_seq = sinceSeq.value();
        command.replay_limit = replayLimit.value();

        if (actionRequiresTopic(command.action) && command.topic.empty()) {
            return pm::unexpected<pm::ApiError>(badRequest("topic is required"));
        }

        if (command.action == ClientAction::Replay && !command.since_seq.has_value()) {
            return pm::unexpected<pm::ApiError>(badRequest("since_seq is required for replay"));
        }

        return command;
    }
} // namespace pm::ws
