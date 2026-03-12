#pragma once

#include <json/json.h>

#include <string>
#include <string_view>

namespace pm::ws {
    [[nodiscard]] constexpr std::string_view protocolVersion() noexcept {
        return "1.0";
    }

    [[nodiscard]] Json::Value makeWelcomePayload();
    [[nodiscard]] Json::Value makeDescribePayload();
    [[nodiscard]] Json::Value makeDescribePayloadForTopic(std::string_view requestedTopic,
                                                          std::string_view normalizedTopic);
} // namespace pm::ws
