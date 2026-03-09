#include "MarketApiClient.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSslError>
#include <QUrlQuery>

#include <functional>

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
} // namespace

MarketApiClient::MarketApiClient(QObject *parent)
    : QObject(parent),
      m_baseUrl(QUrl(normalizeBaseUrl())),
      m_userId(qEnvironmentVariable("PM_FRONTEND_USER_ID")) {}

QString MarketApiClient::baseUrl() const {
    return m_baseUrl.toString();
}

QString MarketApiClient::configuredUserId() const {
    return m_userId;
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

void MarketApiClient::ignoreSslErrorsIfNeeded(QNetworkReply *reply) const {
    if (!reply) {
        return;
    }

    const QUrl url = reply->url();
    const bool isLocalhost = url.host() == QStringLiteral("localhost") ||
                             url.host() == QStringLiteral("127.0.0.1");
    if (url.scheme() == QStringLiteral("https") && isLocalhost) {
        QObject::connect(reply, &QNetworkReply::sslErrors, reply,
                         [reply](const QList<QSslError> &) { reply->ignoreSslErrors(); });
        reply->ignoreSslErrors();
    }
}

void MarketApiClient::getJson(const QUrl &url,
                              const std::function<void(const QJsonDocument &)> &onSuccess,
                              const std::function<void(const QString &)> &onError) {
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    request.setRawHeader("Accept", "application/json");
    if (!m_userId.isEmpty()) {
        request.setRawHeader("X-User-Id", m_userId.toUtf8());
    }

    QNetworkReply *reply = m_manager.get(request);
    ignoreSslErrorsIfNeeded(reply);

    connect(reply, &QNetworkReply::finished, this, [reply, onSuccess, onError]() {
        const QByteArray payload = reply->readAll();
        if (reply->error() != QNetworkReply::NoError) {
            if (onError) {
                const int statusCode =
                    reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
                QString message;
                if (!payload.trimmed().isEmpty()) {
                    message = QStringLiteral("HTTP %1: %2")
                                  .arg(statusCode > 0 ? QString::number(statusCode)
                                                      : QStringLiteral("error"))
                                  .arg(QString::fromUtf8(payload).trimmed());
                } else {
                    message = reply->errorString();
                }
                onError(message);
            }
            reply->deleteLater();
            return;
        }

        QJsonParseError parseError;
        const QJsonDocument document = QJsonDocument::fromJson(payload, &parseError);
        if (parseError.error != QJsonParseError::NoError) {
            if (onError) {
                onError(QStringLiteral("Invalid JSON from %1: %2")
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
                            [this, marketIndex](const QJsonDocument &outcomesDocument) {
                                if (!outcomesDocument.isArray()) {
                                    finishPendingMarketRequest();
                                    return;
                                }

                                auto &market = m_markets[marketIndex];
                                const QJsonArray outcomesArray = outcomesDocument.array();
                                const int fallbackPercent =
                                    fallbackPercentForOutcomeCount(outcomesArray.size());

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
                                for (int outcomeIndex = 0; outcomeIndex < market.outcomes.size();
                                     ++outcomeIndex) {
                                    const QString outcomeId = market.outcomes[outcomeIndex].id;
                                    getJson(apiUrl(QStringLiteral("/outcomes/%1/orderbook?depth=1")
                                                       .arg(outcomeId)),
                                            [this, marketIndex,
                                             outcomeIndex](const QJsonDocument &orderBookDocument) {
                                                if (orderBookDocument.isObject()) {
                                                    bool hasPrice = false;
                                                    const int price = bestIndicativePrice(
                                                        orderBookDocument.object(), &hasPrice);
                                                    if (hasPrice) {
                                                        auto &outcome =
                                                            m_markets[marketIndex].outcomes[outcomeIndex];
                                                        outcome.pricePercent = price;
                                                        outcome.hasLivePrice = true;
                                                    }
                                                }
                                                finishPendingMarketRequest();
                                            },
                                            [this](const QString &) { finishPendingMarketRequest(); });
                                }

                                finishPendingMarketRequest();
                            },
                            [this](const QString &) { finishPendingMarketRequest(); });
                }
            },
            [this](const QString &message) {
                emit marketsError(QStringLiteral("Could not load markets from %1. %2")
                                      .arg(baseUrl(), message));
            });
}

void MarketApiClient::fetchWallet() {
    if (m_userId.trimmed().isEmpty()) {
        emit walletError(QStringLiteral(
            "Set PM_FRONTEND_USER_ID before starting the Qt app to load a wallet."));
        return;
    }

    getJson(apiUrl(QStringLiteral("/wallets/%1").arg(m_userId)),
            [this](const QJsonDocument &document) {
                if (!document.isObject()) {
                    emit walletError(QStringLiteral("/wallets/{userId} did not return an object."));
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
            [this](const QString &message) {
                emit walletError(QStringLiteral("Could not load wallet for %1. %2")
                                     .arg(m_userId, message));
            });
}