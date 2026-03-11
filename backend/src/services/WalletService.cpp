#include "pm/services/WalletService.h"

WalletService::WalletService(WalletRepository repo) : repo_(std::move(repo)) {
}

void WalletService::getWallet(const std::string &userId,
                              std::function<void(std::optional<WalletRow>)> onOk,
                              std::function<void(const drogon::orm::DrogonDbException &)> onErr) const {
    repo_.getWalletByUserId(userId, std::move(onOk), std::move(onErr));
}

void WalletService::deposit(const std::string &userId,
                            std::int64_t amount,
                            std::function<void(WalletRow)> onOk,
                            std::function<void(const pm::ApiError &)> onBizErr,
                            std::function<void(const drogon::orm::DrogonDbException &)> onErr) const {
    repo_.deposit(userId, amount, std::move(onOk), std::move(onBizErr), std::move(onErr));
}
