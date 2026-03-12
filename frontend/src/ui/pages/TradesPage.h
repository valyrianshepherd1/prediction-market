#pragma once

#include "../../network/MarketApiClient.h"

#include <QWidget>

class QLabel;
class QTableWidget;

class TradesPage : public QWidget {
    Q_OBJECT
public:
    explicit TradesPage(QWidget *parent = nullptr);

    void setCurrentUserId(const QString &userId);

public slots:
    void setLoading(const QString &message = QStringLiteral("Loading trades..."));
    void setError(const QString &message);
    void setTrades(const QVector<ApiTrade> &trades);

private:
    QString roleForTrade(const ApiTrade &trade) const;
    void setCell(int row,
                 int column,
                 const QString &text,
                 Qt::Alignment alignment = Qt::AlignLeft | Qt::AlignVCenter);

    QString m_userId;
    QLabel *m_statusLabel = nullptr;
    QTableWidget *m_table = nullptr;
};