#pragma once

#include "../../network/MarketApiClient.h"
#include <QWidget>

class QLabel;
class QGridLayout;
class QScrollArea;

class MarketsPage : public QWidget {
    Q_OBJECT
public:
    explicit MarketsPage(QWidget *parent = nullptr);

public slots:
    void setMarkets(const QVector<ApiMarket> &markets);
    void setLoading(const QString &message = QStringLiteral("Loading markets…"));
    void setError(const QString &message);

private:
    void clearCards();
    void render();
    void addCard(const ApiMarket &market);

    QVector<ApiMarket> m_allMarkets;

    QLabel *m_statusLabel = nullptr;
    QScrollArea *m_scroll = nullptr;
    QWidget *m_container = nullptr;
    QGridLayout *m_grid = nullptr;
};