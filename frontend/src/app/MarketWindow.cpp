#include "frontend/app/MarketWindow.h"

#include "frontend/network/MarketApiClient.h"
#include "frontend/network/MarketWsClient.h"
#include "frontend/repositories/MarketDetailsRepository.h"
#include "frontend/repositories/MarketsRepository.h"
#include "frontend/repositories/OrdersRepository.h"
#include "frontend/repositories/PortfolioRepository.h"
#include "frontend/repositories/TradesRepository.h"
#include "frontend/state/SessionStore.h"
#include "frontend/ui/HeaderBar.h"
#include "frontend/ui/Sidebar.h"
#include "frontend/ui/dialogs/AuthDialog.h"
#include "frontend/ui/pages/AuthRequiredPage.h"
#include "frontend/ui/pages/MarketDetailsPage.h"
#include "frontend/ui/pages/MarketsPage.h"
#include "frontend/ui/pages/OrdersPage.h"
#include "frontend/ui/pages/PortfolioPage.h"
#include "frontend/ui/pages/ProfilePage.h"
#include "frontend/ui/pages/TradesPage.h"

#include <QApplication>
#include <QDateTime>
#include <QHBoxLayout>
#include <QLabel>
#include <QLocale>
#include <QStackedWidget>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

namespace {

QString darkStyle() {
    return R"(
        QWidget { background: #0b0f14; color: #e8eef5; font-size: 14px; }
        #Sidebar { background: #0d141d; border: 1px solid #1b2430; border-radius: 16px; }
        #SidebarLogo { font-size: 24px; font-weight: 700; color: #d8e4f0; padding: 10px 6px; }
        #Sidebar QPushButton { text-align: left; background: transparent; border: 1px solid transparent; border-radius: 10px; padding: 10px 12px; color: #c4d2e0; font-size: 15px; }
        #Sidebar QPushButton:hover { background: #141d29; border-color: #1b2430; }
        #Sidebar QPushButton:checked { background: #182331; border-color: #2a394c; color: white; font-weight: 600; }
        #SidebarStatus { color: #9fb0c3; font-size: 13px; padding-top: 6px; }

        #HeaderBar { background: #0d141d; border: 1px solid #1b2430; border-radius: 16px; }
        #HeaderTitle { font-size: 22px; font-weight: 700; color: white; }
        #HeaderBalance { color: #c8d7e6; font-size: 15px; padding-right: 4px; }
        #HeaderAvatar { background: #1d7dfa; color: white; border: none; border-radius: 20px; font-size: 15px; font-weight: 700; padding: 0; }
        #HeaderAvatar:hover { background: #3c90fb; }

        #CenterPage { background: #0d141d; border: 1px solid #1b2430; border-radius: 18px; }
        #CenterPageTitle { font-size: 28px; font-weight: 700; color: white; }
        #CenterPageText { color: #9fb0c3; font-size: 15px; }

        QPushButton { background: #141c26; border: 1px solid #1b2430; padding: 8px 12px; border-radius: 10px; }
        QPushButton:hover { background: #182131; }

        #MarketCard { background: #101722; border: 1px solid #1b2430; border-radius: 14px; }
        #MarketQuestion { font-size: 15px; font-weight: 600; }
        #MarketVolume { color: #9fb0c3; font-size: 12px; }

        QProgressBar { border: none; background: #0b0f14; border-radius: 5px; }
        QProgressBar::chunk { background: #20c997; border-radius: 5px; }

        #OutcomePct { color: #9fb0c3; }
    )";
}

QString initialsFromName(const QString &name) {
    const QString trimmed = name.trimmed();
    if (trimmed.isEmpty()) {
        return QStringLiteral("U");
    }

    const QStringList parts = trimmed.split(' ', Qt::SkipEmptyParts);
    if (parts.size() >= 2) {
        return QString(parts[0].front()).append(parts[1].front()).toUpper();
    }

    return QString(trimmed.front()).toUpper();
}

QString formatUnits(qint64 value) {
    return QLocale().toString(value);
}

QStackedWidget *createProtectedStack(QWidget *gate, QWidget *content, QWidget *parent = nullptr) {
    auto *stack = new QStackedWidget(parent);
    stack->addWidget(gate);
    stack->addWidget(content);
    stack->setCurrentIndex(0);
    return stack;
}

QString formatDateTimeReadable(const QString &value) {
    const QDateTime dt = QDateTime::fromString(value, Qt::ISODate);
    if (!dt.isValid()) {
        return value;
    }
    return dt.toLocalTime().toString(QStringLiteral("dd MMM yyyy, HH:mm"));
}

} // namespace

MarketWindow::MarketWindow(QWidget *parent)
    : QMainWindow(parent) {
    setWindowFlags(Qt::Window |
                   Qt::CustomizeWindowHint |
                   Qt::WindowMinimizeButtonHint |
                   Qt::WindowMaximizeButtonHint |
                   Qt::WindowCloseButtonHint);
    setWindowTitle(QStringLiteral("PreDgict"));

    auto *central = new QWidget(this);
    auto *root = new QVBoxLayout(central);
    root->setContentsMargins(12, 12, 12, 12);
    root->setSpacing(12);

    auto *body = new QWidget(this);
    auto *bodyLayout = new QHBoxLayout(body);
    bodyLayout->setContentsMargins(0, 0, 0, 0);
    bodyLayout->setSpacing(12);

    auto *rightSide = new QWidget(this);
    auto *rightLayout = new QVBoxLayout(rightSide);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(12);

    m_sidebar = new Sidebar(this);
    m_sidebar->setFixedWidth(220);

    m_transport = new MarketApiClient(this);
    m_realtimeClient = new MarketWsClient(m_transport, this);
    m_sessionStore = new SessionStore(m_transport, this);
    m_marketsRepository = new MarketsRepository(m_transport, this);
    m_marketDetailsRepository = new MarketDetailsRepository(m_transport, this);
    m_portfolioRepository = new PortfolioRepository(m_transport, this);
    m_ordersRepository = new OrdersRepository(m_transport, this);
    m_tradesRepository = new TradesRepository(m_transport, this);

    m_marketsRepository->setRealtimeClient(m_realtimeClient);
    m_marketDetailsRepository->setRealtimeClient(m_realtimeClient);

    m_header = new HeaderBar(this);
    m_markets = new MarketsPage(this);
    m_profile = new ProfilePage(this);
    m_tradesPage = new TradesPage(this);
    m_portfolioPage = new PortfolioPage(this);
    m_ordersPage = new OrdersPage(this);

    m_marketDetailsPage = new MarketDetailsPage(this);
    m_marketDetailsPage->setSessionStore(m_sessionStore);
    m_marketDetailsPage->setOrdersRepository(m_ordersRepository);
    m_marketDetailsPage->setMarketDetailsRepository(m_marketDetailsRepository);
    m_marketDetailsPage->setObjectName(QStringLiteral("CenterPage"));

    m_portfolioGate = new AuthRequiredPage(
        QStringLiteral("View your portfolio"),
        this);

    m_ordersGate = new AuthRequiredPage(
        QStringLiteral("View your orders"),
        this);

    m_tradesGate = new AuthRequiredPage(
        QStringLiteral("View your trades"),
        this);

    m_profileGate = new AuthRequiredPage(
        QStringLiteral("View your profile"),
        this);

    m_markets->setObjectName(QStringLiteral("CenterPage"));
    m_portfolioPage->setObjectName(QStringLiteral("CenterPage"));
    m_ordersPage->setObjectName(QStringLiteral("CenterPage"));
    m_profile->setObjectName(QStringLiteral("CenterPage"));
    m_tradesPage->setObjectName(QStringLiteral("CenterPage"));

    m_portfolioStack = createProtectedStack(m_portfolioGate, m_portfolioPage, this);
    m_ordersStack = createProtectedStack(m_ordersGate, m_ordersPage, this);
    m_tradesStack = createProtectedStack(m_tradesGate, m_tradesPage, this);
    m_profileStack = createProtectedStack(m_profileGate, m_profile, this);

    m_stack = new QStackedWidget(this);
    m_stack->addWidget(m_markets);
    m_stack->addWidget(m_portfolioStack);
    m_stack->addWidget(m_ordersStack);
    m_stack->addWidget(m_tradesStack);
    m_stack->addWidget(m_profileStack);
    m_stack->addWidget(m_marketDetailsPage);

    rightLayout->addWidget(m_header);
    rightLayout->addWidget(m_stack, 1);

    bodyLayout->addWidget(m_sidebar);
    bodyLayout->addWidget(rightSide, 1);

    root->addWidget(body, 1);
    setCentralWidget(central);

    qApp->setStyleSheet(darkStyle());

    m_profile->setUserName(m_profileName);
    m_header->setAvatarText(initialsFromName(m_profileName));
    m_header->setBalanceText(QStringLiteral("Balance: —"));

    connect(m_sidebar, &Sidebar::pageRequested, this, &MarketWindow::showSection);
    connect(m_header, &HeaderBar::profileClicked, this, [this]() {
        showSection(QStringLiteral("profile"));
    });
    connect(m_profile, &ProfilePage::backRequested, this, [this]() {
        showSection(QStringLiteral("markets"));
    });
    connect(m_profile, &ProfilePage::logoutRequested, m_sessionStore, &SessionStore::logout);

    connect(m_markets, &MarketsPage::marketRequested, this,
        [this](const ApiMarket &market, const QString &preferredSide) {
            m_marketDetailsPage->setMarket(market, preferredSide);
            m_stack->setCurrentWidget(m_marketDetailsPage);
            m_header->setTitle(QStringLiteral("Market"));
            m_sidebar->setCurrentPage(QStringLiteral("markets"));
        });

    connect(m_marketDetailsPage, &MarketDetailsPage::backRequested, this, [this]() {
        showSection(QStringLiteral("markets"));
    });

    connect(m_marketDetailsPage, &MarketDetailsPage::loginRequested, this, [this]() {
        showSection(QStringLiteral("profile"));
    });

    connect(m_portfolioGate, &AuthRequiredPage::loginRequested, this, &MarketWindow::openLoginDialog);
    connect(m_portfolioGate, &AuthRequiredPage::signUpRequested, this, &MarketWindow::openSignUpDialog);
    connect(m_ordersGate, &AuthRequiredPage::loginRequested, this, &MarketWindow::openLoginDialog);
    connect(m_ordersGate, &AuthRequiredPage::signUpRequested, this, &MarketWindow::openSignUpDialog);
    connect(m_tradesGate, &AuthRequiredPage::loginRequested, this, &MarketWindow::openLoginDialog);
    connect(m_tradesGate, &AuthRequiredPage::signUpRequested, this, &MarketWindow::openSignUpDialog);
    connect(m_profileGate, &AuthRequiredPage::loginRequested, this, &MarketWindow::openLoginDialog);
    connect(m_profileGate, &AuthRequiredPage::signUpRequested, this, &MarketWindow::openSignUpDialog);

    connect(m_marketsRepository, &MarketsRepository::marketsReady, this, [this](const QVector<ApiMarket> &markets) {
        m_markets->setMarkets(markets);
        m_portfolioPage->setMarkets(markets);
        m_ordersPage->setMarkets(markets);
    });
    connect(m_marketsRepository, &MarketsRepository::marketsError, m_markets, &MarketsPage::setError);

    connect(m_portfolioRepository, &PortfolioRepository::walletReady, this, &MarketWindow::onWalletLoaded);
    connect(m_portfolioRepository, &PortfolioRepository::walletError, this, &MarketWindow::onWalletError);
    connect(m_portfolioRepository, &PortfolioRepository::portfolioReady, m_portfolioPage, &PortfolioPage::setPositions);
    connect(m_portfolioRepository, &PortfolioRepository::portfolioError, m_portfolioPage, &PortfolioPage::setError);

    connect(m_ordersRepository, &OrdersRepository::ordersReady, m_ordersPage, &OrdersPage::setOrders);
    connect(m_ordersRepository, &OrdersRepository::ordersError, m_ordersPage, &OrdersPage::setError);
    connect(m_ordersRepository, &OrdersRepository::orderCancelBusyChanged, m_ordersPage, &OrdersPage::setCancelBusy);
    connect(m_ordersRepository, &OrdersRepository::orderCancelError, m_ordersPage, &OrdersPage::setCancelError);
    connect(m_ordersRepository, &OrdersRepository::orderCanceled, this, [this](const ApiOrder &) {
        if (!m_sessionStore->isAuthenticated()) {
            return;
        }
        m_portfolioRepository->refreshWallet();
        m_portfolioRepository->refreshPortfolio();
    });

    connect(m_ordersPage, &OrdersPage::refreshRequested, this, [this](const QString &statusFilter) {
        if (!m_sessionStore->isAuthenticated()) {
            return;
        }
        m_ordersPage->setLoading(QStringLiteral("Loading orders from backend..."));
        m_ordersRepository->refreshMyOrders(100, 0, statusFilter);
    });
    connect(m_ordersPage, &OrdersPage::cancelRequested, this, [this](const QString &orderId) {
        if (!m_sessionStore->isAuthenticated()) {
            return;
        }
        m_ordersRepository->cancelOrder(orderId);
    });

    connect(m_tradesRepository, &TradesRepository::tradesReady, m_tradesPage, &TradesPage::setTrades);
    connect(m_tradesRepository, &TradesRepository::tradesError, m_tradesPage, &TradesPage::setError);

    connect(m_sessionStore, &SessionStore::sessionReady, this, &MarketWindow::onSessionReady);
    connect(m_sessionStore, &SessionStore::sessionCleared, this, &MarketWindow::onSessionCleared);
    connect(m_sessionStore, &SessionStore::authError, this, &MarketWindow::onAuthError);

    connect(m_realtimeClient, &MarketWsClient::topicMessageReceived, this,
            [this](const QString &topic, const QString &event, const QJsonObject &, quint64, bool) {
                if (!m_sessionStore->isAuthenticated()) {
                    return;
                }

                if (!topic.startsWith(QStringLiteral("user:"))) {
                    return;
                }

                if (event == QStringLiteral("user.wallet.updated") ||
                    event == QStringLiteral("user.position.updated") ||
                    event == QStringLiteral("order.changed") ||
                    event == QStringLiteral("user.trade.created") ||
                    event == QStringLiteral("user.snapshot")) {
                    scheduleUserRealtimeRefresh();
                }
            });

    connect(m_ordersRepository, &OrdersRepository::orderCreated, this, [this](const ApiOrder &) {
        if (!m_sessionStore->isAuthenticated()) {
            return;
        }

        m_portfolioRepository->refreshWallet();
        m_ordersRepository->refreshMyOrders(100, 0, m_ordersPage->selectedStatusFilter());
        m_portfolioRepository->refreshPortfolio();

        if (m_stack->currentWidget() == m_tradesStack) {
            m_tradesPage->setLoading(QStringLiteral("Refreshing your trades..."));
            m_tradesRepository->refreshMyTrades();
        }
    });

    setProtectedPagesAuthenticated(false);

    m_realtimeClient->start();

    m_sidebar->setStatusText(QStringLiteral("Status: loading markets..."));
    m_markets->setLoading(QStringLiteral("Loading markets from %1…").arg(m_transport->baseUrl()));
    m_marketsRepository->refresh();

    m_sidebar->setStatusText(QStringLiteral("Status: restoring session..."));
    m_sessionStore->restoreSession();

    showSection(QStringLiteral("markets"));
}

void MarketWindow::setProtectedPagesAuthenticated(bool authenticated) {
    const int index = authenticated ? 1 : 0;

    if (m_portfolioStack) {
        m_portfolioStack->setCurrentIndex(index);
    }
    if (m_ordersStack) {
        m_ordersStack->setCurrentIndex(index);
    }
    if (m_tradesStack) {
        m_tradesStack->setCurrentIndex(index);
    }
    if (m_profileStack) {
        m_profileStack->setCurrentIndex(index);
    }
}

void MarketWindow::showSection(const QString &pageId) {
    if (pageId == QStringLiteral("markets")) {
        m_stack->setCurrentWidget(m_markets);
        m_header->setTitle(QStringLiteral("Markets"));
    } else if (pageId == QStringLiteral("portfolio")) {
        m_stack->setCurrentWidget(m_portfolioStack);
        m_header->setTitle(QStringLiteral("Portfolio"));

        if (m_sessionStore->isAuthenticated()) {
            m_portfolioPage->setLoading(QStringLiteral("Loading portfolio from backend..."));
            m_portfolioRepository->refreshPortfolio();
        }
    } else if (pageId == QStringLiteral("orders")) {
        m_stack->setCurrentWidget(m_ordersStack);
        m_header->setTitle(QStringLiteral("Orders"));

        if (m_sessionStore->isAuthenticated()) {
            m_ordersPage->setLoading(QStringLiteral("Loading orders from backend..."));
            m_ordersRepository->refreshMyOrders(100, 0, m_ordersPage->selectedStatusFilter());
        }
    } else if (pageId == QStringLiteral("trades")) {
        m_stack->setCurrentWidget(m_tradesStack);
        m_header->setTitle(QStringLiteral("Trades"));

        if (m_sessionStore->isAuthenticated()) {
            m_tradesPage->setCurrentUserId(m_sessionStore->currentSession().userId);
            m_tradesPage->setLoading(QStringLiteral("Loading your trades..."));
            m_tradesRepository->refreshMyTrades();
        }
    } else if (pageId == QStringLiteral("profile")) {
        m_stack->setCurrentWidget(m_profileStack);
        m_header->setTitle(QStringLiteral("Profile"));

        if (m_sessionStore->isAuthenticated()) {
            m_profile->setStatusMessage(QStringLiteral("Refreshing wallet..."));
            m_portfolioRepository->refreshWallet();
        }
    }

    m_sidebar->setCurrentPage(pageId);
}

void MarketWindow::onWalletLoaded(const ApiWallet &wallet) {
    const QString displayName =
        m_profileName.trimmed().isEmpty() ? wallet.userId : m_profileName;

    m_profile->setUserName(displayName);
    m_profile->setWalletAmounts(wallet.available, wallet.reserved);
    m_profile->setStatusMessage(
        wallet.updatedAt.isEmpty()
            ? QStringLiteral("Wallet loaded successfully.")
            : QStringLiteral("Last wallet update: %1")
                  .arg(formatDateTimeReadable(wallet.updatedAt)));

    m_header->setBalanceText(QStringLiteral("Balance: %1").arg(formatUnits(wallet.available)));
    m_header->setAvatarText(initialsFromName(displayName));

    m_sidebar->setStatusText(QStringLiteral("Status: connected"));
}

void MarketWindow::onWalletError(const QString &message) {
    if (!m_sessionStore->isAuthenticated()) {
        return;
    }

    m_profile->setStatusMessage(message, true);
    m_sidebar->setStatusText(QStringLiteral("Status: wallet unavailable"));
}

void MarketWindow::onSessionReady(const ApiSession &session) {
    m_profileName = session.username.trimmed().isEmpty() ? session.email : session.username;

    m_profile->setUserName(m_profileName.isEmpty() ? QStringLiteral("User") : m_profileName);
    m_profile->setEmail(session.email);
    m_profile->setWalletAmounts(0, 0);
    m_profile->setStatusMessage(QStringLiteral("Authenticated. Loading wallet..."));

    m_tradesPage->setCurrentUserId(session.userId);
    m_ordersPage->setLoading(QStringLiteral("Loading orders from backend..."));
    m_portfolioPage->setLoading(QStringLiteral("Loading portfolio from backend..."));

    m_header->setAvatarText(initialsFromName(m_profileName.isEmpty() ? QStringLiteral("User") : m_profileName));
    m_header->setBalanceText(QStringLiteral("Balance: —"));

    setProtectedPagesAuthenticated(true);
    m_sidebar->setStatusText(QStringLiteral("Status: connected"));

    m_realtimeClient->subscribeTopic(QStringLiteral("user:me"), true);
    m_realtimeClient->reconnectNow();

    m_portfolioRepository->refreshWallet();
    m_ordersRepository->refreshMyOrders(100, 0, m_ordersPage->selectedStatusFilter());
    m_portfolioRepository->refreshPortfolio();

    if (m_stack->currentWidget() == m_tradesStack) {
        m_tradesPage->setLoading(QStringLiteral("Loading your trades..."));
        m_tradesRepository->refreshMyTrades();
    }
}

void MarketWindow::onSessionCleared() {
    m_profileName = QStringLiteral("Guest");

    m_profile->setUserName(QStringLiteral("Guest"));
    m_profile->setEmail(QStringLiteral("—"));
    m_profile->setWalletAmounts(0, 0);
    m_profile->setStatusMessage(QStringLiteral("Log in or Sign up to load your wallet."));

    m_tradesPage->setCurrentUserId(QString());
    m_ordersPage->clearOrders();
    m_portfolioPage->clearPositions();

    m_header->setAvatarText(initialsFromName(m_profileName));
    m_header->setBalanceText(QStringLiteral("Balance: —"));

    setProtectedPagesAuthenticated(false);
    m_sidebar->setStatusText(QStringLiteral("Status: guest"));

    m_realtimeClient->unsubscribeTopic(QStringLiteral("user:me"));
    m_realtimeClient->reconnectNow();
}

void MarketWindow::onAuthError(const QString &message) {
    m_portfolioGate->setStatusMessage(message, true);
    m_ordersGate->setStatusMessage(message, true);
    m_tradesGate->setStatusMessage(message, true);
    m_profileGate->setStatusMessage(message, true);
}

void MarketWindow::scheduleUserRealtimeRefresh() {
    if (m_userRealtimeRefreshQueued || !m_sessionStore->isAuthenticated()) {
        return;
    }

    m_userRealtimeRefreshQueued = true;
    QTimer::singleShot(75, this, [this]() {
        m_userRealtimeRefreshQueued = false;
        if (!m_sessionStore->isAuthenticated()) {
            return;
        }

        m_portfolioRepository->refreshWallet();
        m_ordersRepository->refreshMyOrders(100, 0, m_ordersPage->selectedStatusFilter());
        m_portfolioRepository->refreshPortfolio();
        m_tradesRepository->refreshMyTrades();
    });
}

void MarketWindow::openLoginDialog() {
    openAuthDialog(false);
}

void MarketWindow::openSignUpDialog() {
    openAuthDialog(true);
}

void MarketWindow::openAuthDialog(bool startOnSignUp) {
    AuthDialog dialog(this);
    dialog.setMode(startOnSignUp ? AuthDialog::Mode::SignUp : AuthDialog::Mode::Login);

    connect(&dialog, &AuthDialog::loginSubmitted, this, [this](const QString &login, const QString &password) {
        m_sessionStore->login(login, password);
    });

    connect(&dialog,
            &AuthDialog::signUpSubmitted,
            this,
            [this](const QString &email, const QString &username, const QString &password) {
                m_sessionStore->registerUser(email, username, password);
            });

    connect(m_sessionStore, &SessionStore::authBusyChanged, &dialog, &AuthDialog::setBusy);
    connect(m_sessionStore, &SessionStore::authError, &dialog, &AuthDialog::setError);
    connect(m_sessionStore, &SessionStore::sessionReady, &dialog, &AuthDialog::acceptSuccess);

    dialog.exec();
}
