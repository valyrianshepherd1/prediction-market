#include "../../../include/frontend/ui/pages/TradesPage.h"

#include <QAbstractItemView>
#include <QDateTime>
#include <QHeaderView>
#include <QLabel>
#include <QLocale>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

namespace {

QString formatPrice(int basisPoints) {
    return QStringLiteral("%1%")
        .arg(QLocale().toString(static_cast<double>(basisPoints) / 100.0, 'f', 2));
}

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

TradesPage::TradesPage(QWidget *parent)
    : QWidget(parent) {
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(24, 24, 24, 24);
    root->setSpacing(16);

    m_statusLabel = new QLabel(QStringLiteral("Open Trades to load your recent executions."), this);
    m_statusLabel->setWordWrap(true);
    m_statusLabel->setStyleSheet(QStringLiteral("color: #9fb2c7; font-size: 13px;"));
    root->addWidget(m_statusLabel);

    m_table = new QTableWidget(this);
    m_table->setColumnCount(8);
    m_table->setHorizontalHeaderLabels({
        QStringLiteral("Trade ID"),
        QStringLiteral("Time"),
        QStringLiteral("Role"),
        QStringLiteral("Outcome ID"),
        QStringLiteral("Price"),
        QStringLiteral("Quantity"),
        QStringLiteral("Maker Order"),
        QStringLiteral("Taker Order")
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
    m_table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);

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

    setError(QStringLiteral("Log in or Sign up to load your trade history."));
}

void TradesPage::setCurrentUserId(const QString &userId) {
    m_userId = userId;
}

void TradesPage::setLoading(const QString &message) {
    m_statusLabel->setText(message);
    m_statusLabel->setStyleSheet(QStringLiteral("color: #9fb2c7; font-size: 13px;"));
    m_table->setRowCount(0);
}

void TradesPage::setError(const QString &message) {
    m_statusLabel->setText(message);
    m_statusLabel->setStyleSheet(QStringLiteral("color: #ff7b72; font-size: 13px;"));
    m_table->setRowCount(0);
}

void TradesPage::setTrades(const QVector<ApiTrade> &trades) {
    m_table->setRowCount(trades.size());

    for (int row = 0; row < trades.size(); ++row) {
        const ApiTrade &trade = trades[row];

        setCell(row, 0, trade.id);
        setCell(row, 1, formatTimestamp(trade.createdAt));
        setCell(row, 2, roleForTrade(trade), Qt::AlignCenter);
        setCell(row, 3, trade.outcomeId);
        setCell(row, 4, formatPrice(trade.priceBasisPoints), Qt::AlignRight | Qt::AlignVCenter);
        setCell(row, 5, formatQuantity(trade.quantityMicros), Qt::AlignRight | Qt::AlignVCenter);
        setCell(row, 6, trade.makerOrderId);
        setCell(row, 7, trade.takerOrderId);
    }

    if (trades.isEmpty()) {
        m_statusLabel->setText(QStringLiteral("No trades were returned by the backend."));
        m_statusLabel->setStyleSheet(QStringLiteral("color: #9fb2c7; font-size: 13px;"));
        return;
    }

    m_statusLabel->setText(QStringLiteral("Loaded %1 trade%2.")
                               .arg(trades.size())
                               .arg(trades.size() == 1 ? QString() : QStringLiteral("s")));
    m_statusLabel->setStyleSheet(QStringLiteral("color: #9fb2c7; font-size: 13px;"));
}

QString TradesPage::roleForTrade(const ApiTrade &trade) const {
    if (!m_userId.isEmpty()) {
        if (trade.makerUserId == m_userId) {
            return QStringLiteral("Maker");
        }
        if (trade.takerUserId == m_userId) {
            return QStringLiteral("Taker");
        }
    }

    return QStringLiteral("-");
}

void TradesPage::setCell(int row, int column, const QString &text, Qt::Alignment alignment) {
    auto *item = new QTableWidgetItem(text);
    item->setTextAlignment(alignment);
    m_table->setItem(row, column, item);
}