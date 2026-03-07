#pragma once

#include "pm/repositories/OrderRepository.h"
#include <optional>

class OrderService {
public:
    explicit OrderService(OrderRepository repo);

    void createOrder(const std::string &userId,
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

    void getOrderForUser(const std::string &userId,
                         const std::string &orderId,
                         std::function<void(OrderRow)> onOk,
                         std::function<void(const pm::ApiError &)> onBizErr,
                         std::function<void(const drogon::orm::DrogonDbException &)> onErr) const;

    void listOrdersForUser(const std::string &userId,
                           const std::optional<std::string> &status,
                           int limit,
                           int offset,
                           std::function<void(std::vector<OrderRow>)> onOk,
                           std::function<void(const drogon::orm::DrogonDbException &)> onErr) const;

    void cancelOrderForUser(const std::string &userId,
                            const std::string &orderId,
                            std::function<void(OrderRow)> onOk,
                            std::function<void(const pm::ApiError &)> onBizErr,
                            std::function<void(const drogon::orm::DrogonDbException &)> onErr) const;

private:
    OrderRepository repo_;
};
