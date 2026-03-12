#pragma once

#include "../../network/MarketApiClient.h"

#include <QHash>
#include <QWidget>

class QDoubleSpinBox;
class QLabel;
class QPushButton;

class MarketDetailsPage : public QWidget {
    Q_OBJECT
public:
    explicit MarketDetailsPage(QWidget *parent = nullptr);

    void setApiClient(MarketApiClient *api);
    void setMarket(const ApiMarket &market, const QString &preferredSelection = QString());

    signals:
        void backRequested();
    void loginRequested();

private:
    const ApiOutcome *selectedOutcome() const;
    const ApiOutcome *findOutcomeByTitle(const QString &titleUpper) const;

    void setOrderSide(const QString &side);
    void selectBinaryOutcome(const QString &titleUpper);
    void updateOutcomeButtons();
    void updateSideButtons();
    void updateTicketUi();
    void submitOrder();
    void clearTransientStatus();
    void applyFilledOrderToKnownPositions(const ApiOrder &order);
    qint64 knownOwnedMicros(const QString &outcomeId) const;

    QString normalizedSide(const QString &side) const;
    QString centsText(int pricePercent) const;

    ApiMarket m_market;
    QString m_selectedOutcomeId;
    QString m_orderSide = QStringLiteral("BUY");

    MarketApiClient *m_api = nullptr;
    bool m_orderBusy = false;
    QHash<QString, qint64> m_knownOwnedMicrosByOutcome;

    QLabel *m_questionLabel = nullptr;
    QLabel *m_metaLabel = nullptr;
    QLabel *m_ownedPointsLabel = nullptr;
    QLabel *m_tradePreviewLabel = nullptr;
    QLabel *m_payoutPreviewLabel = nullptr;

    QPushButton *m_buyButton = nullptr;
    QPushButton *m_sellButton = nullptr;

    QPushButton *m_yesButton = nullptr;
    QPushButton *m_noButton = nullptr;

    QDoubleSpinBox *m_amountSpin = nullptr;
    QLabel *m_ticketStatusLabel = nullptr;
    QPushButton *m_loginButton = nullptr;
    QPushButton *m_submitButton = nullptr;
};