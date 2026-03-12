#include "../../include/frontend/network/MarketApiClient.h"

#include "../../include/frontend/network/ApiParsers.h"

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

} // namespace

MarketApiClient::MarketApiClient(QObject *parent)
    : QObject(parent),
      m_baseUrl(QUrl(normalizeBaseUrl())) {
    loadPersistedSession();
}


void MarketApiClient::fetchMarketById(const QString &marketId,
                                      const std::function<void(const ApiMarket &)> &onSuccess,
                                      const std::function<void(const QString &)> &onError) {
    if (marketId.trimmed().isEmpty()) {
        if (onError) {
            onError(QStringLiteral("Market id is missing."));
        }
        return;
    }

    getJson(apiUrl(QStringLiteral("/markets/%1").arg(marketId)),
            false,
            [onSuccess, onError](const QJsonDocument &document) {
                if (!document.isObject()) {
                    if (onError) {
                        onError(QStringLiteral("/markets/{id} did not return an object."));
                    }
                    return;
                }

                const auto parsed = api::parse::marketBase(document.object());
                if (!parsed.has_value()) {
                    if (onError) {
                        onError(QStringLiteral("Invalid market payload from backend."));
                    }
                    return;
                }

                if (onSuccess) {
                    onSuccess(*parsed);
                }
            },
            [onError](int, const QString &message) {
                if (onError) {
                    onError(QStringLiteral("Could not load market.%1").arg(message));
                }
            });
}

void MarketApiClient::fetchOutcomesForMarket(const QString &marketId,
                                             const std::function<void(const QVector<ApiOutcome> &)> &onSuccess,
                                             const std::function<void(const QString &)> &onError) {
    if (marketId.trimmed().isEmpty()) {
        if (onError) {
            onError(QStringLiteral("Market id is missing."));
        }
        return;
    }

    getJson(apiUrl(QStringLiteral("/markets/%1/outcomes").arg(marketId)),
            false,
            [onSuccess, onError](const QJsonDocument &document) {
                if (!document.isArray()) {
                    if (onError) {
                        onError(QStringLiteral("/markets/{id}/outcomes did not return an array."));
                    }
                    return;
                }

                if (onSuccess) {
                    onSuccess(api::parse::outcomes(document.array()));
                }
            },
            [onError](int, const QString &message) {
                if (onError) {
                    onError(QStringLiteral("Could not load market outcomes.%1").arg(message));
                }
            });
}

void MarketApiClient::fetchOrderBookForOutcome(const QString &outcomeId,
                                               int depth,
                                               const std::function<void(const ApiOrderBook &)> &onSuccess,
                                               const std::function<void(const QString &)> &onError) {
    if (outcomeId.trimmed().isEmpty()) {
        if (onError) {
            onError(QStringLiteral("Outcome id is missing."));
        }
        return;
    }

    if (depth <= 0) {
        depth = 20;
    }

    getJson(apiUrl(QStringLiteral("/outcomes/%1/orderbook?depth=%2").arg(outcomeId).arg(depth)),
            false,
            [onSuccess, onError](const QJsonDocument &document) {
                if (!document.isObject()) {
                    if (onError) {
                        onError(QStringLiteral("/outcomes/{id}/orderbook did not return an object."));
                    }
                    return;
                }

                const auto parsed = api::parse::orderBook(document.object());
                if (!parsed.has_value()) {
                    if (onError) {
                        onError(QStringLiteral("Invalid order book payload from backend."));
                    }
                    return;
                }

                if (onSuccess) {
                    onSuccess(*parsed);
                }
            },
            [onError](int, const QString &message) {
                if (onError) {
                    onError(QStringLiteral("Could not load order book.%1").arg(message));
                }
            });
}

void MarketApiClient::fetchTradesForOutcome(const QString &outcomeId,
                                            int limit,
                                            int offset,
                                            const std::function<void(const QVector<ApiTrade> &)> &onSuccess,
                                            const std::function<void(const QString &)> &onError) {
    if (outcomeId.trimmed().isEmpty()) {
        if (onError) {
            onError(QStringLiteral("Outcome id is missing."));
        }
        return;
    }

    if (limit <= 0) {
        limit = 20;
    }
    if (offset < 0) {
        offset = 0;
    }

    getJson(apiUrl(QStringLiteral("/outcomes/%1/trades?limit=%2&offset=%3")
                       .arg(outcomeId)
                       .arg(limit)
                       .arg(offset)),
            false,
            [onSuccess, onError](const QJsonDocument &document) {
                if (!document.isArray()) {
                    if (onError) {
                        onError(QStringLiteral("/outcomes/{id}/trades did not return an array."));
                    }
                    return;
                }

                if (onSuccess) {
                    onSuccess(api::parse::trades(document.array()));
                }
            },
            [onError](int, const QString &message) {
                if (onError) {
                    onError(QStringLiteral("Could not load outcome trades.%1").arg(message));
                }
            });
}

QString MarketApiClient::baseUrl() const {
    return m_baseUrl.toString();
}

QUrl MarketApiClient::websocketUrl() const {
    QUrl url = m_baseUrl;

    if (url.scheme() == QStringLiteral("https")) {
        url.setScheme(QStringLiteral("wss"));
    } else if (url.scheme() == QStringLiteral("http")) {
        url.setScheme(QStringLiteral("ws"));
    } else if (url.scheme().trimmed().isEmpty()) {
        url.setScheme(QStringLiteral("wss"));
    }

    url.setPath(QStringLiteral("/ws"));
    url.setQuery(QString());
    return url;
}

QString MarketApiClient::accessToken() const {
    return m_accessToken;
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
    const QString accessToken = root.value(QStringLiteral("access_token")).toString();
    const QString refreshToken = root.value(QStringLiteral("refresh_token")).toString();

    if (accessToken.trimmed().isEmpty()) {
        return false;
    }

    const auto session = api::parse::sessionFromAuthObject(root);
    if (!session.has_value()) {
        return false;
    }

    m_accessToken = accessToken;
    if (!refreshToken.trimmed().isEmpty()) {
        m_refreshToken = refreshToken;
    }

    m_session = *session;
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

    const auto session = api::parse::sessionFromUserObject(document.object());
    if (!session.has_value()) {
        return false;
    }

    m_session = *session;
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

void MarketApiClient::finishPendingMarketRequest(const int requestGeneration) {
    if (requestGeneration != m_marketsRequestGeneration) {
        return;
    }

    if (m_pendingMarketRequests <= 0) {
        return;
    }

    --m_pendingMarketRequests;
    if (m_pendingMarketRequests == 0) {
        emit marketsReady(m_markets);
    }
}

void MarketApiClient::fetchMarkets() {
    const int requestGeneration = ++m_marketsRequestGeneration;
    m_markets.clear();
    m_pendingMarketRequests = 0;

    getJson(apiUrl(QStringLiteral("/markets?limit=100")),
            false,
            [this, requestGeneration](const QJsonDocument &document) {
                if (requestGeneration != m_marketsRequestGeneration) {
                    return;
                }

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
                    const auto parsedMarket = api::parse::marketBase(marketValue.toObject());
                    if (!parsedMarket.has_value()) {
                        finishPendingMarketRequest(requestGeneration);
                        continue;
                    }

                    const int marketIndex = m_markets.size();
                    m_markets.push_back(*parsedMarket);

                    getJson(apiUrl(QStringLiteral("/markets/%1/outcomes").arg(parsedMarket->id)),
                            false,
                            [this, requestGeneration, marketIndex](const QJsonDocument &outcomesDocument) {
                                if (requestGeneration != m_marketsRequestGeneration) {
                                    return;
                                }

                                if (!outcomesDocument.isArray()) {
                                    finishPendingMarketRequest(requestGeneration);
                                    return;
                                }

                                if (marketIndex < 0 || marketIndex >= m_markets.size()) {
                                    finishPendingMarketRequest(requestGeneration);
                                    return;
                                }

                                auto &market = m_markets[marketIndex];
                                market.outcomes = api::parse::outcomes(outcomesDocument.array());
                                const int fallbackPercent =
                                    fallbackPercentForOutcomeCount(market.outcomes.size());

                                for (ApiOutcome &outcome : market.outcomes) {
                                    outcome.pricePercent = fallbackPercent;
                                }

                                if (market.outcomes.isEmpty()) {
                                    finishPendingMarketRequest(requestGeneration);
                                    return;
                                }

                                m_pendingMarketRequests += market.outcomes.size();

                                for (int outcomeIndex = 0; outcomeIndex < market.outcomes.size(); ++outcomeIndex) {
                                    const QString outcomeId = market.outcomes[outcomeIndex].id;

                                    getJson(apiUrl(QStringLiteral("/outcomes/%1/orderbook?depth=1").arg(outcomeId)),
                                            false,
                                            [this, requestGeneration, marketIndex, outcomeIndex](const QJsonDocument &orderBookDocument) {
                                                if (requestGeneration != m_marketsRequestGeneration) {
                                                    return;
                                                }

                                                if (marketIndex >= 0 && marketIndex < m_markets.size()) {
                                                    auto &outcomes = m_markets[marketIndex].outcomes;
                                                    if (outcomeIndex >= 0 && outcomeIndex < outcomes.size() && orderBookDocument.isObject()) {
                                                        bool hasPrice = false;
                                                        const int price =
                                                            bestIndicativePrice(orderBookDocument.object(), &hasPrice);

                                                        if (hasPrice) {
                                                            auto &outcome = outcomes[outcomeIndex];
                                                            outcome.pricePercent = price;
                                                            outcome.hasLivePrice = true;
                                                        }
                                                    }
                                                }

                                                finishPendingMarketRequest(requestGeneration);
                                            },
                                            [this, requestGeneration](int, const QString &) {
                                                finishPendingMarketRequest(requestGeneration);
                                            });
                                }

                                finishPendingMarketRequest(requestGeneration);
                            },
                            [this, requestGeneration](int, const QString &) {
                                finishPendingMarketRequest(requestGeneration);
                            });
                }
            },
            [this, requestGeneration](int, const QString &message) {
                if (requestGeneration != m_marketsRequestGeneration) {
                    return;
                }
                emit marketsError(QStringLiteral("Could not load markets from %1.%2").arg(baseUrl(), message));
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

                const auto walletValue = api::parse::wallet(document.object());
                if (!walletValue.has_value()) {
                    emit walletError(QStringLiteral("Invalid wallet payload from backend."));
                    return;
                }

                emit walletReady(*walletValue);
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

                emit tradesReady(api::parse::trades(document.array()));
            },
            [this, limit, offset](int statusCode, const QString &message) {
                if (statusCode == 401 && !m_refreshToken.trimmed().isEmpty()) {
                    refreshSession(
                        [this, limit, offset]() { fetchMyTrades(limit, offset); },
                        [this](const QString &refreshMessage) {
                            emit tradesError(QStringLiteral("Session expired.\n%1").arg(refreshMessage));
                        });
                    return;
                }

                emit tradesError(QStringLiteral("Could not load trades.\n%1").arg(message));
            });
}

void MarketApiClient::fetchMyOrders(int limit, int offset, const QString &status) {
    if (!isAuthenticated()) {
        emit ordersError(QStringLiteral("Log in or Sign up to load your orders."));
        return;
    }

    if (limit <= 0) {
        limit = 100;
    }
    if (offset < 0) {
        offset = 0;
    }

    QString path = QStringLiteral("/me/orders?limit=%1&offset=%2").arg(limit).arg(offset);
    if (!status.trimmed().isEmpty()) {
        path += QStringLiteral("&status=%1").arg(QString::fromUtf8(QUrl::toPercentEncoding(status.trimmed())));
    }

    getJson(apiUrl(path),
            true,
            [this](const QJsonDocument &document) {
                if (!document.isArray()) {
                    emit ordersError(QStringLiteral("/me/orders did not return an array."));
                    return;
                }

                emit ordersReady(api::parse::orders(document.array()));
            },
            [this, limit, offset, status](int statusCode, const QString &message) {
                if (statusCode == 401 && !m_refreshToken.trimmed().isEmpty()) {
                    refreshSession(
                        [this, limit, offset, status]() { fetchMyOrders(limit, offset, status); },
                        [this](const QString &refreshMessage) {
                            emit ordersError(QStringLiteral("Session expired.\n%1").arg(refreshMessage));
                        });
                    return;
                }

                emit ordersError(QStringLiteral("Could not load orders.\n%1").arg(message));
            });
}

void MarketApiClient::fetchPortfolio(int limit, int offset) {
    if (!isAuthenticated()) {
        emit portfolioError(QStringLiteral("Log in or Sign up to load your portfolio."));
        return;
    }

    if (limit <= 0) {
        limit = 100;
    }
    if (offset < 0) {
        offset = 0;
    }

    getJson(apiUrl(QStringLiteral("/portfolio?limit=%1&offset=%2").arg(limit).arg(offset)),
            true,
            [this](const QJsonDocument &document) {
                if (!document.isArray()) {
                    emit portfolioError(QStringLiteral("/portfolio did not return an array."));
                    return;
                }

                emit portfolioReady(api::parse::portfolioPositions(document.array()));
            },
            [this, limit, offset](int statusCode, const QString &message) {
                if (statusCode == 401 && !m_refreshToken.trimmed().isEmpty()) {
                    refreshSession(
                        [this, limit, offset]() { fetchPortfolio(limit, offset); },
                        [this](const QString &refreshMessage) {
                            emit portfolioError(QStringLiteral("Session expired.\n%1").arg(refreshMessage));
                        });
                    return;
                }

                emit portfolioError(QStringLiteral("Could not load portfolio.\n%1").arg(message));
            });
}

void MarketApiClient::fetchLedger(int limit, int offset) {
    if (!isAuthenticated()) {
        emit ledgerError(QStringLiteral("Log in or Sign up to load your ledger."));
        return;
    }

    if (limit <= 0) {
        limit = 100;
    }
    if (offset < 0) {
        offset = 0;
    }

    getJson(apiUrl(QStringLiteral("/ledger?limit=%1&offset=%2").arg(limit).arg(offset)),
            true,
            [this](const QJsonDocument &document) {
                if (!document.isArray()) {
                    emit ledgerError(QStringLiteral("/ledger did not return an array."));
                    return;
                }

                emit ledgerReady(api::parse::portfolioLedgerEntries(document.array()));
            },
            [this, limit, offset](int statusCode, const QString &message) {
                if (statusCode == 401 && !m_refreshToken.trimmed().isEmpty()) {
                    refreshSession(
                        [this, limit, offset]() { fetchLedger(limit, offset); },
                        [this](const QString &refreshMessage) {
                            emit ledgerError(QStringLiteral("Session expired.\n%1").arg(refreshMessage));
                        });
                    return;
                }

                emit ledgerError(QStringLiteral("Could not load ledger.\n%1").arg(message));
            });
}

void MarketApiClient::adminDepositToCurrentUser(qint64 amount) {
    if (!isAuthenticated()) {
        emit depositError(QStringLiteral("Log in first."));
        return;
    }

    if (m_session.userId.trimmed().isEmpty()) {
        emit depositError(QStringLiteral("Current user id is missing."));
        return;
    }

    if (amount <= 0) {
        emit depositError(QStringLiteral("Amount must be greater than zero."));
        return;
    }

    emit depositBusyChanged(true);

    QJsonObject body;
    body.insert(QStringLiteral("user_id"), m_session.userId);
    body.insert(QStringLiteral("amount"), QJsonValue(static_cast<double>(amount)));

    postJson(apiUrl(QStringLiteral("/admin/deposit")),
             body,
             true,
             [this](const QJsonDocument &document) {
                 emit depositBusyChanged(false);

                 if (!document.isObject()) {
                     emit depositError(QStringLiteral("/admin/deposit did not return an object."));
                     return;
                 }

                 const auto walletValue = api::parse::wallet(document.object());
                 if (!walletValue.has_value()) {
                     emit depositError(QStringLiteral("Invalid deposit response from backend."));
                     return;
                 }

                 emit depositSucceeded(*walletValue);
             },
             [this](int statusCode, const QString &message) {
                 emit depositBusyChanged(false);

                 if (statusCode == 401 || statusCode == 403) {
                     emit depositError(QStringLiteral(
                         "Deposit is currently admin-only in the backend."));
                     return;
                 }

                 emit depositError(QStringLiteral("Could not add money.\n%1").arg(message));
             });
}

void MarketApiClient::createOrder(const QString &outcomeId,
                                  const QString &side,
                                  int priceBasisPoints,
                                  qint64 quantityMicros) {
    if (!isAuthenticated()) {
        emit orderError(QStringLiteral("Log in or Sign up to place an order."));
        return;
    }

    const QString normalizedSide = side.trimmed().toUpper();

    if (outcomeId.trimmed().isEmpty()) {
        emit orderError(QStringLiteral("No outcome is selected."));
        return;
    }

    if (normalizedSide != QStringLiteral("BUY") &&
        normalizedSide != QStringLiteral("SELL")) {
        emit orderError(QStringLiteral("Order side must be BUY or SELL."));
        return;
    }

    if (priceBasisPoints <= 0 || priceBasisPoints >= 10000) {
        emit orderError(QStringLiteral("Price must be between 0.01% and 99.99%."));
        return;
    }

    if (quantityMicros <= 0) {
        emit orderError(QStringLiteral("Quantity must be greater than zero."));
        return;
    }

    emit orderBusyChanged(true);

    QJsonObject body;
    body.insert(QStringLiteral("outcome_id"), outcomeId);
    body.insert(QStringLiteral("side"), normalizedSide);
    body.insert(QStringLiteral("price_bp"), priceBasisPoints);
    body.insert(QStringLiteral("qty_micros"), QJsonValue(static_cast<double>(quantityMicros)));

    postJson(apiUrl(QStringLiteral("/orders")),
             body,
             true,
             [this](const QJsonDocument &document) {
                 emit orderBusyChanged(false);

                 if (!document.isObject()) {
                     emit orderError(QStringLiteral("/orders did not return an object."));
                     return;
                 }

                 const auto orderValue = api::parse::order(document.object());
                 if (!orderValue.has_value()) {
                     emit orderError(QStringLiteral("Invalid order payload from backend."));
                     return;
                 }

                 emit orderCreated(*orderValue);
             },
             [this](int, const QString &message) {
                 emit orderBusyChanged(false);
                 emit orderError(QStringLiteral("Could not place order.\n%1").arg(message));
             });
}

void MarketApiClient::cancelOrder(const QString &orderId) {
    if (!isAuthenticated()) {
        emit orderCancelError(QStringLiteral("Log in or Sign up to cancel orders."));
        return;
    }

    const QString normalizedId = orderId.trimmed();
    if (normalizedId.isEmpty()) {
        emit orderCancelError(QStringLiteral("Order id is missing."));
        return;
    }

    emit orderCancelBusyChanged(true);

    QNetworkRequest request(apiUrl(QStringLiteral("/orders/%1").arg(normalizedId)));
    addCommonHeaders(request, true);

    auto *reply = m_manager.deleteResource(request);
    ignoreSslErrorsIfNeeded(reply);

    QObject::connect(reply, &QNetworkReply::finished, this, [this, reply, normalizedId]() {
        const int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const QByteArray payload = reply->readAll();
        const QString errorMessage = responseErrorMessage(reply, payload);
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            emit orderCancelBusyChanged(false);

            if (statusCode == 401 && !m_refreshToken.trimmed().isEmpty()) {
                refreshSession(
                    [this, normalizedId]() { cancelOrder(normalizedId); },
                    [this](const QString &refreshMessage) {
                        emit orderCancelError(QStringLiteral("Session expired.\n%1").arg(refreshMessage));
                    });
                return;
            }

            emit orderCancelError(QStringLiteral("Could not cancel order.\n%1").arg(errorMessage));
            return;
        }

        QJsonParseError parseError{};
        const QJsonDocument document = QJsonDocument::fromJson(payload, &parseError);
        emit orderCancelBusyChanged(false);

        if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
            emit orderCancelError(QStringLiteral("Invalid cancel response from backend."));
            return;
        }

        const auto orderValue = api::parse::order(document.object());
        if (!orderValue.has_value()) {
            emit orderCancelError(QStringLiteral("Invalid canceled order payload from backend."));
            return;
        }

        emit orderCanceled(*orderValue);
    });
}
