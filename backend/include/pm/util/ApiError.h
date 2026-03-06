#pragma once

#include <drogon/HttpResponse.h>
#include <json/json.h>
#include <string>

namespace pm {

    struct ApiError {
        drogon::HttpStatusCode code{};
        std::string message;
        [[nodiscard]] explicit operator bool() const noexcept {
            return code == drogon::k200OK;
        }

        [[nodiscard]] bool ok() const noexcept {
            return static_cast<bool>(*this);
        }
    };

    inline drogon::HttpResponsePtr jsonError(const ApiError &e) {
        Json::Value j;
        j["error"] = e.message;
        auto resp = drogon::HttpResponse::newHttpJsonResponse(j);
        resp->setStatusCode(e.code);
        return resp;
    }
} // namespace pm