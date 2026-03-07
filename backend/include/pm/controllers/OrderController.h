#pragma once

#include <drogon/HttpController.h>
#include <string>

class OrderController : public drogon::HttpController<OrderController> {
public:
    METHOD_LIST_BEGIN
        ADD_METHOD_TO(OrderController::createOrder, "/orders", drogon::Post);
        ADD_METHOD_TO(OrderController::getOrder, "/orders/{1}", drogon::Get);
        ADD_METHOD_TO(OrderController::cancelOrder, "/orders/{1}", drogon::Delete);
        ADD_METHOD_TO(OrderController::listMyOrders, "/me/orders", drogon::Get);
        ADD_METHOD_TO(OrderController::getOrderBook, "/outcomes/{1}/orderbook", drogon::Get);
    METHOD_LIST_END

    void createOrder(const drogon::HttpRequestPtr &req,
                     std::function<void(const drogon::HttpResponsePtr &)> &&cb) const;

    void getOrder(const drogon::HttpRequestPtr &req,
                  std::function<void(const drogon::HttpResponsePtr &)> &&cb,
                  std::string orderId) const;

    void cancelOrder(const drogon::HttpRequestPtr &req,
                     std::function<void(const drogon::HttpResponsePtr &)> &&cb,
                     std::string orderId) const;

    void listMyOrders(const drogon::HttpRequestPtr &req,
                      std::function<void(const drogon::HttpResponsePtr &)> &&cb) const;

    void getOrderBook(const drogon::HttpRequestPtr &req,
                      std::function<void(const drogon::HttpResponsePtr &)> &&cb,
                      std::string outcomeId) const;
};
