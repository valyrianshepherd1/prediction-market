#pragma once

#include "pm/repositories/AuthRepository.h"

class AuthService {
public:
    explicit AuthService(AuthRepository repo);

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
    AuthRepository repo_;
};
