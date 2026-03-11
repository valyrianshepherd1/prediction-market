#pragma once

#include "../network/MarketApiClient.h"

#include <QFrame>
#include <QVector>

class QLabel;
class QPushButton;
class QVBoxLayout;
class QWidget;

class MarketCardWidget : public QFrame {
    Q_OBJECT
public:
    explicit MarketCardWidget(QWidget *parent = nullptr);

    void setMarket(const ApiMarket &market);

    signals:
        void yesClicked();
    void noClicked();

private:
    void clearVariantRows();
    void addVariantPreview(const ApiOutcome &outcome);
    QString metaText(const ApiMarket &market) const;
    QString noTextFor(int yesPercent) const;

    QLabel *m_question = nullptr;
    QLabel *m_meta = nullptr;
    QWidget *m_variantsContainer = nullptr;
    QVBoxLayout *m_variantsLayout = nullptr;
    QLabel *m_moreLabel = nullptr;
    QPushButton *m_yesButton = nullptr;
    QPushButton *m_noButton = nullptr;
};