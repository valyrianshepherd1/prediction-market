#pragma once

#include "pm/util/AuthGuard.h"

#include <drogon/WebSocketController.h>

#include <json/json.h>

#include <cstddef>
#include <cstdint>
#include <deque>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace pm::ws {
    class WsHub {
    public:
        struct ReplayResult {
            std::uint64_t current_seq{0};
            std::uint64_t last_replayed_seq{0};
            std::size_t replayed_count{0};
            bool gap_detected{false};
            bool truncated{false};
        };

        static WsHub &instance();

        void registerConnection(const drogon::WebSocketConnectionPtr &connection);
        void unregisterConnection(const drogon::WebSocketConnectionPtr &connection);
        void setPrincipal(const drogon::WebSocketConnectionPtr &connection,
                          pm::auth::Principal principal);

        bool subscribe(const drogon::WebSocketConnectionPtr &connection,
                       const std::string &topic,
                       std::string &error);

        void unsubscribe(const drogon::WebSocketConnectionPtr &connection,
                         const std::string &topic);

        std::vector<std::string> subscriptions(const drogon::WebSocketConnectionPtr &connection) const;

        bool resolveTopic(const drogon::WebSocketConnectionPtr &connection,
                          const std::string &topic,
                          std::string &normalizedTopic,
                          std::string &error) const;

        [[nodiscard]] std::uint64_t currentTopicSeq(const std::string &topic) const;

        bool replaySince(const drogon::WebSocketConnectionPtr &connection,
                         const std::string &topic,
                         std::uint64_t sinceSeq,
                         std::size_t maxEvents,
                         ReplayResult &result,
                         std::string &error) const;

        void publish(const std::string &topic,
                     const std::string &event,
                     const Json::Value &payload);

    private:
        struct ConnectionState {
            std::weak_ptr<drogon::WebSocketConnection> connection;
            std::optional<pm::auth::Principal> principal;
            std::unordered_set<std::string> topics;
        };

        struct BufferedEvent {
            std::uint64_t seq{0};
            std::string json;
        };

        using ConnectionKey = const void *;

        static constexpr std::size_t kTopicHistorySize = 256;
        static constexpr std::size_t kMaxReplayEvents = 256;

        static std::string normalizeTopic(const std::string &topic,
                                          const std::optional<pm::auth::Principal> &principal);

        static bool isAllowedTopic(const std::string &topic,
                                   const std::optional<pm::auth::Principal> &principal,
                                   std::string &error);

        void forgetConnectionLocked(ConnectionKey key);

        mutable std::mutex mutex_;
        std::unordered_map<ConnectionKey, ConnectionState> connections_;
        std::unordered_map<std::string, std::unordered_set<ConnectionKey>> topic_index_;
        std::unordered_map<std::string, std::uint64_t> topic_seq_;
        std::unordered_map<std::string, std::deque<BufferedEvent>> topic_history_;
    };
} // namespace pm::ws
