#include "frontend/repositories/MarketDetailsRepository.h"

#include "frontend/network/MarketApiClient.h"
#include "frontend/network/MarketWsClient.h"

#include <QJsonObject>
#include <QTimer>
#include <QtAssert>
#include <QtGlobal>

#include <memory>

namespace {

QString upperTrimmed(const QString &value) {
    return value.trimmed().toUpper();
}

int indicativePricePercent(const ApiOrderBook &orderBook) {
    if (!orderBook.buy.isEmpty()) {
        return qBound(0, orderBook.buy.first().priceBasisPoints / 100, 100);
    }
    if (!orderBook.sell.isEmpty()) {
        return qBound(0, orderBook.sell.first().priceBasisPoints / 100, 100);
    }
    return -1;
}

QString marketTopicForId(const QString &marketId) {
    return marketId.trimmed().isEmpty()
               ? QString()
               : QStringLiteral("market:%1").arg(marketId.trimmed());
}

QString orderbookTopicForOutcome(const QString &outcomeId) {
    return outcomeId.trimmed().isEmpty()
               ? QString()
               : QStringLiteral("orderbook:%1").arg(outcomeId.trimmed());
}

QString tradesTopicForOutcome(const QString &outcomeId) {
    return outcomeId.trimmed().isEmpty()
               ? QString()
               : QStringLiteral("trades:%1").arg(outcomeId.trimmed());
}

} // namespace

MarketDetailsRepository::MarketDetailsRepository(MarketApiClient *transport, QObject *parent)
    : QObject(parent),
      m_transport(transport) {
    Q_ASSERT(m_transport != nullptr);
}

void MarketDetailsRepository::setRealtimeClient(MarketWsClient *realtimeClient) {
    if (m_realtimeClient == realtimeClient) {
        return;
    }

    m_realtimeClient = realtimeClient;
    if (!m_realtimeClient) {
        return;
    }

    connect(m_realtimeClient,
            &MarketWsClient::topicMessageReceived,
            this,
            [this](const QString &topic,
                   const QString &event,
                   const QJsonObject &,
                   quint64,
                   bool) {
                if (m_currentMarketId.trimmed().isEmpty()) {
                    return;
                }

                if (topic == m_marketTopic &&
                    event == QStringLiteral("market.updated")) {
                    scheduleFullRefresh();
                    return;
                }

                if (topic == m_orderbookTopic &&
                    event == QStringLiteral("orderbook.invalidate")) {
                    scheduleSelectedOutcomeRefresh();
                    return;
                }

                if (topic == m_tradesTopic &&
                    event == QStringLiteral("trade.created")) {
                    scheduleSelectedOutcomeRefresh();
                }
            });

    updateRealtimeSubscriptions();
}

void MarketDetailsRepository::openMarket(const QString &marketId, const QString &preferredSelection) {
    if (m_marketTopic != marketTopicForId(marketId) && m_realtimeClient) {
        if (!m_marketTopic.isEmpty()) {
            m_realtimeClient->unsubscribeTopic(m_marketTopic);
        }
        if (!m_orderbookTopic.isEmpty()) {
            m_realtimeClient->unsubscribeTopic(m_orderbookTopic);
        }
        if (!m_tradesTopic.isEmpty()) {
            m_realtimeClient->unsubscribeTopic(m_tradesTopic);
        }
        m_marketTopic.clear();
        m_orderbookTopic.clear();
        m_tradesTopic.clear();
    }

    m_currentMarketId = marketId.trimmed();
    m_preferredSelection = upperTrimmed(preferredSelection);

    if (m_currentMarketId.isEmpty()) {
        emit errorOccurred(QStringLiteral("Market id is missing."));
        return;
    }

    ++m_requestId;
    m_snapshot = ApiMarketDetailsSnapshot{};
    m_snapshot.market.id = m_currentMarketId;
    updateRealtimeSubscriptions();
    setLoading(true);
    fetchMarketAndOutcomes(m_requestId);
}

void MarketDetailsRepository::refresh() {
    if (m_currentMarketId.trimmed().isEmpty()) {
        return;
    }

    ++m_requestId;
    setLoading(true);
    fetchMarketAndOutcomes(m_requestId);
}

void MarketDetailsRepository::refreshSelectedOutcomeData() {
    if (m_currentMarketId.trimmed().isEmpty() || m_snapshot.selectedOutcomeId.trimmed().isEmpty()) {
        return;
    }

    ++m_requestId;
    updateRealtimeSubscriptions();
    setLoading(true);
    fetchSelectedOutcomeData(m_requestId);
}

void MarketDetailsRepository::selectOutcomeById(const QString &outcomeId) {
    const QString normalizedId = outcomeId.trimmed();
    if (normalizedId.isEmpty() || normalizedId == m_snapshot.selectedOutcomeId) {
        return;
    }

    bool found = false;
    for (const ApiOutcome &outcome : m_snapshot.market.outcomes) {
        if (outcome.id == normalizedId) {
            found = true;
            break;
        }
    }
    if (!found) {
        return;
    }

    m_preferredSelection.clear();
    m_snapshot.selectedOutcomeId = normalizedId;
    updateRealtimeSubscriptions();
    emitSnapshot();

    ++m_requestId;
    setLoading(true);
    fetchSelectedOutcomeData(m_requestId);
}

void MarketDetailsRepository::selectOutcomeByTitle(const QString &titleUpper) {
    const QString normalizedTitle = upperTrimmed(titleUpper);
    for (const ApiOutcome &outcome : m_snapshot.market.outcomes) {
        if (upperTrimmed(outcome.title) == normalizedTitle) {
            selectOutcomeById(outcome.id);
            return;
        }
    }
}

ApiMarketDetailsSnapshot MarketDetailsRepository::snapshot() const {
    return m_snapshot;
}

void MarketDetailsRepository::emitSnapshot() {
    emit snapshotReady(m_snapshot);
}

void MarketDetailsRepository::setLoading(bool loading) {
    if (m_loading == loading) {
        return;
    }
    m_loading = loading;
    emit loadingChanged(m_loading);
}

void MarketDetailsRepository::failRequest(const QString &message, int requestId) {
    if (requestId != m_requestId) {
        return;
    }
    setLoading(false);
    emit errorOccurred(message);
}

void MarketDetailsRepository::fetchMarketAndOutcomes(int requestId) {
    m_transport->fetchMarketById(
        m_currentMarketId,
        [this, requestId](const ApiMarket &market) {
            if (requestId != m_requestId) {
                return;
            }

            m_snapshot.market = market;

            m_transport->fetchOutcomesForMarket(
                m_currentMarketId,
                [this, requestId, market](const QVector<ApiOutcome> &outcomes) {
                    if (requestId != m_requestId) {
                        return;
                    }

                    m_snapshot.market = market;
                    m_snapshot.market.outcomes = outcomes;
                    m_snapshot.selectedOutcomeId = resolvePreferredOutcomeId(outcomes);
                    updateRealtimeSubscriptions();
                    emitSnapshot();
                    fetchSelectedOutcomeData(requestId);
                },
                [this, requestId](const QString &message) {
                    failRequest(message, requestId);
                });
        },
        [this, requestId](const QString &message) {
            failRequest(message, requestId);
        });
}

void MarketDetailsRepository::fetchSelectedOutcomeData(int requestId) {
    if (requestId != m_requestId) {
        return;
    }

    const QString outcomeId = m_snapshot.selectedOutcomeId.trimmed();
    if (outcomeId.isEmpty()) {
        setLoading(false);
        emitSnapshot();
        return;
    }

    struct OutcomeFetchState {
        bool orderBookDone = false;
        bool tradesDone = false;
        ApiOrderBook orderBook;
        QVector<ApiTrade> trades;
    };

    auto state = std::make_shared<OutcomeFetchState>();

    auto finalize = [this, requestId, state]() {
        if (requestId != m_requestId) {
            return;
        }
        if (!state->orderBookDone || !state->tradesDone) {
            return;
        }

        m_snapshot.orderBook = state->orderBook;
        m_snapshot.recentTrades = state->trades;

        applyIndicativePriceFromOrderBook(m_snapshot.orderBook);
        applyPriceFallbackFromTrades(m_snapshot.recentTrades);

        emitSnapshot();
        setLoading(false);
    };

    m_transport->fetchOrderBookForOutcome(
        outcomeId,
        20,
        [this, requestId, state, finalize](const ApiOrderBook &orderBook) {
            if (requestId != m_requestId) {
                return;
            }
            state->orderBook = orderBook;
            state->orderBookDone = true;
            finalize();
        },
        [this, requestId, state, finalize](const QString &message) {
            if (requestId != m_requestId) {
                return;
            }
            state->orderBookDone = true;
            emit errorOccurred(message);
            finalize();
        });

    m_transport->fetchTradesForOutcome(
        outcomeId,
        20,
        0,
        [this, requestId, state, finalize](const QVector<ApiTrade> &trades) {
            if (requestId != m_requestId) {
                return;
            }
            state->trades = trades;
            state->tradesDone = true;
            finalize();
        },
        [this, requestId, state, finalize](const QString &message) {
            if (requestId != m_requestId) {
                return;
            }
            state->tradesDone = true;
            emit errorOccurred(message);
            finalize();
        });
}

QString MarketDetailsRepository::resolvePreferredOutcomeId(const QVector<ApiOutcome> &outcomes) const {
    const QString preferred = upperTrimmed(m_preferredSelection);
    if (!preferred.isEmpty()) {
        for (const ApiOutcome &outcome : outcomes) {
            if (upperTrimmed(outcome.title) == preferred) {
                return outcome.id;
            }
        }
    }

    if (!m_snapshot.selectedOutcomeId.trimmed().isEmpty()) {
        for (const ApiOutcome &outcome : outcomes) {
            if (outcome.id == m_snapshot.selectedOutcomeId) {
                return outcome.id;
            }
        }
    }

    for (const ApiOutcome &outcome : outcomes) {
        if (upperTrimmed(outcome.title) == QStringLiteral("YES")) {
            return outcome.id;
        }
    }

    if (!outcomes.isEmpty()) {
        return outcomes.first().id;
    }

    return {};
}

ApiOutcome *MarketDetailsRepository::selectedOutcomeMutable() {
    for (ApiOutcome &outcome : m_snapshot.market.outcomes) {
        if (outcome.id == m_snapshot.selectedOutcomeId) {
            return &outcome;
        }
    }
    return nullptr;
}

void MarketDetailsRepository::applyIndicativePriceFromOrderBook(const ApiOrderBook &orderBook) {
    ApiOutcome *outcome = selectedOutcomeMutable();
    if (!outcome) {
        return;
    }

    const int pricePercent = indicativePricePercent(orderBook);
    if (pricePercent < 0) {
        return;
    }

    outcome->pricePercent = pricePercent;
    outcome->hasLivePrice = true;
}

void MarketDetailsRepository::applyPriceFallbackFromTrades(const QVector<ApiTrade> &trades) {
    ApiOutcome *outcome = selectedOutcomeMutable();
    if (!outcome || outcome->hasLivePrice || trades.isEmpty()) {
        return;
    }

    outcome->pricePercent = qBound(0, trades.first().priceBasisPoints / 100, 100);
}

void MarketDetailsRepository::updateRealtimeSubscriptions() {
    if (!m_realtimeClient) {
        return;
    }

    const QString nextMarketTopic = marketTopicForId(m_currentMarketId);
    const QString nextOrderbookTopic = orderbookTopicForOutcome(m_snapshot.selectedOutcomeId);
    const QString nextTradesTopic = tradesTopicForOutcome(m_snapshot.selectedOutcomeId);

    if (m_marketTopic != nextMarketTopic) {
        if (!m_marketTopic.isEmpty()) {
            m_realtimeClient->unsubscribeTopic(m_marketTopic);
        }
        m_marketTopic = nextMarketTopic;
        if (!m_marketTopic.isEmpty()) {
            m_realtimeClient->subscribeTopic(m_marketTopic, true);
        }
    }

    if (m_orderbookTopic != nextOrderbookTopic) {
        if (!m_orderbookTopic.isEmpty()) {
            m_realtimeClient->unsubscribeTopic(m_orderbookTopic);
        }
        m_orderbookTopic = nextOrderbookTopic;
        if (!m_orderbookTopic.isEmpty()) {
            m_realtimeClient->subscribeTopic(m_orderbookTopic, true);
        }
    }

    if (m_tradesTopic != nextTradesTopic) {
        if (!m_tradesTopic.isEmpty()) {
            m_realtimeClient->unsubscribeTopic(m_tradesTopic);
        }
        m_tradesTopic = nextTradesTopic;
        if (!m_tradesTopic.isEmpty()) {
            m_realtimeClient->subscribeTopic(m_tradesTopic, true);
        }
    }
}

void MarketDetailsRepository::scheduleFullRefresh() {
    if (m_fullRefreshQueued || m_currentMarketId.trimmed().isEmpty()) {
        return;
    }

    m_fullRefreshQueued = true;
    QTimer::singleShot(75, this, [this]() {
        m_fullRefreshQueued = false;
        refresh();
    });
}

void MarketDetailsRepository::scheduleSelectedOutcomeRefresh() {
    if (m_selectedRefreshQueued || m_snapshot.selectedOutcomeId.trimmed().isEmpty()) {
        return;
    }

    m_selectedRefreshQueued = true;
    QTimer::singleShot(75, this, [this]() {
        m_selectedRefreshQueued = false;
        refreshSelectedOutcomeData();
    });
}
