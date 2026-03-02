#pragma once
#include <QObject>
#include <QNetworkAccessManager>

class MarketApiClient : public QObject {
    Q_OBJECT
public:
    explicit MarketApiClient(QObject *parent = nullptr);

    void fetchMarkets();

    signals:
        void marketsReady(const QString &response);
    void error(const QString &message);

private slots:
    void onFinished(QNetworkReply *reply);

private:
    QNetworkAccessManager m_manager;
};