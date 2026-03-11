#pragma once

#include "../network/MarketApiClient.h"

#include <QFrame>

class QLabel;
class QPushButton;
class QMouseEvent;

class MarketCardWidget : public QFrame {
    Q_OBJECT
public:
    explicit MarketCardWidget(QWidget *parent = nullptr);

    void setMarket(const ApiMarket &market);

    signals:
        void openRequested(const QString &preferredSelection);

protected:
    void mousePressEvent(QMouseEvent *event) override;

private:
    bool isBinaryYesNoMarket(const ApiMarket &market) const;
    QString metaText(const ApiMarket &market) const;

    QLabel *m_meta = nullptr;
    QLabel *m_question = nullptr;
    QPushButton *m_yesValue = nullptr;
    QPushButton *m_noValue = nullptr;
};