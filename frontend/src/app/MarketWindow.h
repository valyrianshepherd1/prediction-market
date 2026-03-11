#pragma once

#include "../network/MarketApiClient.h"

#include <QMainWindow>

class AuthRequiredPage;
class HeaderBar;
class MarketsPage;
class ProfilePage;
class Sidebar;
class TradesPage;
class QStackedWidget;
class QWidget;
class MarketDetailsPage;

class MarketWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MarketWindow(QWidget *parent = nullptr);

private slots:
    void showSection(const QString &pageId);
    void onWalletLoaded(const ApiWallet &wallet);
    void onWalletError(const QString &message);

    void onSessionReady(const ApiSession &session);
    void onSessionCleared();
    void onAuthError(const QString &message);

    void openLoginDialog();
    void openSignUpDialog();

private:
    void openAuthDialog(bool startOnSignUp);
    void setProtectedPagesAuthenticated(bool authenticated);

    HeaderBar *m_header = nullptr;
    Sidebar *m_sidebar = nullptr;

    MarketsPage *m_markets = nullptr;
    MarketDetailsPage *m_marketDetailsPage = nullptr;

    QWidget *m_portfolioPage = nullptr;
    QWidget *m_ordersPage = nullptr;
    TradesPage *m_tradesPage = nullptr;
    ProfilePage *m_profile = nullptr;

    AuthRequiredPage *m_portfolioGate = nullptr;
    AuthRequiredPage *m_ordersGate = nullptr;
    AuthRequiredPage *m_tradesGate = nullptr;
    AuthRequiredPage *m_profileGate = nullptr;

    QStackedWidget *m_portfolioStack = nullptr;
    QStackedWidget *m_ordersStack = nullptr;
    QStackedWidget *m_tradesStack = nullptr;
    QStackedWidget *m_profileStack = nullptr;

    QStackedWidget *m_stack = nullptr;
    MarketApiClient *m_api = nullptr;

    QString m_profileName = QStringLiteral("Guest");
};