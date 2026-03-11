#include "pm/util/AuthGuard.h"

#include "pm/repositories/AuthRepository.h"
#include "pm/services/AuthService.h"

#include <memory>
#include <string_view>

namespace pm::auth {
namespace {

pm::ApiError getDbOrError(drogon::orm::DbClientPtr &db) {
    try {
        db = drogon::app().getDbClient("default");
        return {drogon::k200OK, ""};
    } catch (const std::exception &e) {
        return {drogon::k500InternalServerError, std::string("db client not available: ") + e.what()};
    }
}

pm::ApiError extractBearer(const drogon::HttpRequestPtr &req, std::string &tokenOut) {
    const auto authz = req->getHeader("Authorization");
    constexpr std::string_view kPrefix = "Bearer ";
    if (authz.size() <= kPrefix.size() || authz.compare(0, kPrefix.size(), kPrefix) != 0) {
        return {drogon::k401Unauthorized, "Authorization: Bearer <access_token> is required"};
    }
    tokenOut = authz.substr(kPrefix.size());
    if (tokenOut.empty()) {
        return {drogon::k401Unauthorized, "Authorization: Bearer <access_token> is required"};
    }
    return {drogon::k200OK, ""};
}

Principal toPrincipal(const AuthSessionRow &s) {
    Principal p;
    p.session_id = s.session_id;
    p.user_id = s.user.id;
    p.email = s.user.email;
    p.username = s.user.username;
    p.role = s.user.role;
    p.created_at = s.user.created_at;
    return p;
}

}  // namespace

void requireAuthenticatedUser(
    const drogon::HttpRequestPtr &req,
    std::function<void(Principal)> onOk,
    std::function<void(const pm::ApiError &)> onBizErr,
    std::function<void(const drogon::orm::DrogonDbException &)> onErr) {
    std::string accessToken;
    if (auto e = extractBearer(req, accessToken); !e) {
        return onBizErr(e);
    }

    drogon::orm::DbClientPtr db;
    if (auto e = getDbOrError(db); !e) {
        return onBizErr(e);
    }

    auto svc = std::make_shared<AuthService>(AuthRepository{db});
    svc->authenticateAccessToken(
        accessToken,
        [svc, onOk = std::move(onOk)](AuthSessionRow session) mutable {
            onOk(toPrincipal(session));
        },
        std::move(onBizErr),
        std::move(onErr));
}

void requireAdminUser(
    const drogon::HttpRequestPtr &req,
    std::function<void(Principal)> onOk,
    std::function<void(const pm::ApiError &)> onBizErr,
    std::function<void(const drogon::orm::DrogonDbException &)> onErr) {
    requireAuthenticatedUser(
        req,
        [onOk = std::move(onOk), onBizErr](Principal principal) mutable {
            if (principal.role != "admin") {
                return onBizErr({drogon::k403Forbidden, "admin role is required"});
            }
            onOk(std::move(principal));
        },
        std::move(onBizErr),
        std::move(onErr));
}

}  // namespace pm::auth
