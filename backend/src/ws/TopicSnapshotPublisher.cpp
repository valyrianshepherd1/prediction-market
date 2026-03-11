#include "pm/ws/TopicSnapshotPublisher.h"

#include "pm/repositories/OrderRepository.h"
#include "pm/repositories/PortfolioRepository.h"
#include "pm/repositories/TradeRepository.h"
#include "pm/repositories/WalletRepository.h"
#include "pm/ws/WsEnvelope.h"
#include "pm/ws/WsHub.h"

#include <json/json.h>

#include <algorithm>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {
    constexpr int kDefaultOrderbookDepth = 50;
    constexpr int kDefaultTradesLimit = 50;
    constexpr int kDefaultUserPositionsLimit = 100;
    constexpr int kDefaultUserOrdersLimit = 100;
    constexpr int kMaxSnapshotPageSize = 200;

    void sendEvent(const drogon::WebSocketConnectionPtr &connection,
                   const std::string &event,
                   const std::string &topic,
                   const Json::Value &payload,
                   bool snapshot = false) {
        if (!connection || !connection->connected()) {
            return;
        }

        connection->send(pm::ws::stringifyJson(
            pm::ws::makeEnvelope(
                event,
                payload,
                topic,
                pm::ws::makeMeta(pm::ws::WsHub::instance().currentTopicSeq(topic), snapshot))));
    }

    void sendSystemError(const drogon::WebSocketConnectionPtr &connection,
                         const std::string &topic,
                         const std::string &message) {
        Json::Value payload;
        payload["message"] = message;
        if (!connection || !connection->connected()) {
            return;
        }
        connection->send(pm::ws::stringifyJson(
            pm::ws::makeEnvelope("system.error", payload, topic, pm::ws::makeMeta())));
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

    Json::Value orderToJson(const OrderRow &o) {
        Json::Value j;
        j["id"] = o.id;
        j["user_id"] = o.user_id;
        j["outcome_id"] = o.outcome_id;
        j["side"] = o.side;
        j["price_bp"] = o.price_bp;
        j["qty_total_micros"] = Json::Int64(o.qty_total_micros);
        j["qty_remaining_micros"] = Json::Int64(o.qty_remaining_micros);
        j["status"] = o.status;
        j["created_at"] = o.created_at;
        j["updated_at"] = o.updated_at;
        return j;
    }

    Json::Value tradeToJson(const TradeRow &t) {
        Json::Value j;
        j["id"] = t.id;
        j["outcome_id"] = t.outcome_id;
        j["maker_user_id"] = t.maker_user_id;
        j["taker_user_id"] = t.taker_user_id;
        j["maker_order_id"] = t.maker_order_id;
        j["taker_order_id"] = t.taker_order_id;
        j["price_bp"] = t.price_bp;
        j["qty_micros"] = Json::Int64(t.qty_micros);
        j["created_at"] = t.created_at;
        return j;
    }

    Json::Value walletToJson(const WalletRow &w) {
        Json::Value j;
        j["user_id"] = w.user_id;
        j["available"] = Json::Int64(w.available);
        j["reserved"] = Json::Int64(w.reserved);
        j["updated_at"] = w.updated_at;
        return j;
    }

    Json::Value positionToJson(const PortfolioPositionRow &p) {
        Json::Value j;
        j["user_id"] = p.user_id;
        j["outcome_id"] = p.outcome_id;
        j["market_id"] = p.market_id;
        j["market_question"] = p.market_question;
        j["outcome_title"] = p.outcome_title;
        j["outcome_index"] = p.outcome_index;
        j["shares_available"] = Json::Int64(p.shares_available);
        j["shares_reserved"] = Json::Int64(p.shares_reserved);
        j["shares_total"] = Json::Int64(p.shares_available + p.shares_reserved);
        j["updated_at"] = p.updated_at;
        return j;
    }

    std::string suffixAfterPrefix(const std::string &topic,
                                  std::string_view prefix) {
        if (topic.rfind(prefix, 0) != 0 || topic.size() == prefix.size()) {
            return {};
        }
        return topic.substr(prefix.size());
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
                Json::Value payload;
                payload["depth"] = depth;
                payload["buy"] = Json::arrayValue;
                payload["sell"] = Json::arrayValue;
                for (const auto &row: book.buy) {
                    payload["buy"].append(orderToJson(row));
                }
                for (const auto &row: book.sell) {
                    payload["sell"].append(orderToJson(row));
                }
                sendEvent(connection, "orderbook.snapshot", topic, payload, true);
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
                Json::Value payload;
                payload["limit"] = limit;
                payload["offset"] = offset;
                payload["items"] = Json::arrayValue;
                for (const auto &row: rows) {
                    payload["items"].append(tradeToJson(row));
                }
                sendEvent(connection, "trades.snapshot", topic, payload, true);
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
                                Json::Value payload;
                                payload["user_id"] = userId;
                                payload["wallet"] = wallet ? walletToJson(*wallet) : Json::Value(Json::nullValue);

                                payload["positions"] = Json::arrayValue;
                                for (const auto &row: positions) {
                                    payload["positions"].append(positionToJson(row));
                                }
                                payload["positions_limit"] = positionsLimit;
                                payload["positions_offset"] = positionsOffset;

                                payload["open_orders"] = Json::arrayValue;
                                for (const auto &row: orders) {
                                    payload["open_orders"].append(orderToJson(row));
                                }
                                payload["open_orders_limit"] = ordersLimit;
                                payload["open_orders_offset"] = ordersOffset;

                                sendEvent(connection, "user.snapshot", topic, payload, true);
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
