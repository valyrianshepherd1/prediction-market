#pragma once

#include "../../network/MarketApiClient.h"

#include <QWidget>

class QLabel;
class QScrollArea;
class QVBoxLayout;
class QWidget;

class MarketDetailsPage : public QWidget {
    Q_OBJECT
public:
    explicit MarketDetailsPage(QWidget *parent = nullptr);

    void setMarket(const ApiMarket &market, const QString &preferredSide = QString());

    signals:
        void backRequested();

private:
    void render();
    void clearOutcomeRows();
    QWidget *createOutcomeRow(const ApiOutcome &outcome);
    QString formatCreatedAt(const QString &createdAt) const;
    QString noTextFor(int yesPercent) const;

    ApiMarket m_market;
    QString m_preferredSide;

    QLabel *m_questionLabel = nullptr;
    QLabel *m_metaLabel = nullptr;
    QLabel *m_hintLabel = nullptr;

    QScrollArea *m_scroll = nullptr;
    QWidget *m_listContainer = nullptr;
    QVBoxLayout *m_listLayout = nullptr;
};