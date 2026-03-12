#include "../../../include/frontend/ui/pages/PortfolioPage.h"

#include <QAbstractItemView>
#include <QHash>
#include <QHeaderView>
#include <QLabel>
#include <QLocale>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>
#include <algorithm>

namespace {

QString formatQuantity(qint64 quantityMicros) {
    return QLocale().toString(static_cast<double>(quantityMicros) / 1000000.0, 'f', 4);
}

QString formatPrice(double pricePercent) {
    return QStringLiteral("%1%")
        .arg(QLocale().toString(pricePercent, 'f', 2));
}

} // namespace

PortfolioPage::PortfolioPage(QWidget *parent)
    : QWidget(parent) {
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(24, 24, 24, 24);
    root->setSpacing(16);

    m_statusLabel = new QLabel(
        QStringLiteral("Filled positions confirmed in this app session will appear here."),
        this);
    m_statusLabel->setWordWrap(true);
    m_statusLabel->setStyleSheet(QStringLiteral("color: #9fb2c7; font-size: 13px;"));
    root->addWidget(m_statusLabel);

    m_table = new QTableWidget(this);
    m_table->setColumnCount(6);
    m_table->setHorizontalHeaderLabels({
        QStringLiteral("Market"),
        QStringLiteral("Outcome"),
        QStringLiteral("Net Points"),
        QStringLiteral("Buy Filled"),
        QStringLiteral("Sell Filled"),
        QStringLiteral("Avg Buy Price")
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

void PortfolioPage::setApiClient(MarketApiClient *api) {
    if (m_api == api) {
        return;
    }

    m_api = api;
    if (!m_api) {
        return;
    }

    connect(m_api, &MarketApiClient::orderCreated, this, &PortfolioPage::upsertOrder);
    connect(m_api, &MarketApiClient::sessionReady, this, [this](const ApiSession &) {
        clearPositions();
    });
    connect(m_api, &MarketApiClient::sessionCleared, this, &PortfolioPage::clearPositions);
}

void PortfolioPage::setMarkets(const QVector<ApiMarket> &markets) {
    m_markets = markets;
    render();
}

void PortfolioPage::clearPositions() {
    m_orders.clear();
    render();
}

void PortfolioPage::upsertOrder(const ApiOrder &order) {
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

QString PortfolioPage::outcomeTitle(const QString &outcomeId) const {
    for (const ApiMarket &market : m_markets) {
        for (const ApiOutcome &outcome : market.outcomes) {
            if (outcome.id == outcomeId) {
                return outcome.title;
            }
        }
    }
    return outcomeId;
}

QString PortfolioPage::marketQuestion(const QString &outcomeId) const {
    for (const ApiMarket &market : m_markets) {
        for (const ApiOutcome &outcome : market.outcomes) {
            if (outcome.id == outcomeId) {
                return market.question;
            }
        }
    }
    return QStringLiteral("—");
}

void PortfolioPage::render() {
    QHash<QString, PositionSummary> positions;

    for (const ApiOrder &order : m_orders) {
        const qint64 filledMicros = qMax<qint64>(0, order.quantityTotalMicros - order.quantityRemainingMicros);
        if (filledMicros <= 0) {
            continue;
        }

        PositionSummary &summary = positions[order.outcomeId];
        summary.outcomeId = order.outcomeId;

        const bool isSell = order.side.trimmed().toUpper() == QStringLiteral("SELL");
        if (isSell) {
            summary.sellFilledMicros += filledMicros;
            summary.netFilledMicros -= filledMicros;
        } else {
            summary.buyFilledMicros += filledMicros;
            summary.netFilledMicros += filledMicros;
            summary.buyNotional += (static_cast<double>(filledMicros) / 1000000.0) *
                                   (static_cast<double>(order.priceBasisPoints) / 100.0);
        }
    }

    QList<PositionSummary> rows = positions.values();
    std::sort(rows.begin(), rows.end(), [](const PositionSummary &lhs, const PositionSummary &rhs) {
        return lhs.outcomeId < rhs.outcomeId;
    });

    m_table->setRowCount(rows.size());

    for (int row = 0; row < rows.size(); ++row) {
        const PositionSummary &summary = rows[row];
        const double buyFilled = static_cast<double>(summary.buyFilledMicros) / 1000000.0;
        const double avgBuyPricePercent = buyFilled > 0.0 ? (summary.buyNotional / buyFilled) : 0.0;

        setCell(row, 0, marketQuestion(summary.outcomeId));
        setCell(row, 1, outcomeTitle(summary.outcomeId), Qt::AlignCenter);
        setCell(row, 2, formatQuantity(summary.netFilledMicros), Qt::AlignRight | Qt::AlignVCenter);
        setCell(row, 3, formatQuantity(summary.buyFilledMicros), Qt::AlignRight | Qt::AlignVCenter);
        setCell(row, 4, formatQuantity(summary.sellFilledMicros), Qt::AlignRight | Qt::AlignVCenter);
        setCell(row, 5, buyFilled > 0.0 ? formatPrice(avgBuyPricePercent) : QStringLiteral("—"),
                Qt::AlignRight | Qt::AlignVCenter);
    }

    if (rows.isEmpty()) {
        m_statusLabel->setText(QStringLiteral(
            "No filled positions are tracked yet in this app session."));
        m_statusLabel->setStyleSheet(QStringLiteral("color: #9fb2c7; font-size: 13px;"));
        return;
    }

    m_statusLabel->setText(QStringLiteral(
        "Showing %1 tracked position%2 from filled order quantities in this app session.")
            .arg(rows.size())
            .arg(rows.size() == 1 ? QString() : QStringLiteral("s")));
    m_statusLabel->setStyleSheet(QStringLiteral("color: #9fb2c7; font-size: 13px;"));
}

void PortfolioPage::setCell(int row, int column, const QString &text, Qt::Alignment alignment) {
    auto *item = new QTableWidgetItem(text);
    item->setTextAlignment(alignment);
    m_table->setItem(row, column, item);
}