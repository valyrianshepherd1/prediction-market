#include "pm/services/AuthService.h"

AuthService::AuthService(AuthRepository repo) : repo_(std::move(repo)) {
}

void AuthService::registerUser(const std::string &email,
                               const std::string &username,
                               const std::string &password,
                               int accessTtlMinutes,
                               int refreshTtlDays,
                               std::function<void(AuthSessionRow)> onOk,
                               std::function<void(const pm::ApiError &)> onBizErr,
                               std::function<void(const drogon::orm::DrogonDbException &)> onErr) const {
    repo_.registerUser(email,
                       username,
                       password,
                       accessTtlMinutes,
                       refreshTtlDays,
                       std::move(onOk),
                       std::move(onBizErr),
                       std::move(onErr));
}

void AuthService::login(const std::string &login,
                        const std::string &password,
                        int accessTtlMinutes,
                        int refreshTtlDays,
                        std::function<void(AuthSessionRow)> onOk,
                        std::function<void(const pm::ApiError &)> onBizErr,
                        std::function<void(const drogon::orm::DrogonDbException &)> onErr) const {
    repo_.login(login,
                password,
                accessTtlMinutes,
                refreshTtlDays,
                std::move(onOk),
                std::move(onBizErr),
                std::move(onErr));
}

void AuthService::refresh(const std::string &refreshToken,
                          int accessTtlMinutes,
                          int refreshTtlDays,
                          std::function<void(AuthSessionRow)> onOk,
                          std::function<void(const pm::ApiError &)> onBizErr,
                          std::function<void(const drogon::orm::DrogonDbException &)> onErr) const {
    repo_.refresh(refreshToken,
                  accessTtlMinutes,
                  refreshTtlDays,
                  std::move(onOk),
                  std::move(onBizErr),
                  std::move(onErr));
}

void AuthService::authenticateAccessToken(const std::string &accessToken,
                                          std::function<void(AuthSessionRow)> onOk,
                                          std::function<void(const pm::ApiError &)> onBizErr,
                                          std::function<void(const drogon::orm::DrogonDbException &)> onErr) const {
    repo_.authenticateAccessToken(accessToken,
                                  std::move(onOk),
                                  std::move(onBizErr),
                                  std::move(onErr));
}

void AuthService::logout(const std::string &refreshToken,
                         std::function<void()> onOk,
                         std::function<void(const drogon::orm::DrogonDbException &)> onErr) const {
    repo_.logout(refreshToken, std::move(onOk), std::move(onErr));
}
