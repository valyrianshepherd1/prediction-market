#include <pm/controllers/HealthController.h>

#include <drogon/drogon.h>
#include <json/json.h>

void HealthController::healthz(
    const drogon::HttpRequestPtr&,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback) const {
    Json::Value j;
    j["status"] = "ok";
    callback(drogon::HttpResponse::newHttpJsonResponse(j));
}