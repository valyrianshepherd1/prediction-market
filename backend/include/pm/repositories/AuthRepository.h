#pragma once

#include "pm/util/ApiError.h"

#include <drogon/orm/DbClient.h>

#include <functional>
#include <string>

struct AuthUserRow {
    std::string id;
    std::string email;
    std::string username;
    std::string role;
    std::string created_at;
};

struct AuthSessionRow {
    std::string session_id;
    AuthUserRow user;
    std::string refresh_token;
    std::string refresh_expires_at;
};

class AuthRepository {
public:
    explicit AuthRepository(drogon::orm::DbClientPtr db);

    void registerUser(const std::string &email,
                      const std::string &username,
                      const std::string &password,
                      int refreshTtlDays,
                      std::function<void(AuthSessionRow)> onOk,
                      std::function<void(const pm::ApiError &)> onBizErr,
                      std::function<void(const drogon::orm::DrogonDbException &)> onErr) const;

    void login(const std::string &login,
               const std::string &password,
               int refreshTtlDays,
               std::function<void(AuthSessionRow)> onOk,
               std::function<void(const pm::ApiError &)> onBizErr,
               std::function<void(const drogon::orm::DrogonDbException &)> onErr) const;

    void refresh(const std::string &refreshToken,
                 int refreshTtlDays,
                 std::function<void(AuthSessionRow)> onOk,
                 std::function<void(const pm::ApiError &)> onBizErr,
                 std::function<void(const drogon::orm::DrogonDbException &)> onErr) const;

    void logout(const std::string &refreshToken,
                std::function<void()> onOk,
                std::function<void(const drogon::orm::DrogonDbException &)> onErr) const;

private:
    drogon::orm::DbClientPtr db_;
};
