#include "pm/util/JsonSerializers.h"

#include <json/writer.h>

namespace pm::json {

Json::Value toJson(const AuthUserRow &user) {
    Json::Value j;
    j["id"] = user.id;
    j["email"] = user.email;
    j["username"] = user.username;
    j["role"] = user.role;
    j["created_at"] = user.created_at;
    return j;
}

Json::Value toJson(const pm::auth::Principal &principal) {
    Json::Value j;
    j["id"] = principal.user_id;
    j["email"] = principal.email;
    j["username"] = principal.username;
    j["role"] = principal.role;
    j["created_at"] = principal.created_at;
    return j;
}

Json::Value toJson(const AuthSessionRow &session) {
    Json::Value j;
    j["session_id"] = session.session_id;
    j["user"] = toJson(session.user);
    j["token_type"] = "Bearer";
    j["access_token"] = session.access_token;
    j["access_expires_at"] = session.access_expires_at;
    j["refresh_token"] = session.refresh_token;
    j["refresh_expires_at"] = session.refresh_expires_at;
    return j;
}

Json::Value toJson(const MarketRow &market) {
    Json::Value j;
    j["id"] = market.id;
    j["question"] = market.question;
    j["status"] = market.status;
    j["resolved_outcome_id"] = market.resolved_outcome_id ? Json::Value(*market.resolved_outcome_id)
                                                            : Json::Value(Json::nullValue);
    j["created_at"] = market.created_at;
    return j;
}

Json::Value toJson(const OutcomeRow &outcome) {
    Json::Value j;
    j["id"] = outcome.id;
    j["market_id"] = outcome.market_id;
    j["title"] = outcome.title;
    j["outcome_index"] = outcome.outcome_index;
    return j;
}

Json::Value toJsonWithOutcomes(const MarketRow &market,
                               const std::vector<OutcomeRow> &outcomes) {
    Json::Value j = toJson(market);
    Json::Value arr(Json::arrayValue);
    for (const auto &outcome : outcomes) {
        arr.append(toJson(outcome));
    }
    j["outcomes"] = std::move(arr);
    return j;
}

Json::Value toJson(const OrderRow &order) {
    Json::Value j;
    j["id"] = order.id;
    j["user_id"] = order.user_id;
    j["outcome_id"] = order.outcome_id;
    j["side"] = order.side;
    j["price_bp"] = order.price_bp;
    j["qty_total_micros"] = Json::Int64(order.qty_total_micros);
    j["qty_remaining_micros"] = Json::Int64(order.qty_remaining_micros);
    j["status"] = order.status;
    j["created_at"] = order.created_at;
    j["updated_at"] = order.updated_at;
    return j;
}

Json::Value toJson(const OrderBook &book) {
    Json::Value j;
    j["buy"] = Json::arrayValue;
    j["sell"] = Json::arrayValue;
    for (const auto &order : book.buy) {
        j["buy"].append(toJson(order));
    }
    for (const auto &order : book.sell) {
        j["sell"].append(toJson(order));
    }
    return j;
}

Json::Value toJson(const TradeRow &trade) {
    Json::Value j;
    j["id"] = trade.id;
    j["outcome_id"] = trade.outcome_id;
    j["maker_user_id"] = trade.maker_user_id;
    j["taker_user_id"] = trade.taker_user_id;
    j["maker_order_id"] = trade.maker_order_id;
    j["taker_order_id"] = trade.taker_order_id;
    j["price_bp"] = trade.price_bp;
    j["qty_micros"] = Json::Int64(trade.qty_micros);
    j["created_at"] = trade.created_at;
    return j;
}

Json::Value toJson(const WalletRow &wallet) {
    Json::Value j;
    j["user_id"] = wallet.user_id;
    j["available"] = Json::Int64(wallet.available);
    j["reserved"] = Json::Int64(wallet.reserved);
    j["updated_at"] = wallet.updated_at;
    return j;
}

Json::Value toJson(const PortfolioPositionRow &position) {
    Json::Value j;
    j["user_id"] = position.user_id;
    j["outcome_id"] = position.outcome_id;
    j["market_id"] = position.market_id;
    j["market_question"] = position.market_question;
    j["outcome_title"] = position.outcome_title;
    j["outcome_index"] = position.outcome_index;
    j["shares_available"] = Json::Int64(position.shares_available);
    j["shares_reserved"] = Json::Int64(position.shares_reserved);
    j["shares_total"] = Json::Int64(position.shares_available + position.shares_reserved);
    j["updated_at"] = position.updated_at;
    return j;
}

Json::Value toJson(const PortfolioLedgerEntryRow &entry) {
    Json::Value j;
    j["id"] = entry.id;
    j["ledger_type"] = entry.ledger_type;
    j["kind"] = entry.kind;
    if (!entry.outcome_id.empty()) {
        j["outcome_id"] = entry.outcome_id;
    } else {
        j["outcome_id"] = Json::nullValue;
    }
    j["delta_available"] = Json::Int64(entry.delta_available);
    j["delta_reserved"] = Json::Int64(entry.delta_reserved);
    j["ref_type"] = entry.ref_type;
    j["ref_id"] = entry.ref_id;
    j["created_at"] = entry.created_at;
    return j;
}


std::string stringify(const Json::Value &value) {
    Json::StreamWriterBuilder builder;
    builder["indentation"] = "";
    return Json::writeString(builder, value);
}

} // namespace pm::json
