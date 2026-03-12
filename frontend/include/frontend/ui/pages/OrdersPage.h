#pragma once

#include "frontend/network/ApiTypes.h"

#include <QWidget>

class QComboBox;
class QLabel;
class QPushButton;
class QTableWidget;

class OrdersPage : public QWidget {
    Q_OBJECT
public:
    explicit OrdersPage(QWidget *parent = nullptr);

    void setMarkets(const QVector<ApiMarket> &markets);
    [[nodiscard]] QString selectedStatusFilter() const;

    signals:
        void refreshRequested(const QString &statusFilter);
    void cancelRequested(const QString &orderId);

public slots:
    void setLoading(const QString &message = QStringLiteral("Loading orders from backend..."));
    void setError(const QString &message);
    void setOrders(const QVector<ApiOrder> &orders);
    void clearOrders();
    void setCancelBusy(bool busy);
    void setCancelError(const QString &message);

private:
    void render();
    void updateToolbarState();
    [[nodiscard]] QString currentStatusFilter() const;
    [[nodiscard]] bool isOrderCancelable(const ApiOrder &order) const;
    [[nodiscard]] QString selectedOrderId() const;
    QString outcomeTitle(const QString &outcomeId) const;
    QString marketQuestion(const QString &outcomeId) const;
    void setCell(int row,
                 int column,
                 const QString &text,
                 Qt::Alignment alignment = Qt::AlignLeft | Qt::AlignVCenter);

    QVector<ApiMarket> m_markets;
    QVector<ApiOrder> m_orders;
    QVector<ApiOrder> m_visibleOrders;
    QLabel *m_statusLabel = nullptr;
    QComboBox *m_statusFilter = nullptr;
    QPushButton *m_refreshButton = nullptr;
    QPushButton *m_cancelButton = nullptr;
    QTableWidget *m_table = nullptr;
    bool m_cancelBusy = false;
};
