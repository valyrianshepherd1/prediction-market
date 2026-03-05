#pragma once

#include "pm/repositories/WalletRepository.h"

class WalletService {
public:
    explicit WalletService(WalletRepository repo);

    void getWallet(const std::string &userId,
                   std::function<void(std::optional<WalletRow>)> onOk,
                   std::function<void(const drogon::orm::DrogonDbException &)> onErr) const;

    void deposit(const std::string &userId,
                 std::int64_t amount,
                 std::function<void(WalletRow)> onOk,
                 std::function<void(const pm::ApiError &)> onBizErr,
                 std::function<void(const drogon::orm::DrogonDbException &)> onErr) const;

private:
    WalletRepository repo_;
};
