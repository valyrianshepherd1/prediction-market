#pragma once

#include "pm/ws/RealtimePublisher.h"

namespace pm::ws {

    class WalletRealtimePublisher final : public IRealtimePublisher {
    public:
        [[nodiscard]] bool supports(RealtimePublishKind kind) const noexcept override;
        void publish(const RealtimePublishRequest &request) const override;
    };

} // namespace pm::ws
