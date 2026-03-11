#include "pm/ws/WsEnvelope.h"

#include <chrono>

namespace {
    std::uint64_t nowUnixTimeMs() {
        using namespace std::chrono;
        return static_cast<std::uint64_t>(duration_cast<milliseconds>(
            system_clock::now().time_since_epoch()).count());
    }
} // namespace

namespace pm::ws {
    Json::Value makeMeta(std::optional<std::uint64_t> seq,
                         bool snapshot) {
        Json::Value meta(Json::objectValue);
        meta["server_time_ms"] = Json::UInt64(nowUnixTimeMs());
        if (seq.has_value()) {
            meta["seq"] = Json::UInt64(*seq);
        }
        if (snapshot) {
            meta["snapshot"] = true;
        }
        return meta;
    }

    Json::Value makeEnvelope(std::string_view event,
                             const Json::Value &payload,
                             std::string_view topic,
                             const Json::Value &meta) {
        Json::Value envelope(Json::objectValue);
        envelope["event"] = std::string(event);
        if (!topic.empty()) {
            envelope["topic"] = std::string(topic);
        }
        envelope["payload"] = payload;
        envelope["meta"] = meta;
        return envelope;
    }

    std::string stringifyJson(const Json::Value &value) {
        Json::StreamWriterBuilder builder;
        builder["indentation"] = "";
        return Json::writeString(builder, value);
    }
} // namespace pm::ws
