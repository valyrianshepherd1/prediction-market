#include "MarketDetailsPage.h"

#include <QDateTime>
#include <QDoubleSpinBox>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>
#include <QtGlobal>

namespace {

QString formatCreatedAt(const QString &createdAt) {
    const QDateTime dt = QDateTime::fromString(createdAt, Qt::ISODate);
    if (!dt.isValid()) {
        return QString();
    }
    return dt.date().toString(QStringLiteral("dd MMM yyyy"));
}

} // namespace

MarketDetailsPage::MarketDetailsPage(QWidget *parent)
    : QWidget(parent) {
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(24, 24, 24, 24);
    root->setSpacing(18);

    auto *topRow = new QHBoxLayout;
    auto *backButton = new QPushButton(QStringLiteral("Back"), this);
    backButton->setCursor(Qt::PointingHandCursor);
    backButton->setFixedWidth(120);

    topRow->addWidget(backButton, 0, Qt::AlignLeft);
    topRow->addStretch(1);
    root->addLayout(topRow);

    m_questionLabel = new QLabel(this);
    m_questionLabel->setWordWrap(true);
    m_questionLabel->setStyleSheet(QStringLiteral(
        "color: white; font-size: 28px; font-weight: 700;"));
    root->addWidget(m_questionLabel);

    m_metaLabel = new QLabel(this);
    m_metaLabel->setWordWrap(true);
    m_metaLabel->setStyleSheet(QStringLiteral(
        "color: #9fb2c7; font-size: 14px;"));
    root->addWidget(m_metaLabel);

    auto *contentRow = new QHBoxLayout;
    contentRow->setSpacing(18);
    contentRow->addStretch(1);

    auto *ticketCard = new QFrame(this);
    ticketCard->setMinimumWidth(460);
    ticketCard->setMaximumWidth(520);
    ticketCard->setStyleSheet(QStringLiteral(
        "QFrame { background: #07111d; border: 1px solid #132238; border-radius: 16px; }"));

    auto *ticketLayout = new QVBoxLayout(ticketCard);
    ticketLayout->setContentsMargins(20, 20, 20, 20);
    ticketLayout->setSpacing(16);

    auto *ticketTitle = new QLabel(QStringLiteral("Trade"), ticketCard);
    ticketTitle->setAlignment(Qt::AlignCenter);
    ticketTitle->setStyleSheet(QStringLiteral(
        "color: white; font-size: 22px; font-weight: 700;"));
    ticketLayout->addWidget(ticketTitle);

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

    connect(m_amountSpin,
        qOverload<double>(&QDoubleSpinBox::valueChanged),
        this,
        [this](double) {
            m_ticketStatusLabel->clear();
            updateTicketUi();
        });

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

    m_ticketStatusLabel = new QLabel(ticketCard);
    m_ticketStatusLabel->setWordWrap(true);
    m_ticketStatusLabel->setStyleSheet(QStringLiteral(
        "color: #9fb2c7; font-size: 13px;"));
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

    contentRow->addWidget(ticketCard, 0);
    contentRow->addStretch(1);

    root->addLayout(contentRow);

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

    connect(m_loginButton, &QPushButton::clicked, this, &MarketDetailsPage::loginRequested);
    connect(m_submitButton, &QPushButton::clicked, this, &MarketDetailsPage::submitOrder);

    updateTicketUi();
}

void MarketDetailsPage::setApiClient(MarketApiClient *api) {
    if (m_api == api) {
        return;
    }

    m_api = api;

    if (!m_api) {
        updateTicketUi();
        return;
    }

    connect(m_api, &MarketApiClient::orderBusyChanged, this, [this](bool busy) {
        m_orderBusy = busy;

        if (busy) {
            m_ticketStatusLabel->setText(QStringLiteral("Submitting order..."));
            m_ticketStatusLabel->setStyleSheet(QStringLiteral(
                "color: #9fb2c7; font-size: 13px;"));
        }

        updateTicketUi();
    });

    connect(m_api, &MarketApiClient::orderCreated, this, [this](const ApiOrder &order) {
        m_orderBusy = false;
        m_ticketStatusLabel->setText(QStringLiteral("Order placed: %1").arg(order.id));
        m_ticketStatusLabel->setStyleSheet(QStringLiteral(
            "color: #7ef0a8; font-size: 13px;"));
        updateTicketUi();
    });

    connect(m_api, &MarketApiClient::orderError, this, [this](const QString &message) {
        m_orderBusy = false;
        m_ticketStatusLabel->setText(message);
        m_ticketStatusLabel->setStyleSheet(QStringLiteral(
            "color: #ff7b72; font-size: 13px;"));
        updateTicketUi();
    });

    connect(m_api, &MarketApiClient::sessionReady, this, [this](const ApiSession &) {
        updateTicketUi();
    });

    connect(m_api, &MarketApiClient::sessionCleared, this, [this]() {
        updateTicketUi();
    });

    updateTicketUi();
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

void MarketDetailsPage::setMarket(const ApiMarket &market, const QString &preferredSelection) {
    m_market = market;
    m_selectedOutcomeId.clear();
    m_orderSide = QStringLiteral("BUY");

    const QString status = m_market.status.isEmpty()
                               ? QStringLiteral("OPEN")
                               : m_market.status;

    const QString createdAt = formatCreatedAt(m_market.createdAt);

    QString meta = status;
    if (!createdAt.isEmpty()) {
        meta += QStringLiteral(" • %1").arg(createdAt);
    }
    meta += QStringLiteral(" • %1 variants").arg(m_market.outcomes.size());

    m_questionLabel->setText(m_market.question);
    m_metaLabel->setText(meta);

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

    m_ticketStatusLabel->clear();
    m_ticketStatusLabel->setStyleSheet(QStringLiteral(
        "color: #9fb2c7; font-size: 13px;"));

    updateTicketUi();
}

void MarketDetailsPage::setOrderSide(const QString &side) {
    m_orderSide = normalizedSide(side);
    m_ticketStatusLabel->clear();
    updateTicketUi();
}

void MarketDetailsPage::selectBinaryOutcome(const QString &titleUpper) {
    const ApiOutcome *outcome = findOutcomeByTitle(titleUpper);
    if (!outcome) {
        return;
    }

    m_selectedOutcomeId = outcome->id;
    m_ticketStatusLabel->clear();
    updateTicketUi();
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

void MarketDetailsPage::updateTicketUi() {
    const ApiOutcome *outcome = selectedOutcome();

    const bool loggedIn =
    m_api && (
        m_api->isAuthenticated() ||
        !m_api->currentSession().userId.trimmed().isEmpty()
    );

    updateSideButtons();
    updateOutcomeButtons();

    const bool hasOutcome = (outcome != nullptr);

    m_amountSpin->setEnabled(hasOutcome && !m_orderBusy);

    m_loginButton->setVisible(!loggedIn);
    m_submitButton->setVisible(loggedIn);
    m_submitButton->setEnabled(loggedIn && hasOutcome && !m_orderBusy);

    if (m_orderBusy) {
        m_submitButton->setText(QStringLiteral("Submitting..."));
    } else {
        m_submitButton->setText(QStringLiteral("Trade"));
    }

    if (!loggedIn) {
        m_ticketStatusLabel->setText(QStringLiteral("Log in or Sign up to place an order."));
        m_ticketStatusLabel->setStyleSheet(QStringLiteral(
            "color: #9fb2c7; font-size: 13px;"));
    } else if (!hasOutcome) {
        m_ticketStatusLabel->setText(QStringLiteral("Choose Yes or No."));
        m_ticketStatusLabel->setStyleSheet(QStringLiteral(
            "color: #9fb2c7; font-size: 13px;"));
    } else if (m_ticketStatusLabel->text().trimmed().isEmpty()) {
        m_ticketStatusLabel->setText(
            QStringLiteral("%1 %2 for %3")
                .arg(m_orderSide == QStringLiteral("BUY") ? QStringLiteral("Buy") : QStringLiteral("Sell"))
                .arg(outcome->title)
                .arg(QString::number(m_amountSpin->value(), 'f', 2)));
        m_ticketStatusLabel->setStyleSheet(QStringLiteral(
            "color: #9fb2c7; font-size: 13px;"));
    }
}

void MarketDetailsPage::submitOrder() {
    if (!m_api || m_api->currentSession().userId.trimmed().isEmpty()) {
        emit loginRequested();
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
    m_api->createOrder(outcome->id, m_orderSide, priceBasisPoints, quantityMicros);
}