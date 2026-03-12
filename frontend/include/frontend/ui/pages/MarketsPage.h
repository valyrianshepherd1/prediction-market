#pragma once

#include "../../network/MarketApiClient.h"

#include <QWidget>

class QComboBox;
class QGridLayout;
class QLabel;
class QLineEdit;
class QScrollArea;

class MarketsPage : public QWidget {
    Q_OBJECT
public:
    explicit MarketsPage(QWidget *parent = nullptr);

    signals:
        void marketRequested(const ApiMarket &market, const QString &preferredSelection);

public slots:
    void setMarkets(const QVector<ApiMarket> &markets);
    void setLoading(const QString &message = QStringLiteral("Loading markets…"));
    void setError(const QString &message);

private:
    void clearCards();
    void render();
    void addCard(const ApiMarket &market);
    bool matchesFilters(const ApiMarket &market) const;

    QVector<ApiMarket> m_allMarkets;
    QLabel *m_statusLabel = nullptr;
    QLineEdit *m_searchEdit = nullptr;
    QComboBox *m_statusFilter = nullptr;
    QScrollArea *m_scroll = nullptr;
    QWidget *m_container = nullptr;
    QGridLayout *m_grid = nullptr;
};