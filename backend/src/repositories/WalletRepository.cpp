#include "pm/repositories/WalletRepository.h"

#include <drogon/orm/DbClient.h>

using drogon::orm::DbClientPtr;
using drogon::orm::DrogonDbException;
using drogon::orm::Result;
using TransactionPtr = std::shared_ptr<drogon::orm::Transaction>;


namespace {

WalletRow rowToWallet(const Result &r, std::size_t i) {
    WalletRow w;
    w.user_id = r[i]["user_id"].as<std::string>();
    w.available = r[i]["available"].as<long long>();
    w.reserved = r[i]["reserved"].as<long long>();
    w.updated_at = r[i]["updated_at"].as<std::string>();
    return w;
}

} // namespace

WalletRepository::WalletRepository(DbClientPtr db) : db_(std::move(db)) {}

void WalletRepository::getWalletByUserId(const std::string &userId,
                                        std::function<void(std::optional<WalletRow>)> onOk,
                                        std::function<void(const DrogonDbException &)> onErr) const {
    static constexpr std::string_view kSql =
        "SELECT user_id::text AS user_id, available, reserved, updated_at::text AS updated_at "
        "FROM wallets WHERE user_id = $1::uuid";

    db_->execSqlAsync(
        std::string(kSql),
        [onOk = std::move(onOk)](const Result &r) mutable {
            if (r.empty()) return onOk(std::nullopt);
            onOk(rowToWallet(r, 0));
        },
        std::move(onErr),
        userId);
}

void WalletRepository::deposit(const std::string &userId,
                              std::int64_t amount,
                              std::function<void(WalletRow)> onOk,
                              std::function<void(const pm::ApiError &)> onBizErr,
                              std::function<void(const DrogonDbException &)> onErr) const {
    if (amount <= 0) {
        return onBizErr({drogon::k400BadRequest, "amount must be > 0"});
    }

    struct Ctx {
        TransactionPtr tx;
        std::string userId;
        std::int64_t amount{};
        std::function<void(WalletRow)> onOk;
        std::function<void(const pm::ApiError &)> onBizErr;
        std::function<void(const DrogonDbException &)> onErr;
    };

    auto ctx = std::make_shared<Ctx>(Ctx{
        nullptr, userId, amount,
        std::move(onOk), std::move(onBizErr), std::move(onErr)
    });

    db_->newTransactionAsync([ctx](const TransactionPtr &tx) {
        ctx->tx = tx;

        // ensure wallet exists
        tx->execSqlAsync(
            "INSERT INTO wallets(user_id) VALUES($1::uuid) ON CONFLICT DO NOTHING",
            [ctx](const Result &) {
                // update balance + return wallet
                ctx->tx->execSqlAsync(
                    "UPDATE wallets SET available = available + $2, updated_at = now() "
                    "WHERE user_id = $1::uuid "
                    "RETURNING user_id::text AS user_id, available, reserved, updated_at::text AS updated_at",
                    [ctx](const Result &r) {
                        if (r.empty()) {
                            ctx->tx->rollback();
                            return ctx->onBizErr({drogon::k404NotFound, "wallet/user not found"});
                        }
                        WalletRow w = rowToWallet(r, 0);

                        // ledger entry
                        ctx->tx->execSqlAsync(
                            "INSERT INTO cash_ledger(user_id, kind, delta_available, delta_reserved, ref_type, ref_id) "
                            "VALUES($1::uuid, 'DEPOSIT', $2, 0, 'ADMIN', gen_random_uuid())",
                            [ctx, w = std::move(w)](const Result &) mutable {
                                ctx->onOk(std::move(w));
                            },
                            ctx->onErr,
                            ctx->userId,
                            static_cast<long long>(ctx->amount));
                    },
                    ctx->onErr,
                    ctx->userId,
                    static_cast<long long>(ctx->amount));
            },
            ctx->onErr,
            ctx->userId);
    });
}