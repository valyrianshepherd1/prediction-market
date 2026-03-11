#include "MarketWindow.h"

#include "../ui/HeaderBar.h"
#include "../ui/Sidebar.h"
#include "../ui/dialogs/AuthDialog.h"
#include "../ui/pages/AuthRequiredPage.h"
#include "../ui/pages/MarketsPage.h"
#include "../ui/pages/ProfilePage.h"
#include "../ui/pages/TradesPage.h"
#include "../ui/pages/MarketDetailsPage.h"

#include <QApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QLocale>
#include <QStackedWidget>
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

QWidget *createPlaceholderPage(const QString &title,
                               const QString &text,
                               QWidget *parent = nullptr) {
    auto *page = new QWidget(parent);
    page->setObjectName(QStringLiteral("CenterPage"));

    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(36, 36, 36, 36);
    layout->setSpacing(14);

    auto *titleLabel = new QLabel(title, page);
    titleLabel->setObjectName(QStringLiteral("CenterPageTitle"));

    auto *textLabel = new QLabel(text, page);
    textLabel->setObjectName(QStringLiteral("CenterPageText"));
    textLabel->setWordWrap(true);

    layout->addWidget(titleLabel);
    layout->addWidget(textLabel);
    layout->addStretch(1);

    return page;
}

QStackedWidget *createProtectedStack(QWidget *gate, QWidget *content, QWidget *parent = nullptr) {
    auto *stack = new QStackedWidget(parent);
    stack->addWidget(gate);
    stack->addWidget(content);
    stack->setCurrentIndex(0);
    return stack;
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

    m_marketDetailsPage = new MarketDetailsPage(this);
    m_marketDetailsPage->setObjectName(QStringLiteral("CenterPage"));

    m_header = new HeaderBar(this);
    m_markets = new MarketsPage(this);
    m_profile = new ProfilePage(this);
    m_tradesPage = new TradesPage(this);
    m_api = new MarketApiClient(this);

    m_portfolioPage = createPlaceholderPage(
        QStringLiteral("Portfolio"),
        QStringLiteral("This page will show your portfolio, positions and exposure."),
        this);

    m_ordersPage = createPlaceholderPage(
        QStringLiteral("Orders"),
        QStringLiteral("This page will show your active, filled and cancelled orders."),
        this);

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
    connect(m_profile, &ProfilePage::logoutRequested, m_api, &MarketApiClient::logout);

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

    connect(m_portfolioGate, &AuthRequiredPage::loginRequested, this, &MarketWindow::openLoginDialog);
    connect(m_portfolioGate, &AuthRequiredPage::signUpRequested, this, &MarketWindow::openSignUpDialog);
    connect(m_ordersGate, &AuthRequiredPage::loginRequested, this, &MarketWindow::openLoginDialog);
    connect(m_ordersGate, &AuthRequiredPage::signUpRequested, this, &MarketWindow::openSignUpDialog);
    connect(m_tradesGate, &AuthRequiredPage::loginRequested, this, &MarketWindow::openLoginDialog);
    connect(m_tradesGate, &AuthRequiredPage::signUpRequested, this, &MarketWindow::openSignUpDialog);
    connect(m_profileGate, &AuthRequiredPage::loginRequested, this, &MarketWindow::openLoginDialog);
    connect(m_profileGate, &AuthRequiredPage::signUpRequested, this, &MarketWindow::openSignUpDialog);

    connect(m_api, &MarketApiClient::marketsReady, m_markets, &MarketsPage::setMarkets);
    connect(m_api, &MarketApiClient::marketsError, m_markets, &MarketsPage::setError);
    connect(m_api, &MarketApiClient::walletReady, this, &MarketWindow::onWalletLoaded);
    connect(m_api, &MarketApiClient::walletError, this, &MarketWindow::onWalletError);
    connect(m_api, &MarketApiClient::tradesReady, m_tradesPage, &TradesPage::setTrades);
    connect(m_api, &MarketApiClient::tradesError, m_tradesPage, &TradesPage::setError);

    connect(m_api, &MarketApiClient::sessionReady, this, &MarketWindow::onSessionReady);
    connect(m_api, &MarketApiClient::sessionCleared, this, &MarketWindow::onSessionCleared);
    connect(m_api, &MarketApiClient::authError, this, &MarketWindow::onAuthError);

    setProtectedPagesAuthenticated(false);

    m_sidebar->setStatusText(QStringLiteral("Status: loading markets..."));
    m_markets->setLoading(QStringLiteral("Loading markets from %1…").arg(m_api->baseUrl()));
    m_api->fetchMarkets();

    m_sidebar->setStatusText(QStringLiteral("Status: restoring session..."));
    m_api->restoreSession();

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
    } else if (pageId == QStringLiteral("orders")) {
        m_stack->setCurrentWidget(m_ordersStack);
        m_header->setTitle(QStringLiteral("Orders"));
    } else if (pageId == QStringLiteral("trades")) {
        m_stack->setCurrentWidget(m_tradesStack);
        m_header->setTitle(QStringLiteral("Trades"));

        if (m_api->isAuthenticated()) {
            m_tradesPage->setCurrentUserId(m_api->currentSession().userId);
            m_tradesPage->setLoading(QStringLiteral("Loading your trades..."));
            m_api->fetchMyTrades();
        }
    } else if (pageId == QStringLiteral("profile")) {
        m_stack->setCurrentWidget(m_profileStack);
        m_header->setTitle(QStringLiteral("Profile"));

        if (m_api->isAuthenticated()) {
            m_profile->setStatusMessage(QStringLiteral("Refreshing wallet..."));
            m_api->fetchWallet();
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
            : QStringLiteral("Wallet updated at %1").arg(wallet.updatedAt));

    m_header->setBalanceText(QStringLiteral("Balance: %1").arg(formatUnits(wallet.available)));
    m_header->setAvatarText(initialsFromName(displayName));
    m_sidebar->setStatusText(QStringLiteral("Status: connected"));
}

void MarketWindow::onWalletError(const QString &message) {
    if (!m_api->isAuthenticated()) {
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

    m_header->setAvatarText(initialsFromName(m_profileName.isEmpty() ? QStringLiteral("User") : m_profileName));
    m_header->setBalanceText(QStringLiteral("Balance: —"));

    setProtectedPagesAuthenticated(true);
    m_sidebar->setStatusText(QStringLiteral("Status: connected"));

    m_api->fetchWallet();

    if (m_stack->currentWidget() == m_tradesStack) {
        m_tradesPage->setLoading(QStringLiteral("Loading your trades..."));
        m_api->fetchMyTrades();
    }
}

void MarketWindow::onSessionCleared() {
    m_profileName = QStringLiteral("Guest");

    m_profile->setUserName(QStringLiteral("Guest"));
    m_profile->setEmail(QStringLiteral("—"));
    m_profile->setWalletAmounts(0, 0);
    m_profile->setStatusMessage(QStringLiteral("Log in or Sign up to load your wallet."));

    m_tradesPage->setCurrentUserId(QString());

    m_header->setAvatarText(initialsFromName(m_profileName));
    m_header->setBalanceText(QStringLiteral("Balance: —"));

    setProtectedPagesAuthenticated(false);
    m_sidebar->setStatusText(QStringLiteral("Status: guest"));
}

void MarketWindow::onAuthError(const QString &message) {
    m_portfolioGate->setStatusMessage(message, true);
    m_ordersGate->setStatusMessage(message, true);
    m_tradesGate->setStatusMessage(message, true);
    m_profileGate->setStatusMessage(message, true);
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
        m_api->login(login, password);
    });

    connect(&dialog,
            &AuthDialog::signUpSubmitted,
            this,
            [this](const QString &email, const QString &username, const QString &password) {
                m_api->registerUser(email, username, password);
            });

    connect(m_api, &MarketApiClient::authBusyChanged, &dialog, &AuthDialog::setBusy);
    connect(m_api, &MarketApiClient::authError, &dialog, &AuthDialog::setError);
    connect(m_api, &MarketApiClient::sessionReady, &dialog, &AuthDialog::acceptSuccess);

    dialog.exec();
}