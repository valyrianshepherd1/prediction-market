#pragma once

#include <drogon/HttpResponse.h>
#include <json/json.h>
#include <string>

namespace pm {

    struct ApiError {
        drogon::HttpStatusCode code{};
        std::string message;
    };

    inline drogon::HttpResponsePtr jsonError(const ApiError &e) {
        Json::Value j;
        j["error"] = e.message;
        auto resp = drogon::HttpResponse::newHttpJsonResponse(j);
        resp->setStatusCode(e.code);
        return resp;
    }

} // namespace pm