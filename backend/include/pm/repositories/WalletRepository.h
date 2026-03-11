#pragma once

#include "pm/util/ApiError.h"

#include <drogon/orm/DbClient.h>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>

struct WalletRow {
    std::string user_id;
    std::int64_t available{};
    std::int64_t reserved{};
    std::string updated_at;
};

class WalletRepository {
public:
    explicit WalletRepository(drogon::orm::DbClientPtr db);

    void getWalletByUserId(const std::string &userId,
                           std::function<void(std::optional<WalletRow>)> onOk,
                           std::function<void(const drogon::orm::DrogonDbException &)> onErr) const;

    void deposit(const std::string &userId,
                 std::int64_t amount,
                 std::function<void(WalletRow)> onOk,
                 std::function<void(const pm::ApiError &)> onBizErr,
                 std::function<void(const drogon::orm::DrogonDbException &)> onErr) const;

private:
    drogon::orm::DbClientPtr db_;
};
