#include "frontend/ui/pages/MarketDetailsPage.h"

#include "frontend/repositories/MarketDetailsRepository.h"
#include "frontend/repositories/OrdersRepository.h"
#include "frontend/state/SessionStore.h"

#include <QAbstractSpinBox>
#include <QDateTime>
#include <QDoubleSpinBox>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLocale>
#include <QMap>
#include <QStringList>
#include <QPushButton>
#include <QSizePolicy>
#include <QTimer>
#include <QVBoxLayout>
#include <QtMath>

namespace {

QString formatCreatedAt(const QString &value) {
    const QDateTime dt = QDateTime::fromString(value, Qt::ISODate);
    if (!dt.isValid()) {
        return value.trimmed();
    }
    return dt.toLocalTime().toString(QStringLiteral("dd MMM yyyy, HH:mm"));
}

QString formatMoney(double amount) {
    return QStringLiteral("%1").arg(QLocale().toString(amount, 'f', amount >= 100.0 ? 2 : 4));
}

QString formatPoints(qint64 micros) {
    return QLocale().toString(static_cast<double>(micros) / 1000000.0, 'f', 4);
}


int bestBidPercent(const ApiOrderBook &orderBook) {
    if (orderBook.buy.isEmpty()) {
        return -1;
    }
    return qBound(0, orderBook.buy.first().priceBasisPoints / 100, 100);
}

int bestAskPercent(const ApiOrderBook &orderBook) {
    if (orderBook.sell.isEmpty()) {
        return -1;
    }
    return qBound(0, orderBook.sell.first().priceBasisPoints / 100, 100);
}

QString lastTradeSummary(const QVector<ApiTrade> &trades) {
    if (trades.isEmpty()) {
        return {};
    }

    const ApiTrade &trade = trades.first();
    return QStringLiteral("last %1¢").arg(qBound(0, trade.priceBasisPoints / 100, 100));
}

} // namespace

MarketDetailsPage::MarketDetailsPage(QWidget *parent)
    : QWidget(parent) {
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(24, 24, 24, 24);
    root->setSpacing(18);

    auto *topRow = new QHBoxLayout;
    topRow->setSpacing(12);

    auto *backButton = new QPushButton(QStringLiteral("← Back"), this);
    backButton->setCursor(Qt::PointingHandCursor);
    backButton->setFixedWidth(120);
    backButton->setStyleSheet(QStringLiteral(
        "QPushButton { background: #141c26; border: 1px solid #1b2430; color: white; "
        "padding: 10px 14px; border-radius: 12px; font-weight: 600; }"
        "QPushButton:hover { background: #182131; }"));

    m_questionLabel = new QLabel(this);
    m_questionLabel->setWordWrap(true);
    m_questionLabel->setStyleSheet(QStringLiteral(
        "color: white; font-size: 30px; font-weight: 700;"));

    topRow->addWidget(backButton, 0, Qt::AlignLeft | Qt::AlignTop);
    topRow->addWidget(m_questionLabel, 1, Qt::AlignVCenter);

    root->addLayout(topRow);

    auto *contentRow = new QHBoxLayout;
    contentRow->setSpacing(18);
    contentRow->setAlignment(Qt::AlignTop);

    auto *leftPanel = new QWidget(this);
    leftPanel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    auto *leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(0);

    m_photoFrame = new QFrame(leftPanel);
    m_photoFrame->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_photoFrame->setStyleSheet(QStringLiteral(
        "QFrame { background: #07111d; border: 1px solid #132238; border-radius: 18px; }"));

    auto *photoLayout = new QVBoxLayout(m_photoFrame);
    photoLayout->setContentsMargins(28, 28, 28, 28);
    photoLayout->setSpacing(0);

    m_photoLabel = new QLabel(m_photoFrame);
    m_photoLabel->setAlignment(Qt::AlignCenter);
    m_photoLabel->setWordWrap(true);
    m_photoLabel->setMinimumHeight(120);
    m_photoLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    m_photoLabel->setStyleSheet(QStringLiteral(
        "color: #a9bbcf; font-size: 24px; font-weight: 700; "
        "background: transparent; border: none; padding: 18px;"));

    photoLayout->addStretch(1);
    photoLayout->addWidget(m_photoLabel, 0, Qt::AlignCenter);
    photoLayout->addStretch(1);

    leftLayout->addWidget(m_photoFrame, 0, Qt::AlignTop);

    auto *rightPanel = new QWidget(this);
    rightPanel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

    auto *rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(0);

    auto *ticketCard = new QFrame(rightPanel);
    ticketCard->setMinimumWidth(440);
    ticketCard->setMaximumWidth(500);
    ticketCard->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    ticketCard->setStyleSheet(QStringLiteral(
        "QFrame { background: #07111d; border: 1px solid #132238; border-radius: 16px; }"));

    auto *ticketLayout = new QVBoxLayout(ticketCard);
    ticketLayout->setContentsMargins(20, 20, 20, 20);
    ticketLayout->setSpacing(14);

    auto *ticketTitle = new QLabel(QStringLiteral("Trade"), ticketCard);
    ticketTitle->setAlignment(Qt::AlignCenter);
    ticketTitle->setStyleSheet(QStringLiteral(
        "color: white; font-size: 22px; font-weight: 700;"));
    ticketLayout->addWidget(ticketTitle);

    m_metaLabel = new QLabel(ticketCard);
    m_metaLabel->setAlignment(Qt::AlignCenter);
    m_metaLabel->setWordWrap(true);
    m_metaLabel->setStyleSheet(QStringLiteral(
        "color: #9fb2c7; font-size: 14px;"));
    m_metaLabel->setFixedHeight(44);
    ticketLayout->addWidget(m_metaLabel);

    auto *sideRow = new QHBoxLayout;
    sideRow->setSpacing(10);

    m_buyButton = new QPushButton(QStringLiteral("Buy"), ticketCard);
    m_sellButton = new QPushButton(QStringLiteral("Sell"), ticketCard);

    m_buyButton->setCursor(Qt::PointingHandCursor);
    m_sellButton->setCursor(Qt::PointingHandCursor);

    sideRow->addWidget(m_buyButton);
    sideRow->addWidget(m_sellButton);
    ticketLayout->addLayout(sideRow);

    auto *outcomeRow = new QHBoxLayout;
    outcomeRow->setSpacing(10);

    m_yesButton = new QPushButton(QStringLiteral("Yes"), ticketCard);
    m_noButton = new QPushButton(QStringLiteral("No"), ticketCard);

    m_yesButton->setCursor(Qt::PointingHandCursor);
    m_noButton->setCursor(Qt::PointingHandCursor);

    outcomeRow->addWidget(m_yesButton);
    outcomeRow->addWidget(m_noButton);
    ticketLayout->addLayout(outcomeRow);

    m_ownedPointsLabel = new QLabel(ticketCard);
    m_ownedPointsLabel->setStyleSheet(QStringLiteral(
        "color: #9fb2c7; font-size: 14px; font-weight: 600;"));
    ticketLayout->addWidget(m_ownedPointsLabel);

    auto *amountLabel = new QLabel(QStringLiteral("Amount"), ticketCard);
    amountLabel->setStyleSheet(QStringLiteral(
        "color: white; font-size: 18px; font-weight: 700;"));
    ticketLayout->addWidget(amountLabel);

    m_amountSpin = new QDoubleSpinBox(ticketCard);
    m_amountSpin->setDecimals(6);
    m_amountSpin->setRange(0.000001, 1000000.0);
    m_amountSpin->setSingleStep(1.0);
    m_amountSpin->setValue(1.0);
    m_amountSpin->setStyleSheet(QStringLiteral(
        "QDoubleSpinBox { background: #09131f; border: 1px solid #223349; border-radius: 10px; "
        "padding: 10px 12px; color: white; font-size: 20px; }"));
    m_amountSpin->setButtonSymbols(QAbstractSpinBox::NoButtons);
    ticketLayout->addWidget(m_amountSpin);

    auto *quickRow = new QHBoxLayout;
    quickRow->setSpacing(8);

    auto addQuickButton = [this, quickRow, ticketCard](const QString &text, double delta) {
        auto *button = new QPushButton(text, ticketCard);
        button->setCursor(Qt::PointingHandCursor);
        button->setStyleSheet(QStringLiteral(
            "QPushButton { background: #1a2430; border: 1px solid #243447; color: white; "
            "padding: 10px 14px; border-radius: 10px; font-weight: 600; }"
            "QPushButton:hover { background: #243243; }"));
        quickRow->addWidget(button);

        connect(button, &QPushButton::clicked, this, [this, delta]() {
            m_amountSpin->setValue(m_amountSpin->value() + delta);
        });
    };

    addQuickButton(QStringLiteral("+1"), 1.0);
    addQuickButton(QStringLiteral("+5"), 5.0);
    addQuickButton(QStringLiteral("+10"), 10.0);
    addQuickButton(QStringLiteral("+100"), 100.0);

    ticketLayout->addLayout(quickRow);

    m_tradePreviewLabel = new QLabel(ticketCard);
    m_tradePreviewLabel->setWordWrap(true);
    m_tradePreviewLabel->setStyleSheet(QStringLiteral(
        "color: #dbe7f3; font-size: 14px; font-weight: 600;"));
    m_tradePreviewLabel->setFixedHeight(42);
    ticketLayout->addWidget(m_tradePreviewLabel);

    m_payoutPreviewLabel = new QLabel(ticketCard);
    m_payoutPreviewLabel->setWordWrap(true);
    m_payoutPreviewLabel->setStyleSheet(QStringLiteral(
        "color: #9fb2c7; font-size: 13px;"));
    m_payoutPreviewLabel->setFixedHeight(42);
    ticketLayout->addWidget(m_payoutPreviewLabel);

    m_ticketStatusLabel = new QLabel(ticketCard);
    m_ticketStatusLabel->setWordWrap(true);
    m_ticketStatusLabel->setStyleSheet(QStringLiteral(
        "color: #9fb2c7; font-size: 13px;"));
    m_ticketStatusLabel->setFixedHeight(38);
    ticketLayout->addWidget(m_ticketStatusLabel);

    m_loginButton = new QPushButton(QStringLiteral("Log in to trade"), ticketCard);
    m_loginButton->setCursor(Qt::PointingHandCursor);
    m_loginButton->setStyleSheet(QStringLiteral(
        "QPushButton { background: #1a2430; border: 1px solid #243447; color: white; "
        "padding: 12px 16px; border-radius: 12px; font-weight: 700; }"
        "QPushButton:hover { background: #243243; }"));
    ticketLayout->addWidget(m_loginButton);

    m_submitButton = new QPushButton(QStringLiteral("Trade"), ticketCard);
    m_submitButton->setCursor(Qt::PointingHandCursor);
    m_submitButton->setStyleSheet(QStringLiteral(
        "QPushButton { background: #1d7dfa; border: 1px solid #3b8ef9; color: white; "
        "padding: 12px 16px; border-radius: 12px; font-size: 18px; font-weight: 700; }"
        "QPushButton:hover { background: #2c87fb; }"
        "QPushButton:disabled { background: #172331; color: #7c8fa4; border-color: #223349; }"));
    ticketLayout->addWidget(m_submitButton);

    rightLayout->addWidget(ticketCard, 0, Qt::AlignTop | Qt::AlignHCenter);

    contentRow->addWidget(leftPanel, 3, Qt::AlignTop);
    contentRow->addWidget(rightPanel, 2, Qt::AlignTop);

    root->addLayout(contentRow, 1);

    QTimer::singleShot(0, this, [ticketCard, photoFrame = m_photoFrame]() {
        if (!ticketCard || !photoFrame) {
            return;
        }

        const int stableHeight = qMax(ticketCard->sizeHint().height(), ticketCard->minimumSizeHint().height());
        if (stableHeight <= 0) {
            return;
        }

        ticketCard->setMinimumHeight(stableHeight);
        ticketCard->setMaximumHeight(stableHeight);
        photoFrame->setMinimumHeight(stableHeight);
        photoFrame->setMaximumHeight(stableHeight);
    });

    connect(backButton, &QPushButton::clicked, this, &MarketDetailsPage::backRequested);

    connect(m_buyButton, &QPushButton::clicked, this, [this]() {
        setOrderSide(QStringLiteral("BUY"));
    });
    connect(m_sellButton, &QPushButton::clicked, this, [this]() {
        setOrderSide(QStringLiteral("SELL"));
    });

    connect(m_yesButton, &QPushButton::clicked, this, [this]() {
        selectBinaryOutcome(QStringLiteral("YES"));
    });
    connect(m_noButton, &QPushButton::clicked, this, [this]() {
        selectBinaryOutcome(QStringLiteral("NO"));
    });

    connect(m_amountSpin,
            qOverload<double>(&QDoubleSpinBox::valueChanged),
            this,
            [this](double) {
                clearTransientStatus();
                updateTicketUi();
            });

    connect(m_loginButton, &QPushButton::clicked, this, &MarketDetailsPage::loginRequested);
    connect(m_submitButton, &QPushButton::clicked, this, &MarketDetailsPage::submitOrder);

    updatePhotoPanel();
    updateTicketUi();
}

void MarketDetailsPage::setSessionStore(SessionStore *sessionStore) {
    if (m_sessionStore == sessionStore) {
        return;
    }

    m_sessionStore = sessionStore;

    if (!m_sessionStore) {
        updateTicketUi();
        return;
    }

    connect(m_sessionStore, &SessionStore::sessionReady, this, [this](const ApiSession &) {
        m_knownOwnedMicrosByOutcome.clear();
        clearTransientStatus();
        updateTicketUi();
    });

    connect(m_sessionStore, &SessionStore::sessionCleared, this, [this]() {
        m_knownOwnedMicrosByOutcome.clear();
        clearTransientStatus();
        updateTicketUi();
    });

    updateTicketUi();
}

void MarketDetailsPage::setOrdersRepository(OrdersRepository *ordersRepository) {
    if (m_ordersRepository == ordersRepository) {
        return;
    }

    m_ordersRepository = ordersRepository;

    if (!m_ordersRepository) {
        updateTicketUi();
        return;
    }

    connect(m_ordersRepository, &OrdersRepository::orderBusyChanged, this, [this](bool busy) {
        m_orderBusy = busy;

        if (busy) {
            m_ticketStatusLabel->setText(QStringLiteral("Submitting order..."));
            m_ticketStatusLabel->setStyleSheet(QStringLiteral(
                "color: #9fb2c7; font-size: 13px;"));
        }

        updateTicketUi();
    });

    connect(m_ordersRepository, &OrdersRepository::orderCreated, this, [this](const ApiOrder &order) {
        m_orderBusy = false;
        applyFilledOrderToKnownPositions(order);
        m_ticketStatusLabel->setText(QStringLiteral("Order placed: %1").arg(order.id));
        m_ticketStatusLabel->setStyleSheet(QStringLiteral(
            "color: #7ef0a8; font-size: 13px;"));
        if (m_marketDetailsRepository) {
            m_marketDetailsRepository->refreshSelectedOutcomeData();
        }
        updateTicketUi();
    });

    connect(m_ordersRepository, &OrdersRepository::orderError, this, [this](const QString &message) {
        m_orderBusy = false;
        m_ticketStatusLabel->setText(message);
        m_ticketStatusLabel->setStyleSheet(QStringLiteral(
            "color: #ff7b72; font-size: 13px;"));
        updateTicketUi();
    });

    updateTicketUi();
}

void MarketDetailsPage::setMarketDetailsRepository(MarketDetailsRepository *marketDetailsRepository) {
    if (m_marketDetailsRepository == marketDetailsRepository) {
        return;
    }

    m_marketDetailsRepository = marketDetailsRepository;

    if (!m_marketDetailsRepository) {
        return;
    }

    connect(m_marketDetailsRepository, &MarketDetailsRepository::snapshotReady, this,
            [this](const ApiMarketDetailsSnapshot &snapshot) {
                applyMarketDetailsSnapshot(snapshot);
            });

    connect(m_marketDetailsRepository, &MarketDetailsRepository::loadingChanged, this,
            [this](bool loading) {
                if (loading) {
                    if (m_ticketStatusLabel && m_ticketStatusLabel->text().trimmed().isEmpty()) {
                        m_ticketStatusLabel->setText(QStringLiteral("Refreshing market snapshot..."));
                        m_ticketStatusLabel->setStyleSheet(QStringLiteral(
                            "color: #9fb2c7; font-size: 13px;"));
                    }
                    return;
                }

                if (m_ticketStatusLabel) {
                    const QString current = m_ticketStatusLabel->text();
                    if (current == QStringLiteral("Refreshing market snapshot...") ||
                        current == QStringLiteral("Refreshing selected outcome...")) {
                        clearTransientStatus();
                    }
                }
            });

    connect(m_marketDetailsRepository, &MarketDetailsRepository::errorOccurred, this,
            [this](const QString &message) {
                if (m_orderBusy) {
                    return;
                }

                m_ticketStatusLabel->setText(message);
                m_ticketStatusLabel->setStyleSheet(QStringLiteral(
                    "color: #ffb86b; font-size: 13px;"));
            });
}


QString MarketDetailsPage::normalizedSide(const QString &side) const {
    return side.trimmed().toUpper() == QStringLiteral("SELL")
               ? QStringLiteral("SELL")
               : QStringLiteral("BUY");
}

QString MarketDetailsPage::centsText(int pricePercent) const {
    return QStringLiteral("%1¢").arg(qBound(0, pricePercent, 100));
}

const ApiOutcome *MarketDetailsPage::findOutcomeByTitle(const QString &titleUpper) const {
    for (const ApiOutcome &outcome : m_market.outcomes) {
        if (outcome.title.trimmed().toUpper() == titleUpper) {
            return &outcome;
        }
    }
    return nullptr;
}

const ApiOutcome *MarketDetailsPage::selectedOutcome() const {
    for (const ApiOutcome &outcome : m_market.outcomes) {
        if (outcome.id == m_selectedOutcomeId) {
            return &outcome;
        }
    }
    return nullptr;
}

void MarketDetailsPage::updatePhotoPanel() {
    if (!m_photoLabel) {
        return;
    }

    m_photoLabel->setText(QStringLiteral("Photo is not found\nor not used"));
}


void MarketDetailsPage::updateMetaLabel() {
    const QString status = m_market.status.isEmpty()
                               ? QStringLiteral("OPEN")
                               : m_market.status;

    const QString createdAt = formatCreatedAt(m_market.createdAt);

    QStringList parts;
    parts << status;
    if (!createdAt.isEmpty()) {
        parts << createdAt;
    }
    parts << QStringLiteral("%1 variants").arg(m_market.outcomes.size());

    const int bid = bestBidPercent(m_selectedOrderBook);
    const int ask = bestAskPercent(m_selectedOrderBook);
    if (bid >= 0) {
        parts << QStringLiteral("bid %1¢").arg(bid);
    }
    if (ask >= 0) {
        parts << QStringLiteral("ask %1¢").arg(ask);
    }

    const QString tradeSummary = lastTradeSummary(m_recentTrades);
    if (!tradeSummary.isEmpty()) {
        parts << tradeSummary;
    }

    m_metaLabel->setText(parts.join(QStringLiteral(" • ")));
}

void MarketDetailsPage::applyMarketDetailsSnapshot(const ApiMarketDetailsSnapshot &snapshot) {
    if (m_market.id.trimmed().isEmpty() || snapshot.market.id != m_market.id) {
        return;
    }

    ApiMarket mergedMarket = m_market;
    mergedMarket.id = snapshot.market.id;
    if (!snapshot.market.question.trimmed().isEmpty()) {
        mergedMarket.question = snapshot.market.question;
    }
    if (!snapshot.market.status.trimmed().isEmpty()) {
        mergedMarket.status = snapshot.market.status;
    }
    if (!snapshot.market.createdAt.trimmed().isEmpty()) {
        mergedMarket.createdAt = snapshot.market.createdAt;
    }
    if (!snapshot.market.outcomes.isEmpty()) {
        mergedMarket.outcomes = snapshot.market.outcomes;
    }

    m_market = mergedMarket;
    m_selectedOrderBook = snapshot.orderBook;
    m_recentTrades = snapshot.recentTrades;

    if (!snapshot.selectedOutcomeId.trimmed().isEmpty()) {
        m_selectedOutcomeId = snapshot.selectedOutcomeId;
    }

    m_questionLabel->setText(m_market.question);
    updateMetaLabel();
    updateTicketUi();
}

void MarketDetailsPage::setMarket(const ApiMarket &market, const QString &preferredSelection) {
    m_market = market;
    m_selectedOrderBook = ApiOrderBook{};
    m_recentTrades.clear();
    m_selectedOutcomeId.clear();
    m_orderSide = QStringLiteral("BUY");

    const QString preferred = preferredSelection.trimmed().toUpper();
    if (preferred == QStringLiteral("SELL")) {
        m_orderSide = QStringLiteral("SELL");
    }

    if (preferred == QStringLiteral("NO")) {
        const ApiOutcome *noOutcome = findOutcomeByTitle(QStringLiteral("NO"));
        if (noOutcome) {
            m_selectedOutcomeId = noOutcome->id;
        }
    }

    if (m_selectedOutcomeId.isEmpty()) {
        const ApiOutcome *yesOutcome = findOutcomeByTitle(QStringLiteral("YES"));
        if (yesOutcome) {
            m_selectedOutcomeId = yesOutcome->id;
        } else if (!m_market.outcomes.isEmpty()) {
            m_selectedOutcomeId = m_market.outcomes.first().id;
        }
    }

    m_questionLabel->setText(m_market.question);
    updateMetaLabel();
    updatePhotoPanel();
    clearTransientStatus();
    updateTicketUi();

    if (m_marketDetailsRepository && !m_market.id.trimmed().isEmpty()) {
        if (m_ticketStatusLabel) {
            m_ticketStatusLabel->setText(QStringLiteral("Refreshing market snapshot..."));
            m_ticketStatusLabel->setStyleSheet(QStringLiteral(
                "color: #9fb2c7; font-size: 13px;"));
        }
        m_marketDetailsRepository->openMarket(m_market.id, preferredSelection);
    }
}

void MarketDetailsPage::clearTransientStatus() {
    if (!m_ticketStatusLabel) {
        return;
    }

    m_ticketStatusLabel->clear();
    m_ticketStatusLabel->setStyleSheet(QStringLiteral(
        "color: #9fb2c7; font-size: 13px;"));
}

void MarketDetailsPage::setOrderSide(const QString &side) {
    m_orderSide = normalizedSide(side);
    clearTransientStatus();
    updateTicketUi();
}

void MarketDetailsPage::selectBinaryOutcome(const QString &titleUpper) {
    const ApiOutcome *outcome = findOutcomeByTitle(titleUpper);
    if (!outcome) {
        return;
    }

    m_selectedOutcomeId = outcome->id;
    clearTransientStatus();
    if (m_ticketStatusLabel && m_marketDetailsRepository) {
        m_ticketStatusLabel->setText(QStringLiteral("Refreshing selected outcome..."));
        m_ticketStatusLabel->setStyleSheet(QStringLiteral(
            "color: #9fb2c7; font-size: 13px;"));
    }
    updateMetaLabel();
    updateTicketUi();

    if (m_marketDetailsRepository) {
        m_marketDetailsRepository->selectOutcomeById(outcome->id);
    }
}

void MarketDetailsPage::updateSideButtons() {
    const bool isBuy = (m_orderSide == QStringLiteral("BUY"));

    m_buyButton->setStyleSheet(QStringLiteral(
        "QPushButton { background: %1; border: 1px solid %2; color: %3; "
        "padding: 12px 16px; border-radius: 12px; font-size: 18px; font-weight: 700; }"
        "QPushButton:hover { background: %4; }")
        .arg(isBuy ? QStringLiteral("#123222") : QStringLiteral("#1a2430"))
        .arg(isBuy ? QStringLiteral("#1f7a45") : QStringLiteral("#243447"))
        .arg(isBuy ? QStringLiteral("#7ef0a8") : QStringLiteral("#8ea4bc"))
        .arg(isBuy ? QStringLiteral("#17452d") : QStringLiteral("#243243")));

    m_sellButton->setStyleSheet(QStringLiteral(
        "QPushButton { background: %1; border: 1px solid %2; color: %3; "
        "padding: 12px 16px; border-radius: 12px; font-size: 18px; font-weight: 700; }"
        "QPushButton:hover { background: %4; }")
        .arg(!isBuy ? QStringLiteral("#34181c") : QStringLiteral("#1a2430"))
        .arg(!isBuy ? QStringLiteral("#8b2f39") : QStringLiteral("#243447"))
        .arg(!isBuy ? QStringLiteral("#ff8f9b") : QStringLiteral("#8ea4bc"))
        .arg(!isBuy ? QStringLiteral("#472126") : QStringLiteral("#243243")));
}

void MarketDetailsPage::updateOutcomeButtons() {
    const ApiOutcome *yesOutcome = findOutcomeByTitle(QStringLiteral("YES"));
    const ApiOutcome *noOutcome = findOutcomeByTitle(QStringLiteral("NO"));

    const bool yesSelected = yesOutcome && (m_selectedOutcomeId == yesOutcome->id);
    const bool noSelected = noOutcome && (m_selectedOutcomeId == noOutcome->id);

    m_yesButton->setText(QStringLiteral("Yes %1")
                             .arg(yesOutcome ? centsText(yesOutcome->pricePercent) : QStringLiteral("—")));
    m_noButton->setText(QStringLiteral("No %1")
                            .arg(noOutcome ? centsText(noOutcome->pricePercent) : QStringLiteral("—")));

    m_yesButton->setStyleSheet(QStringLiteral(
        "QPushButton { background: %1; border: 1px solid #1f7a45; color: #7ef0a8; "
        "padding: 14px 16px; border-radius: 12px; font-size: 18px; font-weight: 700; }"
        "QPushButton:hover { background: #17452d; }")
        .arg(yesSelected ? QStringLiteral("#17452d") : QStringLiteral("#123222")));

    m_noButton->setStyleSheet(QStringLiteral(
        "QPushButton { background: %1; border: 1px solid #8b2f39; color: #ff8f9b; "
        "padding: 14px 16px; border-radius: 12px; font-size: 18px; font-weight: 700; }"
        "QPushButton:hover { background: #472126; }")
        .arg(noSelected ? QStringLiteral("#472126") : QStringLiteral("#34181c")));
}

qint64 MarketDetailsPage::knownOwnedMicros(const QString &outcomeId) const {
    return m_knownOwnedMicrosByOutcome.value(outcomeId, 0);
}

void MarketDetailsPage::applyFilledOrderToKnownPositions(const ApiOrder &order) {
    const qint64 filledMicros = qMax<qint64>(0, order.quantityTotalMicros - order.quantityRemainingMicros);
    if (filledMicros <= 0) {
        return;
    }

    const bool isSell = order.side.trimmed().toUpper() == QStringLiteral("SELL");
    const qint64 delta = isSell ? -filledMicros : filledMicros;
    m_knownOwnedMicrosByOutcome[order.outcomeId] += delta;
}

void MarketDetailsPage::updateTicketUi() {
    const ApiOutcome *outcome = selectedOutcome();

    const bool loggedIn = m_sessionStore && m_sessionStore->isAuthenticated();

    updateSideButtons();
    updateOutcomeButtons();

    const bool hasOutcome = (outcome != nullptr);
    const QString normalizedMarketStatus = m_market.status.trimmed().isEmpty()
                                               ? QStringLiteral("OPEN")
                                               : m_market.status.trimmed().toUpper();
    const bool marketOpen = (normalizedMarketStatus == QStringLiteral("OPEN"));

    m_amountSpin->setEnabled(hasOutcome && !m_orderBusy && marketOpen);
    m_loginButton->setVisible(!loggedIn && marketOpen);
    m_submitButton->setVisible(loggedIn);
    m_submitButton->setEnabled(loggedIn && hasOutcome && !m_orderBusy && marketOpen);
    m_submitButton->setText(m_orderBusy ? QStringLiteral("Submitting...") : QStringLiteral("Trade"));

    if (!hasOutcome) {
        m_ownedPointsLabel->setText(QStringLiteral("Owned points: 0.0000"));
        m_tradePreviewLabel->setText(QStringLiteral("Choose Yes or No."));
        m_payoutPreviewLabel->clear();
    } else {
        const double amount = m_amountSpin->value();
        const double price = static_cast<double>(outcome->pricePercent) / 100.0;
        const double notional = amount * price;
        const qint64 ownedMicros = knownOwnedMicros(outcome->id);

        m_ownedPointsLabel->setText(
            QStringLiteral("Owned %1 points: %2")
                .arg(outcome->title.trimmed().toUpper())
                .arg(formatPoints(ownedMicros)));

        m_tradePreviewLabel->setText(
            QStringLiteral("%1 %2 at %3 × %4 points")
                .arg(m_orderSide == QStringLiteral("BUY") ? QStringLiteral("Buy") : QStringLiteral("Sell"))
                .arg(outcome->title.trimmed().toUpper())
                .arg(centsText(outcome->pricePercent))
                .arg(formatMoney(amount)));

        if (m_orderSide == QStringLiteral("BUY")) {
            const double payoutIfCorrect = amount;
            const double profitIfCorrect = payoutIfCorrect - notional;
            m_payoutPreviewLabel->setText(
                QStringLiteral("Cost now: %1 • Payout if correct: %2 • Profit if correct: %3")
                    .arg(formatMoney(notional))
                    .arg(formatMoney(payoutIfCorrect))
                    .arg(formatMoney(profitIfCorrect)));
        } else {
            const double receiveNow = notional;
            const double maxLiability = amount - notional;
            m_payoutPreviewLabel->setText(
                QStringLiteral("Receive now: %1 • Max liability if the outcome wins: %2")
                    .arg(formatMoney(receiveNow))
                    .arg(formatMoney(maxLiability)));
        }
    }

    if (!marketOpen) {
        m_ticketStatusLabel->setText(QStringLiteral("Trading is unavailable: market status is %1.")
                                         .arg(normalizedMarketStatus));
        m_ticketStatusLabel->setStyleSheet(QStringLiteral(
            "color: #ffb86b; font-size: 13px;"));
    } else if (!loggedIn) {
        m_ticketStatusLabel->setText(QStringLiteral("Log in or Sign up to place an order."));
        m_ticketStatusLabel->setStyleSheet(QStringLiteral(
            "color: #9fb2c7; font-size: 13px;"));
    } else if (!hasOutcome && m_ticketStatusLabel->text().trimmed().isEmpty()) {
        m_ticketStatusLabel->setText(QStringLiteral("Choose Yes or No."));
        m_ticketStatusLabel->setStyleSheet(QStringLiteral(
            "color: #9fb2c7; font-size: 13px;"));
    }

    if (m_photoFrame && m_photoFrame->minimumHeight() <= 0) {
        m_photoFrame->updateGeometry();
    }
}

void MarketDetailsPage::submitOrder() {
    if (!m_sessionStore || !m_sessionStore->isAuthenticated()) {
        emit loginRequested();
        return;
    }

    if (!m_ordersRepository) {
        m_ticketStatusLabel->setText(QStringLiteral("Trading backend is not connected."));
        m_ticketStatusLabel->setStyleSheet(QStringLiteral(
            "color: #ff7b72; font-size: 13px;"));
        return;
    }

    const ApiOutcome *outcome = selectedOutcome();
    if (!outcome) {
        m_ticketStatusLabel->setText(QStringLiteral("Choose Yes or No first."));
        m_ticketStatusLabel->setStyleSheet(QStringLiteral(
            "color: #ff7b72; font-size: 13px;"));
        return;
    }

    const qint64 quantityMicros = qRound64(m_amountSpin->value() * 1000000.0);
    if (quantityMicros <= 0) {
        m_ticketStatusLabel->setText(QStringLiteral("Amount must be greater than zero."));
        m_ticketStatusLabel->setStyleSheet(QStringLiteral(
            "color: #ff7b72; font-size: 13px;"));
        return;
    }

    const int priceBasisPoints = outcome->pricePercent * 100;
    m_ordersRepository->createOrder(outcome->id, m_orderSide, priceBasisPoints, quantityMicros);
}
