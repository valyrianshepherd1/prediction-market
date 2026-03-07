#pragma once

#include <drogon/HttpController.h>

class AuthController : public drogon::HttpController<AuthController> {
public:
    METHOD_LIST_BEGIN
        ADD_METHOD_TO(AuthController::registerUser, "/auth/register", drogon::Post);
        ADD_METHOD_TO(AuthController::login, "/auth/login", drogon::Post);
        ADD_METHOD_TO(AuthController::refresh, "/auth/refresh", drogon::Post);
        ADD_METHOD_TO(AuthController::logout, "/auth/logout", drogon::Post);
    METHOD_LIST_END

    void registerUser(const drogon::HttpRequestPtr &req,
                      std::function<void(const drogon::HttpResponsePtr &)> &&cb) const;

    void login(const drogon::HttpRequestPtr &req,
               std::function<void(const drogon::HttpResponsePtr &)> &&cb) const;

    void refresh(const drogon::HttpRequestPtr &req,
                 std::function<void(const drogon::HttpResponsePtr &)> &&cb) const;

    void logout(const drogon::HttpRequestPtr &req,
                std::function<void(const drogon::HttpResponsePtr &)> &&cb) const;
};
