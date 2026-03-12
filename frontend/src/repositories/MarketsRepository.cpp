#include "frontend/repositories/MarketsRepository.h"

#include "frontend/network/MarketApiClient.h"
#include "frontend/network/MarketWsClient.h"

MarketsRepository::MarketsRepository(MarketApiClient *transport, QObject *parent)
    : QObject(parent),
      m_transport(transport) {
    Q_ASSERT(m_transport != nullptr);

    connect(m_transport, &MarketApiClient::marketsReady, this, [this](const QVector<ApiMarket> &markets) {
        m_markets = markets;
        emit marketsReady(m_markets);
    });

    connect(m_transport, &MarketApiClient::marketsError, this, &MarketsRepository::marketsError);
}

void MarketsRepository::refresh() {
    if (m_transport) {
        m_transport->fetchMarkets();
    }
}

void MarketsRepository::setRealtimeClient(MarketWsClient *realtimeClient) {
    if (m_realtimeClient == realtimeClient) {
        return;
    }

    m_realtimeClient = realtimeClient;
    if (!m_realtimeClient) {
        return;
    }

    m_realtimeClient->subscribeTopic(QStringLiteral("markets"), true);

    connect(m_realtimeClient,
            &MarketWsClient::topicMessageReceived,
            this,
            [this](const QString &topic,
                   const QString &event,
                   const QJsonObject &,
                   quint64,
                   bool) {
                if (topic != QStringLiteral("markets")) {
                    return;
                }

                if (event == QStringLiteral("markets.invalidate")) {
                    refresh();
                }
            });
}

QVector<ApiMarket> MarketsRepository::markets() const {
    return m_markets;
}
