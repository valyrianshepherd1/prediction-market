#include "pm/controllers/AuthController.h"

#include "pm/repositories/AuthRepository.h"
#include "pm/services/AuthService.h"
#include "pm/util/ApiError.h"

#include <charconv>
#include <cstdlib>
#include <memory>
#include <string_view>

using drogon::HttpResponse;
using drogon::HttpResponsePtr;

namespace {
drogon::orm::DbClientPtr getDbOrNull(std::string &err) {
    try {
        return drogon::app().getDbClient("default");
    } catch (const std::exception &e) {
        err = e.what();
        return nullptr;
    }
}

pm::ApiError validateEmail(const std::string &email) {
    if (email.size() < 5 || email.size() > 320 || email.find('@') == std::string::npos) {
        return {drogon::k400BadRequest, "email must be a valid email-like string"};
    }
    return {drogon::k200OK, ""};
}

pm::ApiError validateUsername(const std::string &username) {
    if (username.size() < 3 || username.size() > 50) {
        return {drogon::k400BadRequest, "username must be 3..50 chars"};
    }
    return {drogon::k200OK, ""};
}

pm::ApiError validatePassword(const std::string &password) {
    if (password.size() < 8 || password.size() > 200) {
        return {drogon::k400BadRequest, "password must be 8..200 chars"};
    }
    return {drogon::k200OK, ""};
}

int refreshTtlDays() {
    int out = 30;
    const char *raw = std::getenv("PM_REFRESH_TTL_DAYS");
    if (!raw || std::string_view(raw).empty()) {
        return out;
    }

    int parsed = 0;
    const auto view = std::string_view(raw);
    const auto *begin = view.data();
    const auto *end = view.data() + view.size();
    const auto [ptr, ec] = std::from_chars(begin, end, parsed);
    if (ec == std::errc{} && ptr == end && parsed > 0 && parsed <= 365) {
        out = parsed;
    }
    return out;
}

Json::Value userToJson(const AuthUserRow &u) {
    Json::Value j;
    j["id"] = u.id;
    j["email"] = u.email;
    j["username"] = u.username;
    j["role"] = u.role;
    j["created_at"] = u.created_at;
    return j;
}

Json::Value sessionToJson(const AuthSessionRow &s) {
    Json::Value j;
    j["session_id"] = s.session_id;
    j["user"] = userToJson(s.user);
    j["x_user_id"] = s.user.id;
    j["refresh_token"] = s.refresh_token;
    j["refresh_expires_at"] = s.refresh_expires_at;
    return j;
}

HttpResponsePtr jsonOk(const Json::Value &j, drogon::HttpStatusCode code = drogon::k200OK) {
    auto resp = HttpResponse::newHttpJsonResponse(j);
    resp->setStatusCode(code);
    return resp;
}
}  // namespace

void AuthController::registerUser(const drogon::HttpRequestPtr &req,
                                  std::function<void(const drogon::HttpResponsePtr &)> &&cb) const {
    auto cbp = std::make_shared<std::function<void(const HttpResponsePtr &)>> (std::move(cb));

    const auto json = req->getJsonObject();
    if (!json) {
        return (*cbp)(pm::jsonError({drogon::k400BadRequest, "expected JSON body"}));
    }

    const auto email = (*json).get("email", "").asString();
    const auto username = (*json).get("username", "").asString();
    const auto password = (*json).get("password", "").asString();

    if (auto e = validateEmail(email); !e) return (*cbp)(pm::jsonError(e));
    if (auto e = validateUsername(username); !e) return (*cbp)(pm::jsonError(e));
    if (auto e = validatePassword(password); !e) return (*cbp)(pm::jsonError(e));

    std::string dbErr;
    auto db = getDbOrNull(dbErr);
    if (!db) {
        return (*cbp)(pm::jsonError({drogon::k500InternalServerError, "db client not available: " + dbErr}));
    }

    AuthService svc{AuthRepository{db}};
    svc.registerUser(
        email,
        username,
        password,
        refreshTtlDays(),
        [cbp](AuthSessionRow session) {
            (*cbp)(jsonOk(sessionToJson(session), drogon::k201Created));
        },
        [cbp](const pm::ApiError &e) {
            (*cbp)(pm::jsonError(e));
        },
        [cbp](const drogon::orm::DrogonDbException &e) {
            (*cbp)(pm::jsonError({drogon::k503ServiceUnavailable, e.base().what()}));
        });
}

void AuthController::login(const drogon::HttpRequestPtr &req,
                           std::function<void(const drogon::HttpResponsePtr &)> &&cb) const {
    auto cbp = std::make_shared<std::function<void(const HttpResponsePtr &)>> (std::move(cb));

    const auto json = req->getJsonObject();
    if (!json) {
        return (*cbp)(pm::jsonError({drogon::k400BadRequest, "expected JSON body"}));
    }

    std::string loginValue = (*json).get("login", "").asString();
    if (loginValue.empty()) loginValue = (*json).get("email", "").asString();
    if (loginValue.empty()) loginValue = (*json).get("username", "").asString();
    const auto password = (*json).get("password", "").asString();

    if (loginValue.empty()) {
        return (*cbp)(pm::jsonError({drogon::k400BadRequest, "login (or email/username) is required"}));
    }
    if (auto e = validatePassword(password); !e) return (*cbp)(pm::jsonError(e));

    std::string dbErr;
    auto db = getDbOrNull(dbErr);
    if (!db) {
        return (*cbp)(pm::jsonError({drogon::k500InternalServerError, "db client not available: " + dbErr}));
    }

    AuthService svc{AuthRepository{db}};
    svc.login(
        loginValue,
        password,
        refreshTtlDays(),
        [cbp](AuthSessionRow session) {
            (*cbp)(jsonOk(sessionToJson(session)));
        },
        [cbp](const pm::ApiError &e) {
            (*cbp)(pm::jsonError(e));
        },
        [cbp](const drogon::orm::DrogonDbException &e) {
            (*cbp)(pm::jsonError({drogon::k503ServiceUnavailable, e.base().what()}));
        });
}

void AuthController::refresh(const drogon::HttpRequestPtr &req,
                             std::function<void(const drogon::HttpResponsePtr &)> &&cb) const {
    auto cbp = std::make_shared<std::function<void(const HttpResponsePtr &)>> (std::move(cb));

    const auto json = req->getJsonObject();
    if (!json) {
        return (*cbp)(pm::jsonError({drogon::k400BadRequest, "expected JSON body"}));
    }

    const auto token = (*json).get("refresh_token", "").asString();
    if (token.empty()) {
        return (*cbp)(pm::jsonError({drogon::k400BadRequest, "refresh_token is required"}));
    }

    std::string dbErr;
    auto db = getDbOrNull(dbErr);
    if (!db) {
        return (*cbp)(pm::jsonError({drogon::k500InternalServerError, "db client not available: " + dbErr}));
    }

    AuthService svc{AuthRepository{db}};
    svc.refresh(
        token,
        refreshTtlDays(),
        [cbp](AuthSessionRow session) {
            (*cbp)(jsonOk(sessionToJson(session)));
        },
        [cbp](const pm::ApiError &e) {
            (*cbp)(pm::jsonError(e));
        },
        [cbp](const drogon::orm::DrogonDbException &e) {
            (*cbp)(pm::jsonError({drogon::k503ServiceUnavailable, e.base().what()}));
        });
}

void AuthController::logout(const drogon::HttpRequestPtr &req,
                            std::function<void(const drogon::HttpResponsePtr &)> &&cb) const {
    auto cbp = std::make_shared<std::function<void(const HttpResponsePtr &)>> (std::move(cb));

    const auto json = req->getJsonObject();
    if (!json) {
        return (*cbp)(pm::jsonError({drogon::k400BadRequest, "expected JSON body"}));
    }

    const auto token = (*json).get("refresh_token", "").asString();
    if (token.empty()) {
        return (*cbp)(pm::jsonError({drogon::k400BadRequest, "refresh_token is required"}));
    }

    std::string dbErr;
    auto db = getDbOrNull(dbErr);
    if (!db) {
        return (*cbp)(pm::jsonError({drogon::k500InternalServerError, "db client not available: " + dbErr}));
    }

    AuthService svc{AuthRepository{db}};
    svc.logout(
        token,
        [cbp]() {
            Json::Value j;
            j["ok"] = true;
            (*cbp)(jsonOk(j));
        },
        [cbp](const drogon::orm::DrogonDbException &e) {
            (*cbp)(pm::jsonError({drogon::k503ServiceUnavailable, e.base().what()}));
        });
}
