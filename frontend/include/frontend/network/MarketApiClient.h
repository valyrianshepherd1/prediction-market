#pragma once

#include "ApiTypes.h"

#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QObject>
#include <QString>
#include <QUrl>
#include <QVector>

#include <functional>

class QJsonDocument;
class QNetworkReply;

class MarketApiClient : public QObject {
    Q_OBJECT
public:
    explicit MarketApiClient(QObject *parent = nullptr);

    void fetchMarkets();
    void fetchWallet();
    void fetchMyTrades(int limit = 50, int offset = 0);
    void fetchMyOrders(int limit = 100, int offset = 0, const QString &status = {});
    void fetchPortfolio(int limit = 100, int offset = 0);
    void fetchLedger(int limit = 100, int offset = 0);

    void restoreSession();
    void login(const QString &login, const QString &password);
    void registerUser(const QString &email, const QString &username, const QString &password);
    void logout();

    void adminDepositToCurrentUser(qint64 amount);

    void createOrder(const QString &outcomeId,
                     const QString &side,
                     int priceBasisPoints,
                     qint64 quantityMicros);
    void cancelOrder(const QString &orderId);

    void fetchMarketById(const QString &marketId,
                         const std::function<void(const ApiMarket &)> &onSuccess,
                         const std::function<void(const QString &)> &onError = {});
    void fetchOutcomesForMarket(const QString &marketId,
                                const std::function<void(const QVector<ApiOutcome> &)> &onSuccess,
                                const std::function<void(const QString &)> &onError = {});
    void fetchOrderBookForOutcome(const QString &outcomeId,
                                  int depth,
                                  const std::function<void(const ApiOrderBook &)> &onSuccess,
                                  const std::function<void(const QString &)> &onError = {});
    void fetchTradesForOutcome(const QString &outcomeId,
                               int limit,
                               int offset,
                               const std::function<void(const QVector<ApiTrade> &)> &onSuccess,
                               const std::function<void(const QString &)> &onError = {});

    [[nodiscard]] QString baseUrl() const;
    [[nodiscard]] QUrl websocketUrl() const;
    [[nodiscard]] QString accessToken() const;
    [[nodiscard]] bool isAuthenticated() const;
    [[nodiscard]] ApiSession currentSession() const;

signals:
    void marketsReady(const QVector<ApiMarket> &markets);
    void walletReady(const ApiWallet &wallet);
    void tradesReady(const QVector<ApiTrade> &trades);
    void ordersReady(const QVector<ApiOrder> &orders);
    void portfolioReady(const QVector<ApiPortfolioPosition> &positions);
    void ledgerReady(const QVector<ApiPortfolioLedgerEntry> &entries);

    void marketsError(const QString &message);
    void walletError(const QString &message);
    void tradesError(const QString &message);
    void ordersError(const QString &message);
    void portfolioError(const QString &message);
    void ledgerError(const QString &message);

    void authBusyChanged(bool busy);
    void sessionReady(const ApiSession &session);
    void sessionCleared();
    void authError(const QString &message);
    void loggedOut();

    void orderBusyChanged(bool busy);
    void orderCreated(const ApiOrder &order);
    void orderCanceled(const ApiOrder &order);
    void orderError(const QString &message);
    void orderCancelBusyChanged(bool busy);
    void orderCancelError(const QString &message);

    void depositBusyChanged(bool busy);
    void depositSucceeded(const ApiWallet &wallet);
    void depositError(const QString &message);

private:
    void getJson(const QUrl &url,
                 bool includeAuth,
                 const std::function<void(const QJsonDocument &)> &onSuccess,
                 const std::function<void(int, const QString &)> &onError = {});

    void postJson(const QUrl &url,
                  const QJsonObject &body,
                  bool includeAuth,
                  const std::function<void(const QJsonDocument &)> &onSuccess,
                  const std::function<void(int, const QString &)> &onError = {});

    void addCommonHeaders(QNetworkRequest &request, bool includeAuth) const;
    void ignoreSslErrorsIfNeeded(QNetworkReply *reply) const;
    void finishPendingMarketRequest(int requestGeneration);
    [[nodiscard]] QUrl apiUrl(const QString &pathWithQuery) const;

    void loadPersistedSession();
    void persistSession() const;
    void clearPersistedSession() const;
    void clearSession(bool emitSignal = true);

    bool storeSessionFromAuthResponse(const QJsonDocument &document, bool emitSignal);
    bool storeSessionFromUserResponse(const QJsonDocument &document, bool emitSignal);

    void refreshSession(const std::function<void()> &onSuccess,
                        const std::function<void(const QString &)> &onFailure);

    QNetworkAccessManager m_manager;
    QUrl m_baseUrl;

    QString m_accessToken;
    QString m_refreshToken;
    ApiSession m_session;

    QVector<ApiMarket> m_markets;
    int m_pendingMarketRequests = 0;
    int m_marketsRequestGeneration = 0;
};
