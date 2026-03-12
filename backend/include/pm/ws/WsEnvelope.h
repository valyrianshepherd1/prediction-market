#pragma once

#include <json/json.h>

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace pm::ws {

    Json::Value makeMeta(std::optional<std::uint64_t> seq = std::nullopt,
                         bool snapshot = false);

    Json::Value makeEnvelope(std::string_view event,
                             const Json::Value &payload,
                             std::string_view topic = {},
                             const Json::Value &meta = Json::Value(Json::objectValue));


} // namespace pm::ws
