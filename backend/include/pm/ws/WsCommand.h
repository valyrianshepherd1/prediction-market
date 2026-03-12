#pragma once

#include "pm/util/ApiError.h"
#include "pm/util/Expected.h"

#include <json/json.h>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace pm::ws {
    enum class ClientAction {
        Subscribe,
        Unsubscribe,
        ListTopics,
        Ping,
        Snapshot,
        Replay,
        Describe,
    };

    struct ClientCommand {
        ClientAction action{ClientAction::Ping};
        std::string topic;
        bool with_snapshot{false};
        std::optional<std::uint64_t> since_seq;
        std::size_t replay_limit{100};
        Json::Value root{Json::objectValue};
    };

    [[nodiscard]] const char *toString(ClientAction action) noexcept;
    [[nodiscard]] bool actionRequiresTopic(ClientAction action) noexcept;

    [[nodiscard]] pm::expected<ClientCommand, pm::ApiError>
    parseClientCommand(std::string_view message, std::size_t maxMessageSize);
} // namespace pm::ws
