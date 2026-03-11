#pragma once

#include "../network/MarketApiClient.h"
#include <QMainWindow>

class HeaderBar;
class MarketsPage;
class Sidebar;
class QStackedWidget;
class ProfilePage;

class MarketWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MarketWindow(QWidget *parent = nullptr);

private slots:
    void showSection(const QString &pageId);
    void onWalletLoaded(const ApiWallet &wallet);
    void onWalletError(const QString &message);

private:
    HeaderBar *m_header = nullptr;
    Sidebar *m_sidebar = nullptr;
    MarketsPage *m_markets = nullptr;
    QWidget *m_portfolioPage = nullptr;
    QWidget *m_ordersPage = nullptr;
    QWidget *m_tradesPage = nullptr;
    ProfilePage *m_profile = nullptr;
    QStackedWidget *m_stack = nullptr;
    MarketApiClient *m_api = nullptr;

    QString m_profileName = QStringLiteral("Guest");
};