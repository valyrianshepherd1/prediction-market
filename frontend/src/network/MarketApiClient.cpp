#include "MarketApiClient.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSettings>
#include <QSslError>
#include <QUrlQuery>

namespace {

QString normalizeBaseUrl() {
    const QString configured = qEnvironmentVariable("PM_API_BASE_URL");
    if (!configured.trimmed().isEmpty()) {
        return configured;
    }
    return QStringLiteral("https://localhost:8443");
}

int fallbackPercentForOutcomeCount(const int outcomeCount) {
    if (outcomeCount <= 0) {
        return 0;
    }
    return qMax(1, 100 / outcomeCount);
}

int bestIndicativePrice(const QJsonObject &orderBook, bool *hasPrice = nullptr) {
    if (hasPrice) {
        *hasPrice = false;
    }

    const auto buyArray = orderBook.value(QStringLiteral("buy")).toArray();
    if (!buyArray.isEmpty()) {
        if (hasPrice) {
            *hasPrice = true;
        }
        return qBound(0, buyArray.first().toObject().value(QStringLiteral("price_bp")).toInt() / 100, 100);
    }

    const auto sellArray = orderBook.value(QStringLiteral("sell")).toArray();
    if (!sellArray.isEmpty()) {
        if (hasPrice) {
            *hasPrice = true;
        }
        return qBound(0, sellArray.first().toObject().value(QStringLiteral("price_bp")).toInt() / 100, 100);
    }

    return 0;
}

QString responseErrorMessage(QNetworkReply *reply, const QByteArray &payload) {
    const int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

    if (!payload.trimmed().isEmpty()) {
        return QStringLiteral("HTTP %1: %2")
            .arg(statusCode > 0 ? QString::number(statusCode) : QStringLiteral("error"))
            .arg(QString::fromUtf8(payload).trimmed());
    }

    if (statusCode > 0) {
        return QStringLiteral("HTTP %1").arg(statusCode);
    }

    return reply->errorString();
}

ApiSession sessionFromUserObject(const QJsonObject &userObject) {
    ApiSession session;
    session.userId = userObject.value(QStringLiteral("id")).toString();
    session.email = userObject.value(QStringLiteral("email")).toString();
    session.username = userObject.value(QStringLiteral("username")).toString();
    session.role = userObject.value(QStringLiteral("role")).toString();
    session.createdAt = userObject.value(QStringLiteral("created_at")).toString();
    return session;
}

} // namespace

MarketApiClient::MarketApiClient(QObject *parent)
    : QObject(parent),
      m_baseUrl(QUrl(normalizeBaseUrl())) {
    loadPersistedSession();
}

QString MarketApiClient::baseUrl() const {
    return m_baseUrl.toString();
}

bool MarketApiClient::isAuthenticated() const {
    return !m_accessToken.trimmed().isEmpty() && !m_session.userId.trimmed().isEmpty();
}

ApiSession MarketApiClient::currentSession() const {
    return m_session;
}

QUrl MarketApiClient::apiUrl(const QString &pathWithQuery) const {
    QUrl url = m_baseUrl;
    QString normalizedPath = pathWithQuery;

    if (!normalizedPath.startsWith('/')) {
        normalizedPath.prepend('/');
    }

    const int querySeparator = normalizedPath.indexOf('?');
    if (querySeparator >= 0) {
        url.setPath(normalizedPath.left(querySeparator));

        QUrlQuery query;
        query.setQuery(normalizedPath.mid(querySeparator + 1));
        url.setQuery(query);
    } else {
        url.setPath(normalizedPath);
        url.setQuery(QString());
    }

    return url;
}

void MarketApiClient::addCommonHeaders(QNetworkRequest &request, bool includeAuth) const {
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    request.setRawHeader("Accept", "application/json");

    if (includeAuth && !m_accessToken.trimmed().isEmpty()) {
        request.setRawHeader("Authorization", QByteArray("Bearer ") + m_accessToken.toUtf8());
    }
}

void MarketApiClient::ignoreSslErrorsIfNeeded(QNetworkReply *reply) const {
    if (!reply) {
        return;
    }

    const QUrl url = reply->url();
    const bool isLocalhost =
        url.host() == QStringLiteral("localhost") ||
        url.host() == QStringLiteral("127.0.0.1");

    if (url.scheme() == QStringLiteral("https") && isLocalhost) {
        QObject::connect(reply, &QNetworkReply::sslErrors, reply, [reply](const QList<QSslError> &) {
            reply->ignoreSslErrors();
        });
        reply->ignoreSslErrors();
    }
}

void MarketApiClient::getJson(const QUrl &url,
                              bool includeAuth,
                              const std::function<void(const QJsonDocument &)> &onSuccess,
                              const std::function<void(int, const QString &)> &onError) {
    QNetworkRequest request(url);
    addCommonHeaders(request, includeAuth);

    QNetworkReply *reply = m_manager.get(request);
    ignoreSslErrorsIfNeeded(reply);

    connect(reply, &QNetworkReply::finished, this, [reply, onSuccess, onError]() {
        const QByteArray payload = reply->readAll();
        const int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        if (reply->error() != QNetworkReply::NoError || statusCode < 200 || statusCode >= 300) {
            if (onError) {
                onError(statusCode, responseErrorMessage(reply, payload));
            }
            reply->deleteLater();
            return;
        }

        QJsonParseError parseError;
        const QJsonDocument document = QJsonDocument::fromJson(payload, &parseError);

        if (parseError.error != QJsonParseError::NoError) {
            if (onError) {
                onError(statusCode,
                        QStringLiteral("Invalid JSON from %1: %2")
                            .arg(reply->url().toString(), parseError.errorString()));
            }
            reply->deleteLater();
            return;
        }

        if (onSuccess) {
            onSuccess(document);
        }

        reply->deleteLater();
    });
}

void MarketApiClient::postJson(const QUrl &url,
                               const QJsonObject &body,
                               bool includeAuth,
                               const std::function<void(const QJsonDocument &)> &onSuccess,
                               const std::function<void(int, const QString &)> &onError) {
    QNetworkRequest request(url);
    addCommonHeaders(request, includeAuth);

    QNetworkReply *reply = m_manager.post(request, QJsonDocument(body).toJson(QJsonDocument::Compact));
    ignoreSslErrorsIfNeeded(reply);

    connect(reply, &QNetworkReply::finished, this, [reply, onSuccess, onError]() {
        const QByteArray payload = reply->readAll();
        const int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        if (reply->error() != QNetworkReply::NoError || statusCode < 200 || statusCode >= 300) {
            if (onError) {
                onError(statusCode, responseErrorMessage(reply, payload));
            }
            reply->deleteLater();
            return;
        }

        QJsonParseError parseError;
        const QJsonDocument document = QJsonDocument::fromJson(payload, &parseError);

        if (parseError.error != QJsonParseError::NoError) {
            if (onError) {
                onError(statusCode,
                        QStringLiteral("Invalid JSON from %1: %2")
                            .arg(reply->url().toString(), parseError.errorString()));
            }
            reply->deleteLater();
            return;
        }

        if (onSuccess) {
            onSuccess(document);
        }

        reply->deleteLater();
    });
}

void MarketApiClient::loadPersistedSession() {
    QSettings settings;

    m_accessToken = settings.value(QStringLiteral("auth/access_token")).toString();
    m_refreshToken = settings.value(QStringLiteral("auth/refresh_token")).toString();
    m_session.userId = settings.value(QStringLiteral("auth/user_id")).toString();
    m_session.email = settings.value(QStringLiteral("auth/email")).toString();
    m_session.username = settings.value(QStringLiteral("auth/username")).toString();
    m_session.role = settings.value(QStringLiteral("auth/role")).toString();
    m_session.createdAt = settings.value(QStringLiteral("auth/created_at")).toString();
}

void MarketApiClient::persistSession() const {
    QSettings settings;

    settings.setValue(QStringLiteral("auth/access_token"), m_accessToken);
    settings.setValue(QStringLiteral("auth/refresh_token"), m_refreshToken);
    settings.setValue(QStringLiteral("auth/user_id"), m_session.userId);
    settings.setValue(QStringLiteral("auth/email"), m_session.email);
    settings.setValue(QStringLiteral("auth/username"), m_session.username);
    settings.setValue(QStringLiteral("auth/role"), m_session.role);
    settings.setValue(QStringLiteral("auth/created_at"), m_session.createdAt);
}

void MarketApiClient::clearPersistedSession() const {
    QSettings settings;
    settings.remove(QStringLiteral("auth"));
}

void MarketApiClient::clearSession(bool emitSignal) {
    m_accessToken.clear();
    m_refreshToken.clear();
    m_session = ApiSession{};
    clearPersistedSession();

    if (emitSignal) {
        emit sessionCleared();
    }
}

bool MarketApiClient::storeSessionFromAuthResponse(const QJsonDocument &document, bool emitSignal) {
    if (!document.isObject()) {
        return false;
    }

    const QJsonObject root = document.object();
    const QJsonObject userObject = root.value(QStringLiteral("user")).toObject();

    if (userObject.isEmpty()) {
        return false;
    }

    const QString accessToken = root.value(QStringLiteral("access_token")).toString();
    const QString refreshToken = root.value(QStringLiteral("refresh_token")).toString();

    if (accessToken.trimmed().isEmpty()) {
        return false;
    }

    m_accessToken = accessToken;

    if (!refreshToken.trimmed().isEmpty()) {
        m_refreshToken = refreshToken;
    }

    m_session = sessionFromUserObject(userObject);
    persistSession();

    if (emitSignal) {
        emit sessionReady(m_session);
    }

    return true;
}

bool MarketApiClient::storeSessionFromUserResponse(const QJsonDocument &document, bool emitSignal) {
    if (!document.isObject()) {
        return false;
    }

    const QJsonObject root = document.object();
    const ApiSession session = sessionFromUserObject(root);

    if (session.userId.trimmed().isEmpty()) {
        return false;
    }

    m_session = session;
    persistSession();

    if (emitSignal) {
        emit sessionReady(m_session);
    }

    return true;
}

void MarketApiClient::refreshSession(const std::function<void()> &onSuccess,
                                     const std::function<void(const QString &)> &onFailure) {
    if (m_refreshToken.trimmed().isEmpty()) {
        if (onFailure) {
            onFailure(QStringLiteral("Your session expired. Please log in again."));
        }
        return;
    }

    QJsonObject body;
    body.insert(QStringLiteral("refresh_token"), m_refreshToken);

    postJson(apiUrl(QStringLiteral("/auth/refresh")),
             body,
             false,
             [this, onSuccess, onFailure](const QJsonDocument &document) {
                 if (!storeSessionFromAuthResponse(document, false)) {
                     clearSession(true);
                     if (onFailure) {
                         onFailure(QStringLiteral("Invalid refresh response from backend."));
                     }
                     return;
                 }

                 if (onSuccess) {
                     onSuccess();
                 }
             },
             [this, onFailure](int, const QString &message) {
                 clearSession(true);
                 if (onFailure) {
                     onFailure(message);
                 }
             });
}

void MarketApiClient::restoreSession() {
    if (m_accessToken.trimmed().isEmpty() && m_refreshToken.trimmed().isEmpty()) {
        emit sessionCleared();
        return;
    }

    if (m_accessToken.trimmed().isEmpty() && !m_refreshToken.trimmed().isEmpty()) {
        refreshSession(
            [this]() { restoreSession(); },
            [this](const QString &) { clearSession(true); });
        return;
    }

    getJson(apiUrl(QStringLiteral("/me")),
            true,
            [this](const QJsonDocument &document) {
                if (!storeSessionFromUserResponse(document, true)) {
                    clearSession(true);
                }
            },
            [this](int statusCode, const QString &) {
                if (statusCode == 401 && !m_refreshToken.trimmed().isEmpty()) {
                    refreshSession(
                        [this]() { restoreSession(); },
                        [this](const QString &) { clearSession(true); });
                    return;
                }

                clearSession(true);
            });
}

void MarketApiClient::login(const QString &loginValue, const QString &password) {
    emit authBusyChanged(true);

    QJsonObject body;
    body.insert(QStringLiteral("login"), loginValue);
    body.insert(QStringLiteral("password"), password);

    postJson(apiUrl(QStringLiteral("/auth/login")),
             body,
             false,
             [this](const QJsonDocument &document) {
                 emit authBusyChanged(false);

                 if (!storeSessionFromAuthResponse(document, true)) {
                     emit authError(QStringLiteral("Invalid login response from backend."));
                 }
             },
             [this](int, const QString &message) {
                 emit authBusyChanged(false);
                 emit authError(message);
             });
}

void MarketApiClient::registerUser(const QString &email,
                                   const QString &username,
                                   const QString &password) {
    emit authBusyChanged(true);

    QJsonObject body;
    body.insert(QStringLiteral("email"), email);
    body.insert(QStringLiteral("username"), username);
    body.insert(QStringLiteral("password"), password);

    postJson(apiUrl(QStringLiteral("/auth/register")),
             body,
             false,
             [this](const QJsonDocument &document) {
                 emit authBusyChanged(false);

                 if (!storeSessionFromAuthResponse(document, true)) {
                     emit authError(QStringLiteral("Invalid sign-up response from backend."));
                 }
             },
             [this](int, const QString &message) {
                 emit authBusyChanged(false);
                 emit authError(message);
             });
}

void MarketApiClient::logout() {
    const QString refreshToken = m_refreshToken;
    clearSession(true);

    if (refreshToken.trimmed().isEmpty()) {
        emit loggedOut();
        return;
    }

    QJsonObject body;
    body.insert(QStringLiteral("refresh_token"), refreshToken);

    postJson(apiUrl(QStringLiteral("/auth/logout")),
             body,
             false,
             [this](const QJsonDocument &) {
                 emit loggedOut();
             },
             [this](int, const QString &) {
                 emit loggedOut();
             });
}

void MarketApiClient::finishPendingMarketRequest() {
    if (m_pendingMarketRequests <= 0) {
        return;
    }

    --m_pendingMarketRequests;
    if (m_pendingMarketRequests == 0) {
        emit marketsReady(m_markets);
    }
}

void MarketApiClient::fetchMarkets() {
    m_markets.clear();
    m_pendingMarketRequests = 0;

    getJson(apiUrl(QStringLiteral("/markets?status=OPEN&limit=100")),
            false,
            [this](const QJsonDocument &document) {
                if (!document.isArray()) {
                    emit marketsError(QStringLiteral("/markets did not return an array."));
                    return;
                }

                const QJsonArray marketsArray = document.array();
                if (marketsArray.isEmpty()) {
                    emit marketsReady({});
                    return;
                }

                m_markets.reserve(marketsArray.size());
                m_pendingMarketRequests = marketsArray.size();

                for (const QJsonValue &marketValue : marketsArray) {
                    const QJsonObject marketObject = marketValue.toObject();

                    ApiMarket market;
                    market.id = marketObject.value(QStringLiteral("id")).toString();
                    market.question = marketObject.value(QStringLiteral("question")).toString();
                    market.status = marketObject.value(QStringLiteral("status")).toString();
                    market.createdAt = marketObject.value(QStringLiteral("created_at")).toString();

                    const int marketIndex = m_markets.size();
                    m_markets.push_back(market);

                    getJson(apiUrl(QStringLiteral("/markets/%1/outcomes").arg(market.id)),
                            false,
                            [this, marketIndex](const QJsonDocument &outcomesDocument) {
                                if (!outcomesDocument.isArray()) {
                                    finishPendingMarketRequest();
                                    return;
                                }

                                auto &market = m_markets[marketIndex];
                                const QJsonArray outcomesArray = outcomesDocument.array();
                                const int fallbackPercent = fallbackPercentForOutcomeCount(outcomesArray.size());

                                market.outcomes.clear();
                                market.outcomes.reserve(outcomesArray.size());

                                for (const QJsonValue &outcomeValue : outcomesArray) {
                                    const QJsonObject outcomeObject = outcomeValue.toObject();

                                    ApiOutcome outcome;
                                    outcome.id = outcomeObject.value(QStringLiteral("id")).toString();
                                    outcome.title = outcomeObject.value(QStringLiteral("title")).toString();
                                    outcome.pricePercent = fallbackPercent;

                                    market.outcomes.push_back(outcome);
                                }

                                if (market.outcomes.isEmpty()) {
                                    finishPendingMarketRequest();
                                    return;
                                }

                                m_pendingMarketRequests += market.outcomes.size();

                                for (int outcomeIndex = 0; outcomeIndex < market.outcomes.size(); ++outcomeIndex) {
                                    const QString outcomeId = market.outcomes[outcomeIndex].id;

                                    getJson(apiUrl(QStringLiteral("/outcomes/%1/orderbook?depth=1").arg(outcomeId)),
                                            false,
                                            [this, marketIndex, outcomeIndex](const QJsonDocument &orderBookDocument) {
                                                if (orderBookDocument.isObject()) {
                                                    bool hasPrice = false;
                                                    const int price =
                                                        bestIndicativePrice(orderBookDocument.object(), &hasPrice);

                                                    if (hasPrice) {
                                                        auto &outcome = m_markets[marketIndex].outcomes[outcomeIndex];
                                                        outcome.pricePercent = price;
                                                        outcome.hasLivePrice = true;
                                                    }
                                                }

                                                finishPendingMarketRequest();
                                            },
                                            [this](int, const QString &) {
                                                finishPendingMarketRequest();
                                            });
                                }

                                finishPendingMarketRequest();
                            },
                            [this](int, const QString &) {
                                finishPendingMarketRequest();
                            });
                }
            },
            [this](int, const QString &message) {
                emit marketsError(QStringLiteral("Could not load markets from %1.\n%2")
                                      .arg(baseUrl(), message));
            });
}

void MarketApiClient::fetchWallet() {
    if (!isAuthenticated()) {
        emit walletError(QStringLiteral("Log in or Sign up to load your wallet."));
        return;
    }

    getJson(apiUrl(QStringLiteral("/wallet")),
            true,
            [this](const QJsonDocument &document) {
                if (!document.isObject()) {
                    emit walletError(QStringLiteral("/wallet did not return an object."));
                    return;
                }

                const QJsonObject object = document.object();

                ApiWallet wallet;
                wallet.userId = object.value(QStringLiteral("user_id")).toString();
                wallet.available = object.value(QStringLiteral("available")).toVariant().toLongLong();
                wallet.reserved = object.value(QStringLiteral("reserved")).toVariant().toLongLong();
                wallet.updatedAt = object.value(QStringLiteral("updated_at")).toString();

                emit walletReady(wallet);
            },
            [this](int statusCode, const QString &message) {
                if (statusCode == 401 && !m_refreshToken.trimmed().isEmpty()) {
                    refreshSession(
                        [this]() { fetchWallet(); },
                        [this](const QString &refreshMessage) {
                            emit walletError(QStringLiteral("Session expired.\n%1").arg(refreshMessage));
                        });
                    return;
                }

                emit walletError(QStringLiteral("Could not load wallet.\n%1").arg(message));
            });
}

void MarketApiClient::fetchMyTrades(int limit, int offset) {
    if (!isAuthenticated()) {
        emit tradesError(QStringLiteral("Log in or Sign up to load your trade history."));
        return;
    }

    if (limit <= 0) {
        limit = 50;
    }
    if (offset < 0) {
        offset = 0;
    }

    getJson(apiUrl(QStringLiteral("/me/trades?limit=%1&offset=%2").arg(limit).arg(offset)),
            true,
            [this](const QJsonDocument &document) {
                if (!document.isArray()) {
                    emit tradesError(QStringLiteral("/me/trades did not return an array."));
                    return;
                }

                QVector<ApiTrade> trades;
                const QJsonArray tradesArray = document.array();
                trades.reserve(tradesArray.size());

                for (const QJsonValue &tradeValue : tradesArray) {
                    if (!tradeValue.isObject()) {
                        continue;
                    }

                    const QJsonObject object = tradeValue.toObject();

                    ApiTrade trade;
                    trade.id = object.value(QStringLiteral("id")).toString();
                    trade.outcomeId = object.value(QStringLiteral("outcome_id")).toString();
                    trade.makerUserId = object.value(QStringLiteral("maker_user_id")).toString();
                    trade.takerUserId = object.value(QStringLiteral("taker_user_id")).toString();
                    trade.makerOrderId = object.value(QStringLiteral("maker_order_id")).toString();
                    trade.takerOrderId = object.value(QStringLiteral("taker_order_id")).toString();
                    trade.priceBasisPoints = object.value(QStringLiteral("price_bp")).toInt();
                    trade.quantityMicros = object.value(QStringLiteral("qty_micros")).toVariant().toLongLong();
                    trade.createdAt = object.value(QStringLiteral("created_at")).toString();

                    trades.push_back(trade);
                }

                emit tradesReady(trades);
            },
            [this](int statusCode, const QString &message) {
                if (statusCode == 401 && !m_refreshToken.trimmed().isEmpty()) {
                    refreshSession(
                        [this]() { fetchMyTrades(); },
                        [this](const QString &refreshMessage) {
                            emit tradesError(QStringLiteral("Session expired.\n%1").arg(refreshMessage));
                        });
                    return;
                }

                emit tradesError(QStringLiteral("Could not load trades.\n%1").arg(message));
            });
}