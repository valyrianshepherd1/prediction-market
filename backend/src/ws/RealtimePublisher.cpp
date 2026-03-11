#include "pm/ws/RealtimePublisher.h"

#include "pm/repositories/WalletRepository.h"
#include "pm/ws/WsHub.h"

#include <drogon/orm/DbClient.h>

#include <json/json.h>

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

using drogon::orm::DbClientPtr;
using drogon::orm::DrogonDbException;
using drogon::orm::Result;

namespace {
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

OrderRow rowToOrder(const Result &r, std::size_t i) {
    OrderRow o;
    o.id = r[i]["id"].as<std::string>();
    o.user_id = r[i]["user_id"].as<std::string>();
    o.outcome_id = r[i]["outcome_id"].as<std::string>();
    o.side = r[i]["side"].as<std::string>();
    o.price_bp = r[i]["price_bp"].as<int>();
    o.qty_total_micros = r[i]["qty_total_micros"].as<std::int64_t>();
    o.qty_remaining_micros = r[i]["qty_remaining_micros"].as<std::int64_t>();
    o.status = r[i]["status"].as<std::string>();
    o.created_at = r[i]["created_at"].as<std::string>();
    o.updated_at = r[i]["updated_at"].as<std::string>();
    return o;
}

void publishOrderbookInvalidate(const OrderRow &order, std::string_view reason) {
    Json::Value payload;
    payload["reason"] = std::string(reason);
    payload["outcome_id"] = order.outcome_id;
    payload["order_id"] = order.id;

    pm::ws::WsHub::instance().publish(
        "orderbook:" + order.outcome_id,
        "orderbook.invalidate",
        payload);
}

void publishOrderChanged(const OrderRow &order, std::string_view reason) {
    Json::Value payload = orderToJson(order);
    payload["reason"] = std::string(reason);

    pm::ws::WsHub::instance().publish(
        "user:" + order.user_id,
        "order.changed",
        payload);
}

void publishPositionSnapshot(const DbClientPtr &db,
                             const std::string &userId,
                             const std::string &outcomeId) {
    if (!db || userId.empty() || outcomeId.empty()) {
        return;
    }

    static constexpr std::string_view kSql =
        "SELECT user_id::text AS user_id, "
        "outcome_id::text AS outcome_id, "
        "shares_available, shares_reserved, updated_at::text AS updated_at "
        "FROM positions "
        "WHERE user_id = $1::uuid AND outcome_id = $2::uuid";

    db->execSqlAsync(
        std::string(kSql),
        [userId](const Result &r) {
            if (r.empty()) {
                return;
            }

            Json::Value payload;
            payload["user_id"] = r[0]["user_id"].as<std::string>();
            payload["outcome_id"] = r[0]["outcome_id"].as<std::string>();
            payload["shares_available"] = Json::Int64(r[0]["shares_available"].as<std::int64_t>());
            payload["shares_reserved"] = Json::Int64(r[0]["shares_reserved"].as<std::int64_t>());
            payload["shares_total"] = Json::Int64(
                r[0]["shares_available"].as<std::int64_t>() +
                r[0]["shares_reserved"].as<std::int64_t>());
            payload["updated_at"] = r[0]["updated_at"].as<std::string>();

            pm::ws::WsHub::instance().publish(
                "user:" + userId,
                "user.position.updated",
                payload);
        },
        [](const DrogonDbException &) {
            // best effort: realtime notification failure must not break request flow
        },
        userId,
        outcomeId);
}

void publishOrderSnapshotById(const DbClientPtr &db,
                              const std::string &orderId,
                              std::string_view reason) {
    if (!db || orderId.empty()) {
        return;
    }

    static constexpr std::string_view kSql =
        "SELECT id::text AS id, user_id::text AS user_id, outcome_id::text AS outcome_id, "
        "side, price_bp, qty_total_micros, qty_remaining_micros, status, "
        "created_at::text AS created_at, updated_at::text AS updated_at "
        "FROM orders WHERE id = $1::uuid";

    db->execSqlAsync(
        std::string(kSql),
        [reason = std::string(reason)](const Result &r) {
            if (r.empty()) {
                return;
            }
            publishOrderChanged(rowToOrder(r, 0), reason);
        },
        [](const DrogonDbException &) {
            // best effort: realtime notification failure must not break request flow
        },
        orderId);
}

void publishPrivateTradeEvent(const TradeRow &trade,
                              const std::string &userId,
                              std::string_view role,
                              const std::string &counterpartyUserId) {
    if (userId.empty()) {
        return;
    }

    Json::Value payload = tradeToJson(trade);
    payload["role"] = std::string(role);
    payload["counterparty_user_id"] = counterpartyUserId;

    pm::ws::WsHub::instance().publish(
        "user:" + userId,
        "user.trade.created",
        payload);
}
} // namespace

namespace pm::ws {
void publishWalletSnapshot(const DbClientPtr &db,
                           const std::string &userId) {
    if (!db || userId.empty()) {
        return;
    }

    WalletRepository{db}.getWalletByUserId(
        userId,
        [userId](std::optional<WalletRow> wallet) {
            if (!wallet) {
                return;
            }

            WsHub::instance().publish(
                "user:" + userId,
                "user.wallet.updated",
                walletToJson(*wallet));
        },
        [](const DrogonDbException &) {
            // best effort: realtime notification failure must not break request flow
        });
}

void publishOrderLifecycle(const DbClientPtr &db,
                           const OrderRow &order,
                           std::string_view reason) {
    publishOrderbookInvalidate(order, reason);
    publishOrderChanged(order, reason);

    if (!db) {
        return;
    }

    if (order.side == "BUY") {
        publishWalletSnapshot(db, order.user_id);
    } else if (order.side == "SELL") {
        publishPositionSnapshot(db, order.user_id, order.outcome_id);
    }
}

void publishTradeExecution(const DbClientPtr &db,
                           const TradeRow &trade) {
    WsHub::instance().publish(
        "trades:" + trade.outcome_id,
        "trade.created",
        tradeToJson(trade));

    publishPrivateTradeEvent(trade, trade.maker_user_id, "maker", trade.taker_user_id);
    if (trade.taker_user_id != trade.maker_user_id) {
        publishPrivateTradeEvent(trade, trade.taker_user_id, "taker", trade.maker_user_id);
    }

    if (!db) {
        return;
    }

    publishOrderSnapshotById(db, trade.maker_order_id, "matched");
    if (trade.taker_order_id != trade.maker_order_id) {
        publishOrderSnapshotById(db, trade.taker_order_id, "matched");
    }

    publishWalletSnapshot(db, trade.maker_user_id);
    if (trade.taker_user_id != trade.maker_user_id) {
        publishWalletSnapshot(db, trade.taker_user_id);
    }

    publishPositionSnapshot(db, trade.maker_user_id, trade.outcome_id);
    if (trade.taker_user_id != trade.maker_user_id) {
        publishPositionSnapshot(db, trade.taker_user_id, trade.outcome_id);
    }
}
} // namespace pm::ws
