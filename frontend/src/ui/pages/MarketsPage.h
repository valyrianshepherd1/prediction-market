#pragma once
#include <QWidget>

class QLabel;
class MarketApiClient;

class MarketsPage : public QWidget {
    Q_OBJECT
public:
    explicit MarketsPage(MarketApiClient *api, QWidget *parent = nullptr);

    signals:
        void backHome();

private slots:
    void onMarkets(const QString &response);
    void onError(const QString &message);

private:
    MarketApiClient *m_api = nullptr;
    QLabel *m_label = nullptr;
};