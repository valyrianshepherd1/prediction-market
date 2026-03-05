#pragma once

#include "pm/util/ApiError.h"

#include <drogon/orm/DbClient.h>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

struct OrderRow {
    std::string id;
    std::string user_id;
    std::string outcome_id;
    std::string side; // BUY / SELL
    int price_bp{};
    std::int64_t qty_total_micros{};
    std::int64_t qty_remaining_micros{};
    std::string status; // OPEN / PARTIALLY_FILLED / FILLED / CANCELED
    std::string created_at;
    std::string updated_at;
};

struct OrderBook {
    std::vector<OrderRow> buy;
    std::vector<OrderRow> sell;
};

class OrderRepository {
public:
    explicit OrderRepository(drogon::orm::DbClientPtr db);

    void createOrderWithReservation(const std::string &userId,
                                    const std::string &outcomeId,
                                    const std::string &side,
                                    int priceBp,
                                    std::int64_t qtyMicros,
                                    std::function<void(OrderRow)> onOk,
                                    std::function<void(const pm::ApiError &)> onBizErr,
                                    std::function<void(const drogon::orm::DrogonDbException &)> onErr) const;

    void getOrderBook(const std::string &outcomeId,
                      int depth,
                      std::function<void(OrderBook)> onOk,
                      std::function<void(const drogon::orm::DrogonDbException &)> onErr) const;

private:
    drogon::orm::DbClientPtr db_;
};
