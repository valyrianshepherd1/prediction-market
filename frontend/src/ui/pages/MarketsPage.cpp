#include "../../../include/frontend/ui/pages/MarketsPage.h"

#include "../../../include/frontend/ui/MarketCardWidget.h"

#include <QFrame>
#include <QGridLayout>
#include <QLabel>
#include <QScrollArea>
#include <QVBoxLayout>

MarketsPage::MarketsPage(QWidget *parent)
    : QWidget(parent) {
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(8);

    m_statusLabel = new QLabel(QStringLiteral("Loading markets…"), this);
    m_statusLabel->setWordWrap(true);
    m_statusLabel->setStyleSheet(QStringLiteral("color: #9fb0c3; padding: 6px 10px;"));
    root->addWidget(m_statusLabel);

    m_scroll = new QScrollArea(this);
    m_scroll->setWidgetResizable(true);
    m_scroll->setFrameShape(QFrame::NoFrame);

    m_container = new QWidget(this);
    m_grid = new QGridLayout(m_container);
    m_grid->setContentsMargins(10, 10, 10, 10);
    m_grid->setHorizontalSpacing(12);
    m_grid->setVerticalSpacing(12);

    m_scroll->setWidget(m_container);
    root->addWidget(m_scroll, 1);
}

void MarketsPage::setLoading(const QString &message) {
    m_statusLabel->setText(message);
    m_statusLabel->show();
    clearCards();
}

void MarketsPage::setError(const QString &message) {
    m_statusLabel->setText(message);
    m_statusLabel->show();
    clearCards();
}

void MarketsPage::setMarkets(const QVector<ApiMarket> &markets) {
    m_allMarkets = markets;
    render();
}

void MarketsPage::clearCards() {
    while (m_grid->count()) {
        QLayoutItem *item = m_grid->takeAt(0);
        delete item->widget();
        delete item;
    }
}

void MarketsPage::addCard(const ApiMarket &market) {
    auto *card = new MarketCardWidget(m_container);
    card->setMarket(market);

    connect(card, &MarketCardWidget::openRequested, this,
            [this, market](const QString &preferredSelection) {
                emit marketRequested(market, preferredSelection);
            });

    const int idx = m_grid->count();
    const int cols = 2;
    const int row = idx / cols;
    const int col = idx % cols;

    m_grid->addWidget(card, row, col);
}

void MarketsPage::render() {
    clearCards();

    for (const ApiMarket &market : m_allMarkets) {
        addCard(market);
    }

    if (m_allMarkets.isEmpty()) {
        m_statusLabel->setText(QStringLiteral("No open markets were returned by the backend."));
        m_statusLabel->show();
        return;
    }

    m_statusLabel->hide();
}