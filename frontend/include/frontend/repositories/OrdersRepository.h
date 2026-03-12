#pragma once

#include "frontend/network/ApiTypes.h"

#include <QObject>
#include <QVector>

class MarketApiClient;

class OrdersRepository : public QObject {
    Q_OBJECT
public:
    explicit OrdersRepository(MarketApiClient *transport, QObject *parent = nullptr);

    void refreshMyOrders(int limit = 100, int offset = 0, const QString &status = {});
    void createOrder(const QString &outcomeId,
                     const QString &side,
                     int priceBasisPoints,
                     qint64 quantityMicros);
    void cancelOrder(const QString &orderId);

    [[nodiscard]] QVector<ApiOrder> orders() const;

    signals:
        void ordersReady(const QVector<ApiOrder> &orders);
    void ordersError(const QString &message);

    void orderBusyChanged(bool busy);
    void orderCreated(const ApiOrder &order);
    void orderCanceled(const ApiOrder &order);
    void orderError(const QString &message);
    void orderCancelBusyChanged(bool busy);
    void orderCancelError(const QString &message);

private:
    MarketApiClient *m_transport = nullptr;
    QVector<ApiOrder> m_orders;
};
