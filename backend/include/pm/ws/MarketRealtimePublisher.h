#pragma once

#include "pm/repositories/MarketRepository.h"

#include <string_view>
#include <vector>

namespace pm::ws {

    void publishMarketLifecycle(const MarketRow &market,
                                std::string_view reason);

    void publishMarketLifecycle(const MarketRow &market,
                                const std::vector<OutcomeRow> &outcomes,
                                std::string_view reason);

} // namespace pm::ws
