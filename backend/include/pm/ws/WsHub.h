#pragma once

#include "pm/util/AuthGuard.h"

#include <drogon/WebSocketController.h>

#include <json/json.h>

#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace pm::ws {
    class WsHub {
    public:
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

        void publish(const std::string &topic,
                     const std::string &event,
                     const Json::Value &payload);

    private:
        struct ConnectionState {
            std::weak_ptr<drogon::WebSocketConnection> connection;
            std::optional<pm::auth::Principal> principal;
            std::unordered_set<std::string> topics;
        };

        using ConnectionKey = const void *;

        static std::string normalizeTopic(const std::string &topic,
                                          const std::optional<pm::auth::Principal> &principal);

        static bool isAllowedTopic(const std::string &topic,
                                   const std::optional<pm::auth::Principal> &principal,
                                   std::string &error);

        static std::string toJsonString(const Json::Value &value);

        void forgetConnectionLocked(ConnectionKey key);

        mutable std::mutex mutex_;
        std::unordered_map<ConnectionKey, ConnectionState> connections_;
        std::unordered_map<std::string, std::unordered_set<ConnectionKey>> topic_index_;
    };
} // namespace pm::ws
