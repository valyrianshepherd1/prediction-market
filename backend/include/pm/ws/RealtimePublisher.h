#pragma once

#include "pm/repositories/OrderRepository.h"
#include "pm/repositories/TradeRepository.h"

#include <drogon/orm/DbClient.h>

#include <string>
#include <string_view>

namespace pm::ws {

    void publishWalletSnapshot(const drogon::orm::DbClientPtr &db,
                               const std::string &userId);

    void publishOrderLifecycle(const drogon::orm::DbClientPtr &db,
                               const OrderRow &order,
                               std::string_view reason);

    void publishTradeExecution(const drogon::orm::DbClientPtr &db,
                               const TradeRow &trade);

} // namespace pm::ws
