#pragma once

#include "frontend/network/ApiTypes.h"

#include <QWidget>

class QLabel;
class QTableWidget;

class PortfolioPage : public QWidget {
    Q_OBJECT
public:
    explicit PortfolioPage(QWidget *parent = nullptr);

    void setMarkets(const QVector<ApiMarket> &markets);

public slots:
    void setLoading(const QString &message = QStringLiteral("Loading portfolio from backend..."));
    void setError(const QString &message);
    void setPositions(const QVector<ApiPortfolioPosition> &positions);
    void clearPositions();

private:
    void render();
    void setCell(int row,
                 int column,
                 const QString &text,
                 Qt::Alignment alignment = Qt::AlignLeft | Qt::AlignVCenter);

    QVector<ApiMarket> m_markets;
    QVector<ApiPortfolioPosition> m_positions;
    QLabel *m_statusLabel = nullptr;
    QTableWidget *m_table = nullptr;
};
