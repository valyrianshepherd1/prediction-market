#pragma once

#include "frontend/network/ApiTypes.h"

#include <QObject>
#include <QVector>

class MarketApiClient;
class MarketWsClient;

class MarketsRepository : public QObject {
    Q_OBJECT
public:
    explicit MarketsRepository(MarketApiClient *transport, QObject *parent = nullptr);

    void refresh();
    void setRealtimeClient(MarketWsClient *realtimeClient);
    [[nodiscard]] QVector<ApiMarket> markets() const;

    signals:
        void marketsReady(const QVector<ApiMarket> &markets);
    void marketsError(const QString &message);

private:
    MarketApiClient *m_transport = nullptr;
    MarketWsClient *m_realtimeClient = nullptr;
    QVector<ApiMarket> m_markets;
};
