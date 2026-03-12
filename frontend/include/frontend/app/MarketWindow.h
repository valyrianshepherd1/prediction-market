#pragma once

#include "frontend/network/ApiTypes.h"

#include <QMainWindow>

class AuthRequiredPage;
class HeaderBar;
class MarketApiClient;
class MarketDetailsPage;
class MarketWsClient;
class MarketsPage;
class MarketsRepository;
class MarketDetailsRepository;
class OrdersPage;
class OrdersRepository;
class PortfolioPage;
class PortfolioRepository;
class ProfilePage;
class QStackedWidget;
class SessionStore;
class Sidebar;
class TradesPage;
class TradesRepository;
class QWidget;

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
    void scheduleUserRealtimeRefresh();

    HeaderBar *m_header = nullptr;
    Sidebar *m_sidebar = nullptr;

    MarketsPage *m_markets = nullptr;
    MarketDetailsPage *m_marketDetailsPage = nullptr;

    PortfolioPage *m_portfolioPage = nullptr;
    OrdersPage *m_ordersPage = nullptr;
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
    MarketApiClient *m_transport = nullptr;
    MarketWsClient *m_realtimeClient = nullptr;
    SessionStore *m_sessionStore = nullptr;
    MarketsRepository *m_marketsRepository = nullptr;
    MarketDetailsRepository *m_marketDetailsRepository = nullptr;
    PortfolioRepository *m_portfolioRepository = nullptr;
    OrdersRepository *m_ordersRepository = nullptr;
    TradesRepository *m_tradesRepository = nullptr;

    bool m_userRealtimeRefreshQueued = false;
    QString m_profileName = QStringLiteral("Guest");
};
