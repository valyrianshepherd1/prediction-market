#include "../../../include/frontend/ui/pages/OrdersPage.h"

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

OrdersPage::OrdersPage(QWidget *parent)
    : QWidget(parent) {
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(24, 24, 24, 24);
    root->setSpacing(16);

    m_statusLabel = new QLabel(
        QStringLiteral("Orders created in this app session will appear here."),
        this);
    m_statusLabel->setWordWrap(true);
    m_statusLabel->setStyleSheet(QStringLiteral("color: #9fb2c7; font-size: 13px;"));
    root->addWidget(m_statusLabel);

    m_table = new QTableWidget(this);
    m_table->setColumnCount(9);
    m_table->setHorizontalHeaderLabels({
        QStringLiteral("Order ID"),
        QStringLiteral("Time"),
        QStringLiteral("Market"),
        QStringLiteral("Outcome"),
        QStringLiteral("Side"),
        QStringLiteral("Price"),
        QStringLiteral("Total Qty"),
        QStringLiteral("Remaining"),
        QStringLiteral("Status")
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
    m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);

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

void OrdersPage::setApiClient(MarketApiClient *api) {
    if (m_api == api) {
        return;
    }

    m_api = api;
    if (!m_api) {
        return;
    }

    connect(m_api, &MarketApiClient::orderCreated, this, &OrdersPage::upsertOrder);
    connect(m_api, &MarketApiClient::sessionReady, this, [this](const ApiSession &) {
        clearOrders();
    });
    connect(m_api, &MarketApiClient::sessionCleared, this, &OrdersPage::clearOrders);
}

void OrdersPage::setMarkets(const QVector<ApiMarket> &markets) {
    m_markets = markets;
    render();
}

void OrdersPage::clearOrders() {
    m_orders.clear();
    render();
}

void OrdersPage::upsertOrder(const ApiOrder &order) {
    for (ApiOrder &existing : m_orders) {
        if (existing.id == order.id) {
            existing = order;
            render();
            return;
        }
    }

    m_orders.prepend(order);
    render();
}

QString OrdersPage::outcomeTitle(const QString &outcomeId) const {
    for (const ApiMarket &market : m_markets) {
        for (const ApiOutcome &outcome : market.outcomes) {
            if (outcome.id == outcomeId) {
                return outcome.title;
            }
        }
    }
    return outcomeId;
}

QString OrdersPage::marketQuestion(const QString &outcomeId) const {
    for (const ApiMarket &market : m_markets) {
        for (const ApiOutcome &outcome : market.outcomes) {
            if (outcome.id == outcomeId) {
                return market.question;
            }
        }
    }
    return QStringLiteral("—");
}

void OrdersPage::render() {
    m_table->setRowCount(m_orders.size());

    for (int row = 0; row < m_orders.size(); ++row) {
        const ApiOrder &order = m_orders[row];
        setCell(row, 0, order.id);
        setCell(row, 1, formatTimestamp(order.createdAt));
        setCell(row, 2, marketQuestion(order.outcomeId));
        setCell(row, 3, outcomeTitle(order.outcomeId), Qt::AlignCenter);
        setCell(row, 4, order.side.trimmed().toUpper(), Qt::AlignCenter);
        setCell(row, 5, formatPrice(order.priceBasisPoints), Qt::AlignRight | Qt::AlignVCenter);
        setCell(row, 6, formatQuantity(order.quantityTotalMicros), Qt::AlignRight | Qt::AlignVCenter);
        setCell(row, 7, formatQuantity(order.quantityRemainingMicros), Qt::AlignRight | Qt::AlignVCenter);
        setCell(row, 8, order.status.trimmed().isEmpty() ? QStringLiteral("—") : order.status.trimmed().toUpper(),
                Qt::AlignCenter);
    }

    if (m_orders.isEmpty()) {
        m_statusLabel->setText(QStringLiteral(
            "Orders created in this app session will appear here."));
        m_statusLabel->setStyleSheet(QStringLiteral("color: #9fb2c7; font-size: 13px;"));
        return;
    }

    m_statusLabel->setText(QStringLiteral(
        "Showing %1 order%2 created in this app session.")
            .arg(m_orders.size())
            .arg(m_orders.size() == 1 ? QString() : QStringLiteral("s")));
    m_statusLabel->setStyleSheet(QStringLiteral("color: #9fb2c7; font-size: 13px;"));
}

void OrdersPage::setCell(int row, int column, const QString &text, Qt::Alignment alignment) {
    auto *item = new QTableWidgetItem(text);
    item->setTextAlignment(alignment);
    m_table->setItem(row, column, item);
}