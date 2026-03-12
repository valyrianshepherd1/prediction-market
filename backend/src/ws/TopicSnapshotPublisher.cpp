#include "pm/ws/TopicSnapshotPublisher.h"

#include "pm/repositories/MarketRepository.h"
#include "pm/repositories/OrderRepository.h"
#include "pm/repositories/PortfolioRepository.h"
#include "pm/repositories/TradeRepository.h"
#include "pm/repositories/WalletRepository.h"
#include "pm/util/JsonSerializers.h"
#include "pm/ws/WsEnvelope.h"
#include "pm/ws/WsHub.h"

#include <json/json.h>

#include <algorithm>
#include <string>
#include <string_view>
#include <utility>

namespace {
    constexpr int kDefaultMarketsLimit = 50;
    constexpr int kDefaultOrderbookDepth = 50;
    constexpr int kDefaultTradesLimit = 50;
    constexpr int kDefaultUserPositionsLimit = 100;
    constexpr int kDefaultUserOrdersLimit = 100;
    constexpr int kMaxSnapshotPageSize = 200;

    void sendSnapshotEvent(const drogon::WebSocketConnectionPtr &connection,
                           const std::string &event,
                           const std::string &topic,
                           const Json::Value &payload) {
        if (!connection || !connection->connected()) {
            return;
        }

        const auto envelope = pm::ws::makeEnvelope(
            event,
            payload,
            topic,
            pm::ws::makeMeta(pm::ws::WsHub::instance().currentTopicSeq(topic), true));
        connection->send(pm::json::stringify(envelope));
    }

    void sendSystemError(const drogon::WebSocketConnectionPtr &connection,
                         const std::string &topic,
                         const std::string &message) {
        if (!connection || !connection->connected()) {
            return;
        }

        Json::Value payload(Json::objectValue);
        payload["message"] = message;
        const auto envelope = pm::ws::makeEnvelope(
            "system.error",
            payload,
            topic,
            pm::ws::makeMeta());
        connection->send(pm::json::stringify(envelope));
    }

    int clampLimit(const Json::Value &options,
                   const char *name,
                   int fallback) {
        int value = options.get(name, fallback).asInt();
        if (value <= 0) {
            value = fallback;
        }
        return std::min(value, kMaxSnapshotPageSize);
    }

    int clampOffset(const Json::Value &options,
                    const char *name,
                    int fallback = 0) {
        int value = options.get(name, fallback).asInt();
        return std::max(value, 0);
    }

    std::string suffixAfterPrefix(const std::string &topic,
                                  std::string_view prefix) {
        if (topic.rfind(prefix, 0) != 0 || topic.size() == prefix.size()) {
            return {};
        }
        return topic.substr(prefix.size());
    }

    void sendMarketsSnapshot(const drogon::WebSocketConnectionPtr &connection,
                             const drogon::orm::DbClientPtr &db,
                             const std::string &topic,
                             const Json::Value &options) {
        const int limit = clampLimit(options, "limit", kDefaultMarketsLimit);
        const int offset = clampOffset(options, "offset");

        std::optional<std::string> status;
        if (options.isMember("status") && options["status"].isString()) {
            const auto statusValue = options["status"].asString();
            if (!statusValue.empty()) {
                status = statusValue;
            }
        }

        MarketRepository{db}.listMarkets(
            status,
            limit,
            offset,
            [connection, topic, limit, offset, status](std::vector<MarketRow> rows) {
                Json::Value payload(Json::objectValue);
                payload["limit"] = limit;
                payload["offset"] = offset;
                payload["status"] = status ? Json::Value(*status) : Json::Value(Json::nullValue);
                payload["items"] = Json::arrayValue;
                for (const auto &row : rows) {
                    payload["items"].append(pm::json::toJson(row));
                }
                sendSnapshotEvent(connection, "markets.snapshot", topic, payload);
            },
            [connection, topic](const drogon::orm::DrogonDbException &e) {
                sendSystemError(connection, topic, e.base().what());
            });
    }

    void sendMarketSnapshot(const drogon::WebSocketConnectionPtr &connection,
                            const drogon::orm::DbClientPtr &db,
                            const std::string &topic,
                            const Json::Value &) {
        const auto marketId = suffixAfterPrefix(topic, "market:");
        if (marketId.empty()) {
            sendSystemError(connection, topic, "market snapshot requires market:<market_id>");
            return;
        }

        MarketRepository{db}.getMarketById(
            marketId,
            [connection, db, topic, marketId](std::optional<MarketRow> market) {
                if (!market) {
                    sendSystemError(connection, topic, "market not found");
                    return;
                }

                MarketRepository{db}.listOutcomesByMarketId(
                    marketId,
                    [connection, topic, market = std::move(*market)](std::vector<OutcomeRow> outcomes) mutable {
                        sendSnapshotEvent(
                            connection,
                            "market.snapshot",
                            topic,
                            pm::json::toJsonWithOutcomes(market, outcomes));
                    },
                    [connection, topic](const drogon::orm::DrogonDbException &e) {
                        sendSystemError(connection, topic, e.base().what());
                    });
            },
            [connection, topic](const drogon::orm::DrogonDbException &e) {
                sendSystemError(connection, topic, e.base().what());
            });
    }

    void sendOrderbookSnapshot(const drogon::WebSocketConnectionPtr &connection,
                               const drogon::orm::DbClientPtr &db,
                               const std::string &topic,
                               const Json::Value &options) {
        const auto outcomeId = suffixAfterPrefix(topic, "orderbook:");
        if (outcomeId.empty()) {
            sendSystemError(connection, topic, "orderbook snapshot requires orderbook:<outcome_id>");
            return;
        }

        const int depth = clampLimit(options, "depth", kDefaultOrderbookDepth);
        OrderRepository{db}.getOrderBook(
            outcomeId,
            depth,
            [connection, topic, depth](OrderBook book) {
                Json::Value payload = pm::json::toJson(book);
                payload["depth"] = depth;
                sendSnapshotEvent(connection, "orderbook.snapshot", topic, payload);
            },
            [connection, topic](const drogon::orm::DrogonDbException &e) {
                sendSystemError(connection, topic, e.base().what());
            });
    }

    void sendTradesSnapshot(const drogon::WebSocketConnectionPtr &connection,
                            const drogon::orm::DbClientPtr &db,
                            const std::string &topic,
                            const Json::Value &options) {
        const auto outcomeId = suffixAfterPrefix(topic, "trades:");
        if (outcomeId.empty()) {
            sendSystemError(connection, topic, "trades snapshot requires trades:<outcome_id>");
            return;
        }

        const int limit = clampLimit(options, "limit", kDefaultTradesLimit);
        const int offset = clampOffset(options, "offset");
        TradeRepository{db}.listByOutcome(
            outcomeId,
            limit,
            offset,
            [connection, topic, limit, offset](std::vector<TradeRow> rows) {
                Json::Value payload(Json::objectValue);
                payload["limit"] = limit;
                payload["offset"] = offset;
                payload["items"] = Json::arrayValue;
                for (const auto &row : rows) {
                    payload["items"].append(pm::json::toJson(row));
                }
                sendSnapshotEvent(connection, "trades.snapshot", topic, payload);
            },
            [connection, topic](const drogon::orm::DrogonDbException &e) {
                sendSystemError(connection, topic, e.base().what());
            });
    }

    void sendUserSnapshot(const drogon::WebSocketConnectionPtr &connection,
                          const drogon::orm::DbClientPtr &db,
                          const std::string &topic,
                          const Json::Value &options) {
        const auto userId = suffixAfterPrefix(topic, "user:");
        if (userId.empty()) {
            sendSystemError(connection, topic, "user snapshot requires user:<user_id>");
            return;
        }

        const int positionsLimit = clampLimit(options, "positions_limit", kDefaultUserPositionsLimit);
        const int positionsOffset = clampOffset(options, "positions_offset");
        const int ordersLimit = clampLimit(options, "orders_limit", kDefaultUserOrdersLimit);
        const int ordersOffset = clampOffset(options, "orders_offset");

        WalletRepository{db}.getWalletByUserId(
            userId,
            [connection, db, topic, userId, positionsLimit, positionsOffset, ordersLimit, ordersOffset]
            (std::optional<WalletRow> wallet) {
                PortfolioRepository{db}.listPositionsForUser(
                    userId,
                    positionsLimit,
                    positionsOffset,
                    [connection, db, topic, userId, wallet, positionsLimit, positionsOffset, ordersLimit, ordersOffset]
                    (std::vector<PortfolioPositionRow> positions) mutable {
                        OrderRepository{db}.listOrdersForUser(
                            userId,
                            std::optional<std::string>{"OPEN"},
                            ordersLimit,
                            ordersOffset,
                            [connection, topic, userId, wallet = std::move(wallet), positions = std::move(positions), positionsLimit,
                             positionsOffset, ordersLimit, ordersOffset](std::vector<OrderRow> orders) mutable {
                                Json::Value payload(Json::objectValue);
                                payload["user_id"] = userId;
                                payload["wallet"] = wallet ? pm::json::toJson(*wallet) : Json::Value(Json::nullValue);

                                payload["positions"] = Json::arrayValue;
                                for (const auto &row : positions) {
                                    payload["positions"].append(pm::json::toJson(row));
                                }
                                payload["positions_limit"] = positionsLimit;
                                payload["positions_offset"] = positionsOffset;

                                payload["open_orders"] = Json::arrayValue;
                                for (const auto &row : orders) {
                                    payload["open_orders"].append(pm::json::toJson(row));
                                }
                                payload["open_orders_limit"] = ordersLimit;
                                payload["open_orders_offset"] = ordersOffset;

                                sendSnapshotEvent(connection, "user.snapshot", topic, payload);
                            },
                            [connection, topic](const drogon::orm::DrogonDbException &e) {
                                sendSystemError(connection, topic, e.base().what());
                            });
                    },
                    [connection, topic](const drogon::orm::DrogonDbException &e) {
                        sendSystemError(connection, topic, e.base().what());
                    });
            },
            [connection, topic](const drogon::orm::DrogonDbException &e) {
                sendSystemError(connection, topic, e.base().what());
            });
    }
} // namespace

namespace pm::ws {
    void sendTopicSnapshot(const drogon::WebSocketConnectionPtr &connection,
                           const drogon::orm::DbClientPtr &db,
                           const std::string &topic,
                           const Json::Value &options) {
        if (!connection || !db) {
            return;
        }

        if (topic == "markets") {
            sendMarketsSnapshot(connection, db, topic, options);
            return;
        }

        constexpr std::string_view marketPrefix = "market:";
        if (topic.rfind(marketPrefix, 0) == 0) {
            sendMarketSnapshot(connection, db, topic, options);
            return;
        }

        constexpr std::string_view orderbookPrefix = "orderbook:";
        if (topic.rfind(orderbookPrefix, 0) == 0) {
            sendOrderbookSnapshot(connection, db, topic, options);
            return;
        }

        constexpr std::string_view tradesPrefix = "trades:";
        if (topic.rfind(tradesPrefix, 0) == 0) {
            sendTradesSnapshot(connection, db, topic, options);
            return;
        }

        constexpr std::string_view userPrefix = "user:";
        if (topic.rfind(userPrefix, 0) == 0) {
            sendUserSnapshot(connection, db, topic, options);
            return;
        }

        sendSystemError(connection, topic, "snapshot is not supported for this topic");
    }
} // namespace pm::ws
