#pragma once

#include "../network/MarketApiClient.h"
#include <QMainWindow>

class HeaderBar;
class MarketsPage;
class QStackedWidget;
class ProfilePage;

class MarketWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MarketWindow(QWidget *parent = nullptr);

private slots:
    void openProfile();
    void showMarkets();
    void onWalletLoaded(const ApiWallet &wallet);
    void onWalletError(const QString &message);

private:
    HeaderBar *m_header = nullptr;
    MarketsPage *m_markets = nullptr;
    QStackedWidget *m_stack = nullptr;
    ProfilePage *m_profile = nullptr;
    MarketApiClient *m_api = nullptr;

    QString m_profileName = QStringLiteral("Guest");
};