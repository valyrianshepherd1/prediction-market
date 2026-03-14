#pragma once

#include "pm/repositories/MarketRepository.h"
#include "pm/repositories/OrderRepository.h"
#include "pm/repositories/TradeRepository.h"

#include <drogon/orm/DbClient.h>

#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace pm::ws {

    enum class RealtimePublishKind {
        WalletSnapshot,
        OrderLifecycle,
        TradeExecution,
        MarketLifecycle,
    };

    struct WalletSnapshotRequest {
        drogon::orm::DbClientPtr db;
        std::string user_id;
    };

    struct OrderLifecycleRequest {
        drogon::orm::DbClientPtr db;
        OrderRow order;
        std::string reason;
    };

    struct TradeExecutionRequest {
        drogon::orm::DbClientPtr db;
        TradeRow trade;
    };

    struct MarketLifecycleRequest {
        MarketRow market;
        std::optional<std::vector<OutcomeRow>> outcomes;
        std::string reason;
    };

    using RealtimePublishRequest = std::variant<
        WalletSnapshotRequest,
        OrderLifecycleRequest,
        TradeExecutionRequest,
        MarketLifecycleRequest>;

    class IRealtimePublisher {
    public:
        virtual ~IRealtimePublisher() = default;

        [[nodiscard]] virtual bool supports(RealtimePublishKind kind) const noexcept = 0;
        virtual void publish(const RealtimePublishRequest &request) const = 0;
    };

    void publishRealtime(const RealtimePublishRequest &request);

    void publishWalletSnapshot(const drogon::orm::DbClientPtr &db,
                               const std::string &userId);

    void publishOrderLifecycle(const drogon::orm::DbClientPtr &db,
                               const OrderRow &order,
                               std::string_view reason);

    void publishTradeExecution(const drogon::orm::DbClientPtr &db,
                               const TradeRow &trade);

    void publishMarketLifecycle(const MarketRow &market,
                                std::string_view reason);

    void publishMarketLifecycle(const MarketRow &market,
                                const std::vector<OutcomeRow> &outcomes,
                                std::string_view reason);

} // namespace pm::ws
