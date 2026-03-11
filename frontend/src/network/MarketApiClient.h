#pragma once

#include <QJsonObject>
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

struct ApiSession {
    QString userId;
    QString email;
    QString username;
    QString role;
    QString createdAt;
};

Q_DECLARE_METATYPE(ApiOutcome)
Q_DECLARE_METATYPE(ApiMarket)
Q_DECLARE_METATYPE(ApiWallet)
Q_DECLARE_METATYPE(ApiTrade)
Q_DECLARE_METATYPE(ApiSession)
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

    void restoreSession();
    void login(const QString &login, const QString &password);
    void registerUser(const QString &email, const QString &username, const QString &password);
    void logout();

    [[nodiscard]] QString baseUrl() const;
    [[nodiscard]] bool isAuthenticated() const;
    [[nodiscard]] ApiSession currentSession() const;

signals:
    void marketsReady(const QVector<ApiMarket> &markets);
    void walletReady(const ApiWallet &wallet);
    void tradesReady(const QVector<ApiTrade> &trades);

    void marketsError(const QString &message);
    void walletError(const QString &message);
    void tradesError(const QString &message);

    void authBusyChanged(bool busy);
    void sessionReady(const ApiSession &session);
    void sessionCleared();
    void authError(const QString &message);
    void loggedOut();

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
    void finishPendingMarketRequest();
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
};