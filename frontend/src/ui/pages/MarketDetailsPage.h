#pragma once

#include "../../network/MarketApiClient.h"

#include <QWidget>

class QDoubleSpinBox;
class QLabel;
class QPushButton;
class QScrollArea;
class QVBoxLayout;
class QWidget;

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
    bool isBinaryYesNoMarket() const;
    const ApiOutcome *selectedOutcome() const;
    const ApiOutcome *findOutcomeByTitle(const QString &titleUpper) const;

    void render();
    void clearOutcomeRows();
    QWidget *createOutcomeRow(const ApiOutcome &outcome);

    void selectOutcome(const QString &outcomeId);
    void selectBinaryOutcome(const QString &titleUpper);
    void setOrderSide(const QString &side);

    void syncAmountDisplay();
    void updateBinaryChoiceButtons();
    void updateTicketUi();
    void submitOrder();

    QString formatCreatedAt(const QString &createdAt) const;
    QString normalizedSide(const QString &side) const;
    QString centsText(int pricePercent) const;
    QString outcomeButtonText(const QString &label, int pricePercent) const;

    ApiMarket m_market;
    QString m_selectedOutcomeId;
    QString m_orderSide = QStringLiteral("BUY");

    MarketApiClient *m_api = nullptr;
    bool m_orderBusy = false;

    QLabel *m_questionLabel = nullptr;
    QLabel *m_metaLabel = nullptr;

    QScrollArea *m_scroll = nullptr;
    QWidget *m_listContainer = nullptr;
    QVBoxLayout *m_listLayout = nullptr;

    QLabel *m_selectedVariantValue = nullptr;
    QPushButton *m_buyButton = nullptr;
    QPushButton *m_sellButton = nullptr;

    QWidget *m_binaryChoiceContainer = nullptr;
    QPushButton *m_yesChoiceButton = nullptr;
    QPushButton *m_noChoiceButton = nullptr;

    QDoubleSpinBox *m_priceSpin = nullptr;
    QDoubleSpinBox *m_amountSpin = nullptr;
    QLabel *m_amountValueLabel = nullptr;
    QLabel *m_ticketStatusLabel = nullptr;
    QPushButton *m_loginButton = nullptr;
    QPushButton *m_submitButton = nullptr;
};