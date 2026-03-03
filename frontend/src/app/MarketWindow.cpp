#include "MarketWindow.h"
#include "../ui/HeaderBar.h"
#include "../ui/Sidebar.h"
#include "../ui/pages/MarketsPage.h"
#include <QApplication>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>

static QString darkStyle() {
    return R"(
        QWidget { background: #0b0f14; color: #e8eef5; font-size: 14px; }
        #HeaderBar { background: #0b0f14; }
        #HeaderTitle { font-size: 18px; font-weight: 600; }

        #Sidebar { background: #0b0f14; }
        #SidebarTitle { font-size: 13px; color: #9fb0c3; }

        QListWidget { background: #0b0f14; border: 1px solid #1b2430; border-radius: 10px; padding: 6px; }
        QListWidget::item { padding: 10px; border-radius: 8px; }
        QListWidget::item:selected { background: #141c26; }

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

MarketWindow::MarketWindow(QWidget *parent) : QMainWindow(parent) {
    setWindowTitle("Prediction Market");

    auto *central = new QWidget(this);
    auto *root = new QVBoxLayout(central);
    root->setContentsMargins(12, 12, 12, 12);
    root->setSpacing(10);

    m_header = new HeaderBar(this);
    root->addWidget(m_header);

    auto *body = new QWidget(this);
    auto *h = new QHBoxLayout(body);
    h->setContentsMargins(0, 0, 0, 0);
    h->setSpacing(12);

    m_sidebar = new Sidebar(this);
    m_sidebar->setFixedWidth(200);

    m_markets = new MarketsPage(this);

    h->addWidget(m_sidebar);
    h->addWidget(m_markets, 1);

    root->addWidget(body, 1);
    setCentralWidget(central);

    qApp->setStyleSheet(darkStyle());

    connect(m_sidebar, &Sidebar::categoryChanged, this, &MarketWindow::onCategoryChanged);
    connect(m_header, &HeaderBar::homeClicked, [this] { onCategoryChanged("Trending"); });
    connect(m_header, &HeaderBar::profileClicked, this, &MarketWindow::openProfile);

    onCategoryChanged("Trending");
}

void MarketWindow::onCategoryChanged(const QString &cat) {
    m_header->setTitle(cat);
    m_markets->setCategory(cat);
}

void MarketWindow::openProfile() {
    QMessageBox::information(this, "Profile", "Profile clicked!");
}