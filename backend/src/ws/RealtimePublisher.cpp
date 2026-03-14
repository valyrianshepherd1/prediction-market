#include "pm/ws/RealtimePublisher.h"

#include "pm/ws/MarketRealtimePublisher.h"
#include "pm/ws/OrderRealtimePublisher.h"
#include "pm/ws/TradeRealtimePublisher.h"
#include "pm/ws/WalletRealtimePublisher.h"

#include <memory>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace {

    [[nodiscard]] pm::ws::RealtimePublishKind requestKind(const pm::ws::RealtimePublishRequest &request) {
        return std::visit(
            [](const auto &typedRequest) -> pm::ws::RealtimePublishKind {
                using T = std::decay_t<decltype(typedRequest)>;
                if constexpr (std::is_same_v<T, pm::ws::WalletSnapshotRequest>) {
                    return pm::ws::RealtimePublishKind::WalletSnapshot;
                } else if constexpr (std::is_same_v<T, pm::ws::OrderLifecycleRequest>) {
                    return pm::ws::RealtimePublishKind::OrderLifecycle;
                } else if constexpr (std::is_same_v<T, pm::ws::TradeExecutionRequest>) {
                    return pm::ws::RealtimePublishKind::TradeExecution;
                } else {
                    return pm::ws::RealtimePublishKind::MarketLifecycle;
                }
            },
            request);
    }

    class RealtimePublisherRegistry {
    public:
        static RealtimePublisherRegistry &instance() {
            static RealtimePublisherRegistry registry;
            return registry;
        }

        void publish(const pm::ws::RealtimePublishRequest &request) const {
            const auto kind = requestKind(request);
            for (const auto &publisher: publishers_) {
                if (publisher && publisher->supports(kind)) {
                    publisher->publish(request);
                    return;
                }
            }
        }

    private:
        RealtimePublisherRegistry() {
            publishers_.push_back(std::make_unique<pm::ws::WalletRealtimePublisher>());
            publishers_.push_back(std::make_unique<pm::ws::OrderRealtimePublisher>());
            publishers_.push_back(std::make_unique<pm::ws::TradeRealtimePublisher>());
            publishers_.push_back(std::make_unique<pm::ws::MarketRealtimePublisher>());
        }

        std::vector<std::unique_ptr<pm::ws::IRealtimePublisher>> publishers_;
    };

} // namespace

namespace pm::ws {

    void publishRealtime(const RealtimePublishRequest &request) {
        RealtimePublisherRegistry::instance().publish(request);
    }

    void publishWalletSnapshot(const drogon::orm::DbClientPtr &db,
                               const std::string &userId) {
        publishRealtime(WalletSnapshotRequest{db, userId});
    }

    void publishOrderLifecycle(const drogon::orm::DbClientPtr &db,
                               const OrderRow &order,
                               std::string_view reason) {
        publishRealtime(OrderLifecycleRequest{db, order, std::string(reason)});
    }

    void publishTradeExecution(const drogon::orm::DbClientPtr &db,
                               const TradeRow &trade) {
        publishRealtime(TradeExecutionRequest{db, trade});
    }

    void publishMarketLifecycle(const MarketRow &market,
                                std::string_view reason) {
        publishRealtime(MarketLifecycleRequest{market, std::nullopt, std::string(reason)});
    }

    void publishMarketLifecycle(const MarketRow &market,
                                const std::vector<OutcomeRow> &outcomes,
                                std::string_view reason) {
        publishRealtime(MarketLifecycleRequest{market, std::optional<std::vector<OutcomeRow>>{outcomes}, std::string(reason)});
    }

} // namespace pm::ws
