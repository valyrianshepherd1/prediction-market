#include "MarketWindow.h"
#include "../ui/HeaderBar.h"
#include "../ui/pages/MarketsPage.h"
#include "../ui/pages/ProfilePage.h"

#include <QApplication>
#include <QHBoxLayout>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QWidget>

namespace {
QString darkStyle() {
    return R"(
        QWidget { background: #0b0f14; color: #e8eef5; font-size: 14px; }
        #HeaderBar { background: #0b0f14; }
        #HeaderTitle { font-size: 18px; font-weight: 600; }

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

QString profileNameFromEnvironment() {
    const QString configured = qEnvironmentVariable("PM_FRONTEND_USER_NAME");
    if (!configured.trimmed().isEmpty()) {
        return configured;
    }
    return QStringLiteral("Guest");
}
} // namespace

MarketWindow::MarketWindow(QWidget *parent) : QMainWindow(parent) {
    setWindowTitle(QStringLiteral("Prediction Market"));

    auto *central = new QWidget(this);
    auto *root = new QVBoxLayout(central);
    root->setContentsMargins(12, 12, 12, 12);
    root->setSpacing(10);

    m_header = new HeaderBar(this);
    root->addWidget(m_header);

    auto *body = new QWidget(this);
    auto *bodyLayout = new QHBoxLayout(body);
    bodyLayout->setContentsMargins(0, 0, 0, 0);
    bodyLayout->setSpacing(0);

    m_markets = new MarketsPage(this);
    m_profile = new ProfilePage(this);
    m_api = new MarketApiClient(this);

    m_stack = new QStackedWidget(this);
    m_stack->addWidget(m_markets);
    m_stack->addWidget(m_profile);

    bodyLayout->addWidget(m_stack, 1);

    root->addWidget(body, 1);
    setCentralWidget(central);

    qApp->setStyleSheet(darkStyle());

    m_profileName = profileNameFromEnvironment();
    m_profile->setUserName(m_profileName);

    connect(m_header, &HeaderBar::profileClicked, this, &MarketWindow::openProfile);
    connect(m_profile, &ProfilePage::backRequested, this, &MarketWindow::showMarkets);

    connect(m_api, &MarketApiClient::marketsReady, m_markets, &MarketsPage::setMarkets);
    connect(m_api, &MarketApiClient::marketsError, m_markets, &MarketsPage::setError);
    connect(m_api, &MarketApiClient::walletReady, this, &MarketWindow::onWalletLoaded);
    connect(m_api, &MarketApiClient::walletError, this, &MarketWindow::onWalletError);

    m_header->setTitle(QStringLiteral("Markets"));

    m_markets->setLoading(QStringLiteral("Loading markets from %1…").arg(m_api->baseUrl()));
    m_api->fetchMarkets();

    if (!m_api->configuredUserId().trimmed().isEmpty()) {
        m_profile->setStatusMessage(QStringLiteral("Loading wallet from %1…").arg(m_api->baseUrl()));
        m_api->fetchWallet();
    }
}

void MarketWindow::openProfile() {
    m_header->setTitle(QStringLiteral("Profile"));
    if (m_stack) {
        m_stack->setCurrentWidget(m_profile);
    }

    if (!m_api->configuredUserId().trimmed().isEmpty()) {
        m_profile->setStatusMessage(QStringLiteral("Refreshing wallet…"));
        m_api->fetchWallet();
    }
}

void MarketWindow::showMarkets() {
    if (m_stack) {
        m_stack->setCurrentWidget(m_markets);
    }
    m_header->setTitle(QStringLiteral("Markets"));
}

void MarketWindow::onWalletLoaded(const ApiWallet &wallet) {
    const QString displayName = m_profileName == QStringLiteral("Guest") && !wallet.userId.isEmpty()
                                    ? wallet.userId
                                    : m_profileName;
    m_profile->setUserName(displayName);
    m_profile->setWalletAmounts(wallet.available, wallet.reserved);
    m_profile->setStatusMessage(
        wallet.updatedAt.isEmpty()
            ? QStringLiteral("Wallet loaded successfully.")
            : QStringLiteral("Wallet updated at %1").arg(wallet.updatedAt));
}

void MarketWindow::onWalletError(const QString &message) {
    m_profile->setStatusMessage(message, true);
}