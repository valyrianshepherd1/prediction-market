#pragma once

#include <QNetworkAccessManager>
#include <QObject>
#include <QString>
#include <QUrl>
#include <QVector>

#include <functional>

struct ApiOutcome {
    QString id;
    QString title;
    int pricePercent = 50;
    bool hasLivePrice = false;
};

struct ApiMarket {
    QString id;
    QString question;
    QString status;
    QString createdAt;
    QVector<ApiOutcome> outcomes;
};

struct ApiWallet {
    QString userId;
    qint64 available = 0;
    qint64 reserved = 0;
    QString updatedAt;
};

struct ApiTrade {
    QString id;
    QString outcomeId;
    QString makerUserId;
    QString takerUserId;
    QString makerOrderId;
    QString takerOrderId;
    int priceBasisPoints = 0;
    qint64 quantityMicros = 0;
    QString createdAt;
};

Q_DECLARE_METATYPE(ApiOutcome)
Q_DECLARE_METATYPE(ApiMarket)
Q_DECLARE_METATYPE(ApiWallet)
Q_DECLARE_METATYPE(ApiTrade)
Q_DECLARE_METATYPE(QVector<ApiMarket>)
Q_DECLARE_METATYPE(QVector<ApiTrade>)

class QJsonDocument;
class QNetworkReply;

class MarketApiClient : public QObject {
    Q_OBJECT
public:
    explicit MarketApiClient(QObject *parent = nullptr);

    void fetchMarkets();
    void fetchWallet();
    void fetchMyTrades(int limit = 50, int offset = 0);

    [[nodiscard]] QString baseUrl() const;
    [[nodiscard]] QString configuredUserId() const;

signals:
    void marketsReady(const QVector<ApiMarket> &markets);
    void walletReady(const ApiWallet &wallet);
    void tradesReady(const QVector<ApiTrade> &trades);

    void marketsError(const QString &message);
    void walletError(const QString &message);
    void tradesError(const QString &message);

private:
    void getJson(const QUrl &url,
                 const std::function<void(const QJsonDocument &)> &onSuccess,
                 const std::function<void(const QString &)> &onError = {});
    void ignoreSslErrorsIfNeeded(QNetworkReply *reply) const;
    void finishPendingMarketRequest();
    [[nodiscard]] QUrl apiUrl(const QString &pathWithQuery) const;

    QNetworkAccessManager m_manager;
    QUrl m_baseUrl;
    QString m_userId;
    QVector<ApiMarket> m_markets;
    int m_pendingMarketRequests = 0;
};