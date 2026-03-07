#include "pm/services/OrderService.h"

OrderService::OrderService(OrderRepository repo) : repo_(std::move(repo)) {}

void OrderService::createOrder(const std::string &userId,
                               const std::string &outcomeId,
                               const std::string &side,
                               int priceBp,
                               std::int64_t qtyMicros,
                               std::function<void(OrderRow)> onOk,
                               std::function<void(const pm::ApiError &)> onBizErr,
                               std::function<void(const drogon::orm::DrogonDbException &)> onErr) const {
    repo_.createOrderWithReservation(userId, outcomeId, side, priceBp, qtyMicros,
                                    std::move(onOk), std::move(onBizErr), std::move(onErr));
}

void OrderService::getOrderBook(const std::string &outcomeId,
                                int depth,
                                std::function<void(OrderBook)> onOk,
                                std::function<void(const drogon::orm::DrogonDbException &)> onErr) const {
    repo_.getOrderBook(outcomeId, depth, std::move(onOk), std::move(onErr));
}

void OrderService::getOrderForUser(const std::string &userId,
                                   const std::string &orderId,
                                   std::function<void(OrderRow)> onOk,
                                   std::function<void(const pm::ApiError &)> onBizErr,
                                   std::function<void(const drogon::orm::DrogonDbException &)> onErr) const {
    repo_.getOrderForUser(userId, orderId, std::move(onOk), std::move(onBizErr), std::move(onErr));
}

void OrderService::listOrdersForUser(const std::string &userId,
                                     const std::optional<std::string> &status,
                                     int limit,
                                     int offset,
                                     std::function<void(std::vector<OrderRow>)> onOk,
                                     std::function<void(const drogon::orm::DrogonDbException &)> onErr) const {
    repo_.listOrdersForUser(userId, status, limit, offset, std::move(onOk), std::move(onErr));
}

void OrderService::cancelOrderForUser(const std::string &userId,
                                      const std::string &orderId,
                                      std::function<void(OrderRow)> onOk,
                                      std::function<void(const pm::ApiError &)> onBizErr,
                                      std::function<void(const drogon::orm::DrogonDbException &)> onErr) const {
    repo_.cancelOrderWithRelease(userId, orderId, std::move(onOk), std::move(onBizErr), std::move(onErr));
}