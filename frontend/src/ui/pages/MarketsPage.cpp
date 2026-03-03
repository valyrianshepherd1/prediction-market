#include "MarketsPage.h"
#include "../MarketCardWidget.h"

#include <QScrollArea>
#include <QGridLayout>
#include <QVBoxLayout>

MarketsPage::MarketsPage(QWidget *parent) : QWidget(parent) {
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    m_scroll = new QScrollArea(this);
    m_scroll->setWidgetResizable(true);
    m_scroll->setFrameShape(QFrame::NoFrame);

    m_container = new QWidget(this);
    m_grid = new QGridLayout(m_container);
    m_grid->setContentsMargins(10, 10, 10, 10);
    m_grid->setHorizontalSpacing(12);
    m_grid->setVerticalSpacing(12);

    m_scroll->setWidget(m_container);
    root->addWidget(m_scroll);

    setDemoMarkets();
}

void MarketsPage::clearCards() {
    while (m_grid->count()) {
        auto *item = m_grid->takeAt(0);
        delete item->widget();
        delete item;
    }
}

void MarketsPage::addCard(const QString &q, const QVector<OutcomeView> &o, const QString &vol) {
    auto *card = new MarketCardWidget(m_container);
    card->setMarket(q, o, vol);

    int idx = m_grid->count();
    int cols = 2;                 // simple 2-column grid
    int row = idx / cols;
    int col = idx % cols;
    m_grid->addWidget(card, row, col);
}

void MarketsPage::setCategory(const QString &category) {
    m_category = category;
    // for now just refresh demo list (later you filter + call API)
    setDemoMarkets();
}

void MarketsPage::setDemoMarkets() {
    clearCards();

    addCard("Who will be the next Supreme Leader of Iran?",
            {{"Alireza Arafi", 27}, {"Position abolished", 16}},
            "$3,483,499 vol");

    addCard("How long will the government shutdown last?",
            {{"At least 43 days", 50}, {"At least 40 days", 53}},
            "$2,518,143 vol");

    addCard("Who will Trump nominate as Fed Chair?",
            {{"Kevin Warsh", 94}, {"Judy Shelton", 4}},
            "$202,785,117 vol");
}