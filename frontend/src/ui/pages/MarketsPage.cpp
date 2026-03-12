#include "../../../include/frontend/ui/pages/MarketsPage.h"

#include "../../../include/frontend/ui/MarketCardWidget.h"

#include <QComboBox>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QScrollArea>
#include <QVBoxLayout>

MarketsPage::MarketsPage(QWidget *parent)
    : QWidget(parent) {
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(8);

    auto *toolbar = new QHBoxLayout;
    toolbar->setSpacing(10);

    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText(QStringLiteral("Search markets..."));
    m_searchEdit->setClearButtonEnabled(true);
    m_searchEdit->setStyleSheet(QStringLiteral(
        "QLineEdit { background: #07111d; border: 1px solid #132238; border-radius: 10px; padding: 8px 10px; color: #e8eef5; }"));

    m_statusFilter = new QComboBox(this);
    m_statusFilter->addItem(QStringLiteral("All statuses"), QString());
    m_statusFilter->addItem(QStringLiteral("Open"), QStringLiteral("OPEN"));
    m_statusFilter->addItem(QStringLiteral("Closed"), QStringLiteral("CLOSED"));
    m_statusFilter->addItem(QStringLiteral("Resolved"), QStringLiteral("RESOLVED"));
    m_statusFilter->addItem(QStringLiteral("Archived"), QStringLiteral("ARCHIVED"));
    m_statusFilter->setStyleSheet(QStringLiteral(
        "QComboBox { background: #07111d; border: 1px solid #132238; border-radius: 10px; padding: 8px 10px; color: #e8eef5; }"
        "QComboBox QAbstractItemView { background: #101722; color: #e8eef5; selection-background-color: #182331; }"));

    toolbar->addWidget(m_searchEdit, 1);
    toolbar->addWidget(m_statusFilter);
    m_statusFilter->setCurrentIndex(1);
    root->addLayout(toolbar);

    m_statusLabel = new QLabel(QStringLiteral("Loading markets…"), this);
    m_statusLabel->setWordWrap(true);
    m_statusLabel->setStyleSheet(QStringLiteral("color: #9fb0c3; padding: 6px 10px;"));
    root->addWidget(m_statusLabel);

    connect(m_searchEdit, &QLineEdit::textChanged, this, [this](const QString &) { render(); });
    connect(m_statusFilter, &QComboBox::currentTextChanged, this, [this](const QString &) { render(); });

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

    int visibleCount = 0;
    for (const ApiMarket &market : m_allMarkets) {
        if (!matchesFilters(market)) {
            continue;
        }
        addCard(market);
        ++visibleCount;
    }

    if (m_allMarkets.isEmpty()) {
        m_statusLabel->setText(QStringLiteral("No markets were returned by the backend."));
        m_statusLabel->show();
        return;
    }

    if (visibleCount == 0) {
        m_statusLabel->setText(QStringLiteral("No markets match the current filters."));
        m_statusLabel->show();
        return;
    }

    m_statusLabel->setText(QStringLiteral("Showing %1 market%2.")
                               .arg(visibleCount)
                               .arg(visibleCount == 1 ? QString() : QStringLiteral("s")));
    m_statusLabel->show();
}

bool MarketsPage::matchesFilters(const ApiMarket &market) const {
    const QString query = m_searchEdit ? m_searchEdit->text().trimmed() : QString();
    if (!query.isEmpty()) {
        const QString haystack = QStringLiteral("%1 %2")
            .arg(market.question, market.id)
            .toLower();
        if (!haystack.contains(query.toLower())) {
            return false;
        }
    }

    const QString requiredStatus = m_statusFilter ? m_statusFilter->currentData().toString().trimmed().toUpper() : QString();
    if (!requiredStatus.isEmpty()) {
        const QString marketStatus = market.status.trimmed().isEmpty() ? QStringLiteral("OPEN") : market.status.trimmed().toUpper();
        if (marketStatus != requiredStatus) {
            return false;
        }
    }

    return true;
}
