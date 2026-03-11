#pragma once

#include <drogon/WebSocketConnection.h>
#include <drogon/orm/DbClient.h>

#include <json/json.h>

#include <string>

namespace pm::ws {

    void sendTopicSnapshot(const drogon::WebSocketConnectionPtr &connection,
                           const drogon::orm::DbClientPtr &db,
                           const std::string &topic,
                           const Json::Value &options = Json::Value(Json::objectValue));

} // namespace pm::ws
