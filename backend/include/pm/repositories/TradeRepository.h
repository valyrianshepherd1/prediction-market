#pragma once

#include <drogon/orm/DbClient.h>

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

struct TradeRow {
    std::string id;
    std::string outcome_id;
    std::string maker_user_id;
    std::string taker_user_id;
    std::string maker_order_id;
    std::string taker_order_id;
    int price_bp{};
    std::int64_t qty_micros{};
    std::string created_at;
};

class TradeRepository {
public:
    explicit TradeRepository(drogon::orm::DbClientPtr db);

    void listByOutcome(const std::string &outcomeId, int limit, int offset,
                       std::function<void(std::vector<TradeRow>)> onOk,
                       std::function<void(const drogon::orm::DrogonDbException &)> onErr) const;

    void listByUser(const std::string &userId, int limit, int offset,
                    std::function<void(std::vector<TradeRow>)> onOk,
                    std::function<void(const drogon::orm::DrogonDbException &)> onErr) const;

    void listByOrder(const std::string &orderId,
                     std::function<void(std::vector<TradeRow>)> onOk,
                     std::function<void(const drogon::orm::DrogonDbException &)> onErr) const;

private:
    drogon::orm::DbClientPtr db_;
};
