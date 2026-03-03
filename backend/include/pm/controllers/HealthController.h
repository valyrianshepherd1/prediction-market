#pragma once
#include <drogon/HttpController.h>

class HealthController : public drogon::HttpController<HealthController> {
public:
    METHOD_LIST_BEGIN
        ADD_METHOD_TO(HealthController::healthz, "/healthz", drogon::Get);
        ADD_METHOD_TO(HealthController::healthzDb, "/healthz/db", drogon::Get);
    METHOD_LIST_END

    void healthz(const drogon::HttpRequestPtr &, std::function<void(const drogon::HttpResponsePtr &)> &&) const;

    void healthzDb(const drogon::HttpRequestPtr &, std::function<void(const drogon::HttpResponsePtr &)> &&) const;
};
