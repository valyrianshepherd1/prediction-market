#pragma once

#include "frontend/network/ApiTypes.h"

#include <QHash>
#include <QWidget>

class MarketDetailsRepository;
class OrdersRepository;
class SessionStore;
class QDoubleSpinBox;
class QLabel;
class QPushButton;
class QFrame;

class MarketDetailsPage : public QWidget {
    Q_OBJECT
public:
    explicit MarketDetailsPage(QWidget *parent = nullptr);

    void setSessionStore(SessionStore *sessionStore);
    void setOrdersRepository(OrdersRepository *ordersRepository);
    void setMarketDetailsRepository(MarketDetailsRepository *marketDetailsRepository);
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
    void updatePhotoPanel();
    void updateMetaLabel();
    void applyMarketDetailsSnapshot(const ApiMarketDetailsSnapshot &snapshot);
    void submitOrder();
    void clearTransientStatus();
    void applyFilledOrderToKnownPositions(const ApiOrder &order);
    qint64 knownOwnedMicros(const QString &outcomeId) const;

    QString normalizedSide(const QString &side) const;
    QString centsText(int pricePercent) const;

    ApiMarket m_market;
    QString m_selectedOutcomeId;
    QString m_orderSide = QStringLiteral("BUY");

    SessionStore *m_sessionStore = nullptr;
    OrdersRepository *m_ordersRepository = nullptr;
    MarketDetailsRepository *m_marketDetailsRepository = nullptr;
    bool m_orderBusy = false;
    QHash<QString, qint64> m_knownOwnedMicrosByOutcome;
    ApiOrderBook m_selectedOrderBook;
    QVector<ApiTrade> m_recentTrades;

    QLabel *m_questionLabel = nullptr;

    QFrame *m_photoFrame = nullptr;
    QLabel *m_photoLabel = nullptr;

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
