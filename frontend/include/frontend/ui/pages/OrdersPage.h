#pragma once

#include "../../network/MarketApiClient.h"

#include <QWidget>

class QLabel;
class QTableWidget;

class OrdersPage : public QWidget {
    Q_OBJECT
public:
    explicit OrdersPage(QWidget *parent = nullptr);

    void setApiClient(MarketApiClient *api);
    void setMarkets(const QVector<ApiMarket> &markets);

public slots:
    void clearOrders();
    void upsertOrder(const ApiOrder &order);

private:
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