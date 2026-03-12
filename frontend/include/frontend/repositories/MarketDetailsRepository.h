#pragma once

#include "frontend/network/ApiTypes.h"

#include <QObject>

class MarketApiClient;
class MarketWsClient;

class MarketDetailsRepository : public QObject {
    Q_OBJECT
public:
    explicit MarketDetailsRepository(MarketApiClient *transport, QObject *parent = nullptr);

    void setRealtimeClient(MarketWsClient *realtimeClient);
    void openMarket(const QString &marketId, const QString &preferredSelection = {});
    void refresh();
    void refreshSelectedOutcomeData();
    void selectOutcomeById(const QString &outcomeId);
    void selectOutcomeByTitle(const QString &titleUpper);

    [[nodiscard]] ApiMarketDetailsSnapshot snapshot() const;

    signals:
        void snapshotReady(const ApiMarketDetailsSnapshot &snapshot);
    void loadingChanged(bool loading);
    void errorOccurred(const QString &message);

private:
    void emitSnapshot();
    void setLoading(bool loading);
    void failRequest(const QString &message, int requestId);
    void fetchMarketAndOutcomes(int requestId);
    void fetchSelectedOutcomeData(int requestId);
    [[nodiscard]] QString resolvePreferredOutcomeId(const QVector<ApiOutcome> &outcomes) const;
    ApiOutcome *selectedOutcomeMutable();
    void applyIndicativePriceFromOrderBook(const ApiOrderBook &orderBook);
    void applyPriceFallbackFromTrades(const QVector<ApiTrade> &trades);
    void updateRealtimeSubscriptions();
    void scheduleFullRefresh();
    void scheduleSelectedOutcomeRefresh();

    MarketApiClient *m_transport = nullptr;
    MarketWsClient *m_realtimeClient = nullptr;
    ApiMarketDetailsSnapshot m_snapshot;
    QString m_currentMarketId;
    QString m_preferredSelection;
    QString m_marketTopic;
    QString m_orderbookTopic;
    QString m_tradesTopic;
    bool m_loading = false;
    bool m_fullRefreshQueued = false;
    bool m_selectedRefreshQueued = false;
    int m_requestId = 0;
};
