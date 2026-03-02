#include "MarketApiClient.h"
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>

MarketApiClient::MarketApiClient(QObject *parent)
    : QObject(parent)
{
    connect(&m_manager, &QNetworkAccessManager::finished,
            this, &MarketApiClient::onFinished);
}

void MarketApiClient::fetchMarkets() {
    QNetworkRequest request(QUrl("http://localhost:8000/markets"));
    m_manager.get(request);
}

void MarketApiClient::onFinished(QNetworkReply *reply) {
    if (!reply) return;

    if (reply->error() == QNetworkReply::NoError) {
        const QString response = QString::fromUtf8(reply->readAll());
        emit marketsReady(response);
    } else {
        emit error(reply->errorString());
    }

    reply->deleteLater();
}