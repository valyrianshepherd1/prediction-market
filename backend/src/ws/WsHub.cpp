#include "pm/ws/WsHub.h"

#include <algorithm>
#include <cctype>
#include <utility>

#include "pm/util/JsonSerializers.h"
#include "pm/ws/WsEnvelope.h"

namespace pm::ws {
    namespace {
        const void *makeKey(const drogon::WebSocketConnectionPtr &connection) {
            return connection.get();
        }

        std::string trim(std::string value) {
            auto notSpace = [](unsigned char ch) { return !std::isspace(ch); };
            value.erase(value.begin(), std::find_if(value.begin(), value.end(), notSpace));
            value.erase(std::find_if(value.rbegin(), value.rend(), notSpace).base(), value.end());
            return value;
        }
    } // namespace

    WsHub &WsHub::instance() {
        static WsHub hub;
        return hub;
    }

    void WsHub::registerConnection(const drogon::WebSocketConnectionPtr &connection) {
        std::scoped_lock lock(mutex_);
        connections_[makeKey(connection)] = ConnectionState{connection, std::nullopt, {}};
    }

    void WsHub::unregisterConnection(const drogon::WebSocketConnectionPtr &connection) {
        std::scoped_lock lock(mutex_);
        forgetConnectionLocked(makeKey(connection));
    }

    void WsHub::setPrincipal(const drogon::WebSocketConnectionPtr &connection,
                             pm::auth::Principal principal) {
        std::scoped_lock lock(mutex_);
        const auto key = makeKey(connection);
        auto &state = connections_[key];
        state.connection = connection;
        state.principal = std::move(principal);
    }

    bool WsHub::subscribe(const drogon::WebSocketConnectionPtr &connection,
                          const std::string &topic,
                          std::string &error) {
        std::scoped_lock lock(mutex_);

        const auto key = makeKey(connection);
        auto it = connections_.find(key);
        if (it == connections_.end()) {
            error = "connection is not registered";
            return false;
        }

        auto normalizedTopic = normalizeTopic(topic, it->second.principal);
        if (!isAllowedTopic(normalizedTopic, it->second.principal, error)) {
            return false;
        }

        it->second.topics.insert(normalizedTopic);
        topic_index_[normalizedTopic].insert(key);
        return true;
    }

    void WsHub::unsubscribe(const drogon::WebSocketConnectionPtr &connection,
                            const std::string &topic) {
        std::scoped_lock lock(mutex_);

        const auto key = makeKey(connection);
        auto it = connections_.find(key);
        if (it == connections_.end()) {
            return;
        }

        const auto normalizedTopic = normalizeTopic(topic, it->second.principal);
        it->second.topics.erase(normalizedTopic);

        auto topicIt = topic_index_.find(normalizedTopic);
        if (topicIt == topic_index_.end()) {
            return;
        }

        topicIt->second.erase(key);
        if (topicIt->second.empty()) {
            topic_index_.erase(topicIt);
        }
    }

    std::vector<std::string> WsHub::subscriptions(const drogon::WebSocketConnectionPtr &connection) const {
        std::scoped_lock lock(mutex_);

        const auto it = connections_.find(makeKey(connection));
        if (it == connections_.end()) {
            return {};
        }

        return {it->second.topics.begin(), it->second.topics.end()};
    }

    bool WsHub::resolveTopic(const drogon::WebSocketConnectionPtr &connection,
                             const std::string &topic,
                             std::string &normalizedTopic,
                             std::string &error) const {
        std::scoped_lock lock(mutex_);

        const auto it = connections_.find(makeKey(connection));
        if (it == connections_.end()) {
            error = "connection is not registered";
            normalizedTopic.clear();
            return false;
        }

        normalizedTopic = normalizeTopic(topic, it->second.principal);
        return isAllowedTopic(normalizedTopic, it->second.principal, error);
    }

    std::uint64_t WsHub::currentTopicSeq(const std::string &topic) const {
        std::scoped_lock lock(mutex_);

        const auto normalizedTopic = normalizeTopic(topic, std::nullopt);
        const auto it = topic_seq_.find(normalizedTopic);
        if (it == topic_seq_.end()) {
            return 0;
        }
        return it->second;
    }

    bool WsHub::replaySince(const drogon::WebSocketConnectionPtr &connection,
                            const std::string &topic,
                            std::uint64_t sinceSeq,
                            std::size_t maxEvents,
                            ReplayResult &result,
                            std::string &error) const {
        if (!connection) {
            error = "connection is not available";
            return false;
        }

        std::vector<std::string> messages;
        messages.reserve(std::min(maxEvents, kMaxReplayEvents));

        {
            std::scoped_lock lock(mutex_);

            const auto connIt = connections_.find(makeKey(connection));
            if (connIt == connections_.end()) {
                error = "connection is not registered";
                return false;
            }

            const auto normalizedTopic = normalizeTopic(topic, connIt->second.principal);
            if (!isAllowedTopic(normalizedTopic, connIt->second.principal, error)) {
                return false;
            }

            if (connIt->second.topics.find(normalizedTopic) == connIt->second.topics.end()) {
                error = "replay requires active subscription to the topic";
                return false;
            }

            const auto seqIt = topic_seq_.find(normalizedTopic);
            if (seqIt == topic_seq_.end()) {
                result.current_seq = 0;
                return true;
            }

            result.current_seq = seqIt->second;
            const auto historyIt = topic_history_.find(normalizedTopic);
            if (historyIt == topic_history_.end() || historyIt->second.empty()) {
                return true;
            }

            const auto &history = historyIt->second;
            if (history.front().seq > sinceSeq + 1 && result.current_seq > sinceSeq) {
                result.gap_detected = true;
            }

            const auto replayLimit = std::min(maxEvents, kMaxReplayEvents);
            for (const auto &entry : history) {
                if (entry.seq <= sinceSeq) {
                    continue;
                }

                if (messages.size() >= replayLimit) {
                    result.truncated = true;
                    break;
                }

                messages.push_back(entry.json);
                result.last_replayed_seq = entry.seq;
            }

            result.replayed_count = messages.size();
        }

        for (const auto &json : messages) {
            if (!connection->connected()) {
                break;
            }
            connection->send(json);
        }

        return true;
    }

    void WsHub::publish(const std::string &topic,
                        const std::string &event,
                        const Json::Value &payload) {
        const auto normalizedTopic = normalizeTopic(topic, std::nullopt);
        if (normalizedTopic.empty()) {
            return;
        }

        std::vector<drogon::WebSocketConnectionPtr> recipients;
        std::vector<ConnectionKey> staleKeys;
        std::string json;

        {
            std::scoped_lock lock(mutex_);
            auto topicIt = topic_index_.find(normalizedTopic);
            if (topicIt == topic_index_.end()) {
                return;
            }

            auto &seq = topic_seq_[normalizedTopic];
            ++seq;

            const auto envelope = pm::ws::makeEnvelope(
                event,
                payload,
                normalizedTopic,
                pm::ws::makeMeta(seq, false));
            json = pm::json::stringify(envelope);

            auto &history = topic_history_[normalizedTopic];
            history.push_back(BufferedEvent{seq, json});
            while (history.size() > kTopicHistorySize) {
                history.pop_front();
            }

            for (auto it = topicIt->second.begin(); it != topicIt->second.end();) {
                auto connIt = connections_.find(*it);
                if (connIt == connections_.end()) {
                    staleKeys.push_back(*it);
                    it = topicIt->second.erase(it);
                    continue;
                }

                if (auto connection = connIt->second.connection.lock()) {
                    recipients.push_back(std::move(connection));
                    ++it;
                } else {
                    staleKeys.push_back(*it);
                    it = topicIt->second.erase(it);
                }
            }

            if (topicIt->second.empty()) {
                topic_index_.erase(topicIt);
            }

            for (const auto key: staleKeys) {
                forgetConnectionLocked(key);
            }
        }

        if (recipients.empty()) {
            return;
        }

        for (const auto &connection: recipients) {
            if (connection && connection->connected()) {
                connection->send(json);
            }
        }
    }

    std::string WsHub::normalizeTopic(const std::string &topic,
                                      const std::optional<pm::auth::Principal> &principal) {
        auto normalized = trim(topic);
        if (normalized.empty()) {
            return normalized;
        }

        constexpr std::string_view userPrefix = "user:";
        if (normalized.rfind(userPrefix, 0) == 0 && normalized.substr(userPrefix.size()) == "me") {
            if (principal) {
                normalized = std::string(userPrefix) + principal->user_id;
            }
        }

        return normalized;
    }

    bool WsHub::isAllowedTopic(const std::string &topic,
                               const std::optional<pm::auth::Principal> &principal,
                               std::string &error) {
        if (topic.empty()) {
            error = "topic must not be empty";
            return false;
        }

        if (topic == "markets") {
            return true;
        }

        constexpr std::string_view marketPrefix = "market:";
        if (topic.rfind(marketPrefix, 0) == 0) {
            if (topic.size() == marketPrefix.size()) {
                error = "market topic must contain a market id";
                return false;
            }
            return true;
        }

        constexpr std::string_view orderbookPrefix = "orderbook:";
        if (topic.rfind(orderbookPrefix, 0) == 0) {
            if (topic.size() == orderbookPrefix.size()) {
                error = "orderbook topic must contain an outcome id";
                return false;
            }
            return true;
        }

        constexpr std::string_view tradesPrefix = "trades:";
        if (topic.rfind(tradesPrefix, 0) == 0) {
            if (topic.size() == tradesPrefix.size()) {
                error = "trades topic must contain an outcome id";
                return false;
            }
            return true;
        }

        constexpr std::string_view userPrefix = "user:";
        if (topic.rfind(userPrefix, 0) == 0) {
            if (!principal) {
                error = "user:* topics require Authorization: Bearer <access_token> during websocket handshake";
                return false;
            }

            const auto requestedUserId = topic.substr(userPrefix.size());
            if (requestedUserId.empty()) {
                error = "user topic must contain a user id";
                return false;
            }

            if (requestedUserId != principal->user_id) {
                error = "you can subscribe only to your own user topic";
                return false;
            }

            return true;
        }

        error = "unknown topic; expected markets, market:<market_id>, orderbook:<outcome_id>, trades:<outcome_id>, user:<user_id|me>";
        return false;
    }

    void WsHub::forgetConnectionLocked(ConnectionKey key) {
        auto connIt = connections_.find(key);
        if (connIt == connections_.end()) {
            return;
        }

        for (const auto &topic: connIt->second.topics) {
            auto topicIt = topic_index_.find(topic);
            if (topicIt == topic_index_.end()) {
                continue;
            }

            topicIt->second.erase(key);
            if (topicIt->second.empty()) {
                topic_index_.erase(topicIt);
            }
        }

        connections_.erase(connIt);
    }
} // namespace pm::ws
