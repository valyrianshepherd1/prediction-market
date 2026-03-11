#pragma once

#include "pm/repositories/AuthRepository.h"
#include "pm/repositories/MarketRepository.h"
#include "pm/repositories/OrderRepository.h"
#include "pm/repositories/PortfolioRepository.h"
#include "pm/repositories/TradeRepository.h"
#include "pm/repositories/WalletRepository.h"
#include "pm/util/AuthGuard.h"

#include <json/json.h>

#include <string>
#include <string_view>
#include <vector>

namespace pm::json {

    [[nodiscard]] Json::Value toJson(const AuthUserRow &user);
    [[nodiscard]] Json::Value toJson(const pm::auth::Principal &principal);
    [[nodiscard]] Json::Value toJson(const AuthSessionRow &session);

    [[nodiscard]] Json::Value toJson(const MarketRow &market);
    [[nodiscard]] Json::Value toJson(const OutcomeRow &outcome);
    [[nodiscard]] Json::Value toJsonWithOutcomes(const MarketRow &market,
                                                 const std::vector<OutcomeRow> &outcomes);

    [[nodiscard]] Json::Value toJson(const OrderRow &order);
    [[nodiscard]] Json::Value toJson(const OrderBook &book);

    [[nodiscard]] Json::Value toJson(const TradeRow &trade);
    [[nodiscard]] Json::Value toJson(const WalletRow &wallet);
    [[nodiscard]] Json::Value toJson(const PortfolioPositionRow &position);
    [[nodiscard]] Json::Value toJson(const PortfolioLedgerEntryRow &entry);

    [[nodiscard]] Json::Value makeWsEnvelope(std::string_view event,
                                             std::string_view topic,
                                             const Json::Value &payload);
    [[nodiscard]] std::string stringify(const Json::Value &value);

} // namespace pm::json
