#include <pm/controllers/HealthController.h>

#include <drogon/drogon.h>
#include <json/json.h>

void HealthController::healthz(
    const drogon::HttpRequestPtr &,
    std::function<void(const drogon::HttpResponsePtr &)> &&cb) const {

    Json::Value j;
    j["status"] = "ok";
    cb(drogon::HttpResponse::newHttpJsonResponse(j));

}

void HealthController::healthzDb(
    const drogon::HttpRequestPtr &,
    std::function<void(const drogon::HttpResponsePtr &)> &&cb) const {
    drogon::orm::DbClientPtr client;
    try {
        client = drogon::app().getDbClient("default");
    } catch (const std::exception &e) {
        Json::Value j;
        j["status"] = "fail";
        j["error"] = std::string("db client not available: ") + e.what();
        auto resp = drogon::HttpResponse::newHttpJsonResponse(j);
        resp->setStatusCode(drogon::k500InternalServerError);
        return cb(resp);
    }

    client->execSqlAsync(
        "SELECT 1",
        [cb = std::move(cb)](const drogon::orm::Result &) mutable {
            Json::Value ok;
            ok["status"] = "ok";
            auto resp = drogon::HttpResponse::newHttpJsonResponse(ok);
            resp->setStatusCode(drogon::k200OK);
            cb(resp);
        },
        [cb = std::move(cb)](const drogon::orm::DrogonDbException &e) mutable {
            Json::Value fail;
            fail["status"] = "fail";
            fail["error"] = e.base().what();
            auto resp = drogon::HttpResponse::newHttpJsonResponse(fail);
            resp->setStatusCode(drogon::k503ServiceUnavailable);
            cb(resp);
        });
}
