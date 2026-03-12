#include "frontend/ui/pages/OrdersPage.h"

#include <QAbstractItemView>
#include <QColor>
#include <QComboBox>
#include <QDateTime>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QLocale>
#include <QPushButton>
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

QString normalizeStatus(const QString &status) {
    const QString normalized = status.trimmed().toUpper();
    return normalized.isEmpty() ? QStringLiteral("OPEN") : normalized;
}

} // namespace

OrdersPage::OrdersPage(QWidget *parent)
    : QWidget(parent) {
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(24, 24, 24, 24);
    root->setSpacing(16);

    auto *toolbar = new QHBoxLayout;
    toolbar->setSpacing(10);

    m_statusFilter = new QComboBox(this);
    m_statusFilter->addItem(QStringLiteral("All statuses"), QString());
    m_statusFilter->addItem(QStringLiteral("Open"), QStringLiteral("OPEN"));
    m_statusFilter->addItem(QStringLiteral("Partially filled"), QStringLiteral("PARTIALLY_FILLED"));
    m_statusFilter->addItem(QStringLiteral("Filled"), QStringLiteral("FILLED"));
    m_statusFilter->addItem(QStringLiteral("Canceled"), QStringLiteral("CANCELED"));
    m_statusFilter->setStyleSheet(QStringLiteral(
        "QComboBox { background: #07111d; border: 1px solid #132238; border-radius: 10px; padding: 8px 10px; color: #e8eef5; }"
        "QComboBox QAbstractItemView { background: #101722; color: #e8eef5; selection-background-color: #182331; }"));

    m_refreshButton = new QPushButton(QStringLiteral("Refresh"), this);
    m_cancelButton = new QPushButton(QStringLiteral("Cancel selected"), this);
    m_cancelButton->setEnabled(false);

    toolbar->addWidget(m_statusFilter);
    toolbar->addWidget(m_refreshButton);
    toolbar->addStretch(1);
    toolbar->addWidget(m_cancelButton);
    root->addLayout(toolbar);

    m_statusLabel = new QLabel(
        QStringLiteral("Open Orders to load your live backend orders."),
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
            alternate-background-color: #0b1625;
            color: #e8eef5;
            selection-background-color: #17324d;
            selection-color: #ffffff;
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

    connect(m_refreshButton, &QPushButton::clicked, this, [this]() {
        emit refreshRequested(currentStatusFilter());
    });
    connect(m_statusFilter, &QComboBox::currentTextChanged, this, [this](const QString &) {
        emit refreshRequested(currentStatusFilter());
    });
    connect(m_cancelButton, &QPushButton::clicked, this, [this]() {
        const QString orderId = selectedOrderId();
        if (!orderId.isEmpty()) {
            emit cancelRequested(orderId);
        }
    });
    connect(m_table, &QTableWidget::itemSelectionChanged, this, &OrdersPage::updateToolbarState);
}

QString OrdersPage::selectedStatusFilter() const {
    return currentStatusFilter();
}

void OrdersPage::setMarkets(const QVector<ApiMarket> &markets) {
    m_markets = markets;
    render();
}

void OrdersPage::setLoading(const QString &message) {
    m_orders.clear();
    m_visibleOrders.clear();
    m_table->setRowCount(0);
    m_statusLabel->setText(message);
    m_statusLabel->setStyleSheet(QStringLiteral("color: #9fb2c7; font-size: 13px;"));
    updateToolbarState();
}

void OrdersPage::setError(const QString &message) {
    m_orders.clear();
    m_visibleOrders.clear();
    m_table->setRowCount(0);
    m_statusLabel->setText(message);
    m_statusLabel->setStyleSheet(QStringLiteral("color: #ff8b8b; font-size: 13px;"));
    updateToolbarState();
}

void OrdersPage::setOrders(const QVector<ApiOrder> &orders) {
    m_orders = orders;
    render();
}

void OrdersPage::clearOrders() {
    m_orders.clear();
    m_visibleOrders.clear();
    m_table->setRowCount(0);
    m_statusLabel->setText(QStringLiteral("Log in or Sign up to load your orders."));
    m_statusLabel->setStyleSheet(QStringLiteral("color: #9fb2c7; font-size: 13px;"));
    updateToolbarState();
}

void OrdersPage::setCancelBusy(bool busy) {
    m_cancelBusy = busy;
    if (busy) {
        m_statusLabel->setText(QStringLiteral("Canceling selected order..."));
        m_statusLabel->setStyleSheet(QStringLiteral("color: #9fb2c7; font-size: 13px;"));
    }
    updateToolbarState();
}

void OrdersPage::setCancelError(const QString &message) {
    m_cancelBusy = false;
    m_statusLabel->setText(message);
    m_statusLabel->setStyleSheet(QStringLiteral("color: #ff8b8b; font-size: 13px;"));
    updateToolbarState();
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

QString OrdersPage::currentStatusFilter() const {
    return m_statusFilter ? m_statusFilter->currentData().toString() : QString();
}

bool OrdersPage::isOrderCancelable(const ApiOrder &order) const {
    const QString status = normalizeStatus(order.status);
    return status == QStringLiteral("OPEN") || status == QStringLiteral("PARTIALLY_FILLED");
}

QString OrdersPage::selectedOrderId() const {
    if (!m_table) {
        return QString();
    }

    const int row = m_table->currentRow();
    if (row < 0 || row >= m_visibleOrders.size()) {
        return QString();
    }

    return m_visibleOrders[row].id;
}

void OrdersPage::updateToolbarState() {
    const bool hasSelection = m_table && m_table->currentRow() >= 0 && m_table->currentRow() < m_visibleOrders.size();
    const bool canCancel = hasSelection && isOrderCancelable(m_visibleOrders[m_table->currentRow()]);

    if (m_refreshButton) {
        m_refreshButton->setEnabled(!m_cancelBusy);
    }
    if (m_statusFilter) {
        m_statusFilter->setEnabled(!m_cancelBusy);
    }
    if (m_cancelButton) {
        m_cancelButton->setEnabled(!m_cancelBusy && canCancel);
        m_cancelButton->setText(m_cancelBusy ? QStringLiteral("Canceling...") : QStringLiteral("Cancel selected"));
    }
}

void OrdersPage::render() {
    m_visibleOrders.clear();
    const QString activeFilter = currentStatusFilter().trimmed().toUpper();
    for (const ApiOrder &order : m_orders) {
        if (!activeFilter.isEmpty() && normalizeStatus(order.status) != activeFilter) {
            continue;
        }
        m_visibleOrders.push_back(order);
    }

    m_table->setRowCount(m_visibleOrders.size());

    for (int row = 0; row < m_visibleOrders.size(); ++row) {
        const ApiOrder &order = m_visibleOrders[row];
        setCell(row, 0, order.id);
        setCell(row, 1, formatTimestamp(order.createdAt));
        setCell(row, 2, marketQuestion(order.outcomeId));
        setCell(row, 3, outcomeTitle(order.outcomeId), Qt::AlignCenter);
        setCell(row, 4, order.side.trimmed().toUpper(), Qt::AlignCenter);
        setCell(row, 5, formatPrice(order.priceBasisPoints), Qt::AlignRight | Qt::AlignVCenter);
        setCell(row, 6, formatQuantity(order.quantityTotalMicros), Qt::AlignRight | Qt::AlignVCenter);
        setCell(row, 7, formatQuantity(order.quantityRemainingMicros), Qt::AlignRight | Qt::AlignVCenter);
        setCell(row, 8, normalizeStatus(order.status), Qt::AlignCenter);
    }

    if (m_visibleOrders.isEmpty()) {
        const QString activeFilterText = currentStatusFilter();
        m_statusLabel->setText(activeFilterText.isEmpty()
                                   ? QStringLiteral("No orders were returned by the backend yet.")
                                   : QStringLiteral("No %1 orders were returned by the backend.").arg(activeFilterText.toLower()));
        m_statusLabel->setStyleSheet(QStringLiteral("color: #9fb2c7; font-size: 13px;"));
        updateToolbarState();
        return;
    }

    m_statusLabel->setText(QStringLiteral("Showing %1 order%2 from backend.")
                               .arg(m_visibleOrders.size())
                               .arg(m_visibleOrders.size() == 1 ? QString() : QStringLiteral("s")));
    m_statusLabel->setStyleSheet(QStringLiteral("color: #9fb2c7; font-size: 13px;"));
    updateToolbarState();
}

void OrdersPage::setCell(int row, int column, const QString &text, Qt::Alignment alignment) {
    auto *item = new QTableWidgetItem(text);
    item->setTextAlignment(alignment);
    item->setForeground(QColor(QStringLiteral("#e8eef5")));
    item->setBackground(QColor(row % 2 == 0 ? QStringLiteral("#07111d")
                                            : QStringLiteral("#0b1625")));
    m_table->setItem(row, column, item);
}
