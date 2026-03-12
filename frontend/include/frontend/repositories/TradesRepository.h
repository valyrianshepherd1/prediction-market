#pragma once

#include "frontend/network/ApiTypes.h"

#include <QObject>
#include <QVector>

class MarketApiClient;

class TradesRepository : public QObject {
    Q_OBJECT
public:
    explicit TradesRepository(MarketApiClient *transport, QObject *parent = nullptr);

    void refreshMyTrades(int limit = 50, int offset = 0);
    [[nodiscard]] QVector<ApiTrade> trades() const;

signals:
    void tradesReady(const QVector<ApiTrade> &trades);
    void tradesError(const QString &message);

private:
    MarketApiClient *m_transport = nullptr;
    QVector<ApiTrade> m_trades;
};
