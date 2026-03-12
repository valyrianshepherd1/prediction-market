#pragma once

#include "../../network/MarketApiClient.h"

#include <QWidget>

class QLabel;
class QTableWidget;

class PortfolioPage : public QWidget {
    Q_OBJECT
public:
    explicit PortfolioPage(QWidget *parent = nullptr);

    void setApiClient(MarketApiClient *api);
    void setMarkets(const QVector<ApiMarket> &markets);

public slots:
    void clearPositions();
    void upsertOrder(const ApiOrder &order);

private:
    struct PositionSummary {
        QString outcomeId;
        qint64 netFilledMicros = 0;
        qint64 buyFilledMicros = 0;
        qint64 sellFilledMicros = 0;
        double buyNotional = 0.0;
    };

    void render();
    QString outcomeTitle(const QString &outcomeId) const;
    QString marketQuestion(const QString &outcomeId) const;
    void setCell(int row,
                 int column,
                 const QString &text,
                 Qt::Alignment alignment = Qt::AlignLeft | Qt::AlignVCenter);

    MarketApiClient *m_api = nullptr;
    QVector<ApiMarket> m_markets;
    QVector<ApiOrder> m_orders;
    QLabel *m_statusLabel = nullptr;
    QTableWidget *m_table = nullptr;
};