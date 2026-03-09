#pragma once

#include "pm/util/ApiError.h"

#include <drogon/drogon.h>

#include <functional>
#include <string>

namespace pm::auth {

    struct Principal {
        std::string session_id;
        std::string user_id;
        std::string email;
        std::string username;
        std::string role;
        std::string created_at;
    };

    void requireAuthenticatedUser(
        const drogon::HttpRequestPtr &req,
        std::function<void(Principal)> onOk,
        std::function<void(const pm::ApiError &)> onBizErr,
        std::function<void(const drogon::orm::DrogonDbException &)> onErr);

    void requireAdminUser(
        const drogon::HttpRequestPtr &req,
        std::function<void(Principal)> onOk,
        std::function<void(const pm::ApiError &)> onBizErr,
        std::function<void(const drogon::orm::DrogonDbException &)> onErr);

}  // namespace pm::auth
