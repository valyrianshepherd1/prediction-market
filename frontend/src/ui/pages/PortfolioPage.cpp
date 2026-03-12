#include "frontend/ui/pages/PortfolioPage.h"

#include <QAbstractItemView>
#include <QDateTime>
#include <QHeaderView>
#include <QLabel>
#include <QLocale>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

namespace {

QString formatQuantity(qint64 quantityMicros) {
    return QLocale().toString(static_cast<double>(quantityMicros) / 1000000.0, 'f', 4);
}

QString formatTimestamp(const QString &timestamp) {
    const QDateTime parsed = QDateTime::fromString(timestamp, Qt::ISODate);
    if (!parsed.isValid()) {
        return timestamp;
    }
    return QLocale().toString(parsed.toLocalTime(), QLocale::ShortFormat);
}

} // namespace

PortfolioPage::PortfolioPage(QWidget *parent)
    : QWidget(parent) {
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(24, 24, 24, 24);
    root->setSpacing(16);

    m_statusLabel = new QLabel(
        QStringLiteral("Open Portfolio to load your live positions from backend."),
        this);
    m_statusLabel->setWordWrap(true);
    m_statusLabel->setStyleSheet(QStringLiteral("color: #9fb2c7; font-size: 13px;"));
    root->addWidget(m_statusLabel);

    m_table = new QTableWidget(this);
    m_table->setColumnCount(6);
    m_table->setHorizontalHeaderLabels({
        QStringLiteral("Market"),
        QStringLiteral("Outcome"),
        QStringLiteral("Available"),
        QStringLiteral("Reserved"),
        QStringLiteral("Total"),
        QStringLiteral("Updated")
    });

    m_table->setAlternatingRowColors(true);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setShowGrid(false);
    m_table->verticalHeader()->setVisible(false);
    m_table->horizontalHeader()->setHighlightSections(false);
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);

    m_table->setStyleSheet(R"(
        QTableWidget {
            background: #07111d;
            border: 1px solid #132238;
            border-radius: 16px;
            gridline-color: #132238;
        }
        QTableWidget::item {
            padding: 6px;
            color: #e8eef5;
        }
        QHeaderView::section {
            background: #101722;
            color: #c8d7e6;
            border: none;
            border-bottom: 1px solid #132238;
            padding: 8px 10px;
            font-weight: 600;
        }
    )");

    root->addWidget(m_table, 1);
}

void PortfolioPage::setMarkets(const QVector<ApiMarket> &markets) {
    m_markets = markets;
    Q_UNUSED(m_markets);
}

void PortfolioPage::setLoading(const QString &message) {
    m_positions.clear();
    m_table->setRowCount(0);
    m_statusLabel->setText(message);
    m_statusLabel->setStyleSheet(QStringLiteral("color: #9fb2c7; font-size: 13px;"));
}

void PortfolioPage::setError(const QString &message) {
    m_positions.clear();
    m_table->setRowCount(0);
    m_statusLabel->setText(message);
    m_statusLabel->setStyleSheet(QStringLiteral("color: #ff8b8b; font-size: 13px;"));
}

void PortfolioPage::setPositions(const QVector<ApiPortfolioPosition> &positions) {
    m_positions = positions;
    render();
}

void PortfolioPage::clearPositions() {
    m_positions.clear();
    m_table->setRowCount(0);
    m_statusLabel->setText(QStringLiteral("Log in or Sign up to load your portfolio."));
    m_statusLabel->setStyleSheet(QStringLiteral("color: #9fb2c7; font-size: 13px;"));
}

void PortfolioPage::render() {
    m_table->setRowCount(m_positions.size());

    for (int row = 0; row < m_positions.size(); ++row) {
        const ApiPortfolioPosition &position = m_positions[row];
        const qint64 totalShares = position.sharesTotal > 0
                                      ? position.sharesTotal
                                      : (position.sharesAvailable + position.sharesReserved);

        setCell(row, 0, position.marketQuestion.isEmpty() ? position.marketId : position.marketQuestion);
        setCell(row, 1, position.outcomeTitle.isEmpty() ? position.outcomeId : position.outcomeTitle, Qt::AlignCenter);
        setCell(row, 2, formatQuantity(position.sharesAvailable), Qt::AlignRight | Qt::AlignVCenter);
        setCell(row, 3, formatQuantity(position.sharesReserved), Qt::AlignRight | Qt::AlignVCenter);
        setCell(row, 4, formatQuantity(totalShares), Qt::AlignRight | Qt::AlignVCenter);
        setCell(row, 5, formatTimestamp(position.updatedAt));
    }

    if (m_positions.isEmpty()) {
        m_statusLabel->setText(QStringLiteral(
            "No portfolio positions were returned by the backend yet."));
        m_statusLabel->setStyleSheet(QStringLiteral("color: #9fb2c7; font-size: 13px;"));
        return;
    }

    m_statusLabel->setText(QStringLiteral(
        "Showing %1 portfolio position%2 from backend.")
            .arg(m_positions.size())
            .arg(m_positions.size() == 1 ? QString() : QStringLiteral("s")));
    m_statusLabel->setStyleSheet(QStringLiteral("color: #9fb2c7; font-size: 13px;"));
}

void PortfolioPage::setCell(int row, int column, const QString &text, Qt::Alignment alignment) {
    auto *item = new QTableWidgetItem(text);
    item->setTextAlignment(alignment);
    m_table->setItem(row, column, item);
}
