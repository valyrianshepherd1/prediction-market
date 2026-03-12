#include "frontend/repositories/TradesRepository.h"

#include "frontend/network/MarketApiClient.h"

TradesRepository::TradesRepository(MarketApiClient *transport, QObject *parent)
    : QObject(parent),
      m_transport(transport) {
    Q_ASSERT(m_transport != nullptr);

    connect(m_transport, &MarketApiClient::tradesReady, this, [this](const QVector<ApiTrade> &tradesValue) {
        m_trades = tradesValue;
        emit tradesReady(m_trades);
    });
    connect(m_transport, &MarketApiClient::tradesError, this, &TradesRepository::tradesError);
}

void TradesRepository::refreshMyTrades(int limit, int offset) {
    if (m_transport) {
        m_transport->fetchMyTrades(limit, offset);
    }
}

QVector<ApiTrade> TradesRepository::trades() const {
    return m_trades;
}
