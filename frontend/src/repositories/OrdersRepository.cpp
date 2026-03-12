#include "frontend/repositories/OrdersRepository.h"

#include "frontend/network/MarketApiClient.h"

OrdersRepository::OrdersRepository(MarketApiClient *transport, QObject *parent)
    : QObject(parent),
      m_transport(transport) {
    Q_ASSERT(m_transport != nullptr);

    connect(m_transport, &MarketApiClient::ordersReady, this, [this](const QVector<ApiOrder> &orders) {
        m_orders = orders;
        emit ordersReady(m_orders);
    });
    connect(m_transport, &MarketApiClient::ordersError, this, &OrdersRepository::ordersError);

    connect(m_transport, &MarketApiClient::orderBusyChanged, this, &OrdersRepository::orderBusyChanged);
    connect(m_transport, &MarketApiClient::orderCreated, this, &OrdersRepository::orderCreated);
    connect(m_transport, &MarketApiClient::orderError, this, &OrdersRepository::orderError);
    connect(m_transport, &MarketApiClient::orderCancelBusyChanged, this, &OrdersRepository::orderCancelBusyChanged);
    connect(m_transport, &MarketApiClient::orderCanceled, this, [this](const ApiOrder &order) {
        bool replaced = false;
        for (ApiOrder &existing : m_orders) {
            if (existing.id == order.id) {
                existing = order;
                replaced = true;
                break;
            }
        }
        if (!replaced) {
            m_orders.prepend(order);
        }
        emit ordersReady(m_orders);
        emit orderCanceled(order);
    });
    connect(m_transport, &MarketApiClient::orderCancelError, this, &OrdersRepository::orderCancelError);
}

void OrdersRepository::refreshMyOrders(int limit, int offset, const QString &status) {
    if (m_transport) {
        m_transport->fetchMyOrders(limit, offset, status);
    }
}

void OrdersRepository::createOrder(const QString &outcomeId,
                                   const QString &side,
                                   int priceBasisPoints,
                                   qint64 quantityMicros) {
    if (m_transport) {
        m_transport->createOrder(outcomeId, side, priceBasisPoints, quantityMicros);
    }
}

QVector<ApiOrder> OrdersRepository::orders() const {
    return m_orders;
}

void OrdersRepository::cancelOrder(const QString &orderId) {
    if (m_transport) {
        m_transport->cancelOrder(orderId);
    }
}
