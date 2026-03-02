#include "MarketWindow.h"

#include "../ui/HeaderBar.h"
#include "../ui/pages/HomePage.h"
#include "../ui/pages/MarketsPage.h"
#include "../network/MarketApiClient.h"

#include <QVBoxLayout>
#include <QStackedWidget>
#include <QWidget>
#include <QMessageBox>

MarketWindow::MarketWindow(QWidget *parent) : QMainWindow(parent) {
    setWindowTitle("Prediction Market");

    // One API client for the whole app
    m_api = new MarketApiClient(this);

    auto *central = new QWidget(this);
    auto *root = new QVBoxLayout(central);
    root->setContentsMargins(12, 12, 12, 12);
    root->setSpacing(10);

    m_header = new HeaderBar(this);
    root->addWidget(m_header);

    m_stack = new QStackedWidget(this);
    root->addWidget(m_stack);

    m_home = new HomePage(this);
    m_markets = new MarketsPage(m_api, this);

    m_stack->addWidget(m_home);     // index 0
    m_stack->addWidget(m_markets);  // index 1

    setCentralWidget(central);

    connect(m_header, &HeaderBar::homeClicked, this, &MarketWindow::showHome);
    connect(m_header, &HeaderBar::profileClicked, this, &MarketWindow::openProfile);

    connect(m_home, &HomePage::goToMarkets, this, &MarketWindow::showMarkets);
    connect(m_markets, &MarketsPage::backHome, this, &MarketWindow::showHome);

    showHome();
}

void MarketWindow::showHome() {
    m_stack->setCurrentIndex(0);
}

void MarketWindow::showMarkets() {
    m_stack->setCurrentIndex(1);
}

void MarketWindow::openProfile() {
    QMessageBox::information(this, "Profile", "Profile clicked!");
}