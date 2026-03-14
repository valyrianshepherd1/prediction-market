#include "pm/ws/WalletRealtimePublisher.h"

#include "pm/repositories/WalletRepository.h"
#include "pm/util/JsonSerializers.h"
#include "pm/ws/WsHub.h"

#include <drogon/orm/DbClient.h>

#include <optional>

using drogon::orm::DrogonDbException;

namespace pm::ws {

    bool WalletRealtimePublisher::supports(RealtimePublishKind kind) const noexcept {
        return kind == RealtimePublishKind::WalletSnapshot;
    }

    void WalletRealtimePublisher::publish(const RealtimePublishRequest &request) const {
        const auto *walletRequest = std::get_if<WalletSnapshotRequest>(&request);
        if (!walletRequest || !walletRequest->db || walletRequest->user_id.empty()) {
            return;
        }

        WalletRepository{walletRequest->db}.getWalletByUserId(
            walletRequest->user_id,
            [userId = walletRequest->user_id](std::optional<WalletRow> wallet) {
                if (!wallet) {
                    return;
                }

                WsHub::instance().publish(
                    "user:" + userId,
                    "user.wallet.updated",
                    pm::json::toJson(*wallet));
            },
            [](const DrogonDbException &) {
                //realtime notification failure must not break request flow
            });
    }

} // namespace pm::ws
