#include "MarketDetailsPage.h"

#include <QDateTime>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QWidget>
#include <QtGlobal>

namespace {

QString formatPercentText(double value) {
    return QString::number(value, 'f', 2) + QStringLiteral("%");
}

} // namespace

MarketDetailsPage::MarketDetailsPage(QWidget *parent)
    : QWidget(parent) {
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(24, 24, 24, 24);
    root->setSpacing(16);

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
    contentRow->setSpacing(20);

    m_scroll = new QScrollArea(this);
    m_scroll->setWidgetResizable(true);
    m_scroll->setFrameShape(QFrame::NoFrame);
    m_scroll->setMinimumWidth(0);

    m_listContainer = new QWidget(this);
    m_listLayout = new QVBoxLayout(m_listContainer);
    m_listLayout->setContentsMargins(0, 0, 0, 0);
    m_listLayout->setSpacing(12);

    m_scroll->setWidget(m_listContainer);
    contentRow->addWidget(m_scroll, 1);

    auto *ticketCard = new QFrame(this);
    ticketCard->setObjectName(QStringLiteral("TradeTicketCard"));
    ticketCard->setMinimumWidth(360);
    ticketCard->setMaximumWidth(420);
    ticketCard->setStyleSheet(R"(
        #TradeTicketCard {
            background: #07111d;
            border: 1px solid #132238;
            border-radius: 16px;
        }
        #TradeTicketCard QLabel {
            background: transparent;
            border: none;
        }
    )");

    auto *ticketLayout = new QVBoxLayout(ticketCard);
    ticketLayout->setContentsMargins(20, 20, 20, 20);
    ticketLayout->setSpacing(14);

    auto *ticketTitle = new QLabel(QStringLiteral("Trade"), ticketCard);
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

    m_binaryChoiceContainer = new QWidget(ticketCard);
    auto *binaryLayout = new QHBoxLayout(m_binaryChoiceContainer);
    binaryLayout->setContentsMargins(0, 0, 0, 0);
    binaryLayout->setSpacing(10);

    m_yesChoiceButton = new QPushButton(QStringLiteral("Yes"), m_binaryChoiceContainer);
    m_noChoiceButton = new QPushButton(QStringLiteral("No"), m_binaryChoiceContainer);

    m_yesChoiceButton->setCursor(Qt::PointingHandCursor);
    m_noChoiceButton->setCursor(Qt::PointingHandCursor);

    binaryLayout->addWidget(m_yesChoiceButton);
    binaryLayout->addWidget(m_noChoiceButton);

    ticketLayout->addWidget(m_binaryChoiceContainer);

    auto *form = new QFormLayout;
    form->setContentsMargins(0, 0, 0, 0);
    form->setSpacing(10);

    m_selectedVariantValue = new QLabel(QStringLiteral("Choose a variant"), ticketCard);
    m_selectedVariantValue->setWordWrap(true);
    m_selectedVariantValue->setStyleSheet(QStringLiteral(
        "color: white; font-size: 16px; font-weight: 600;"));

    m_priceSpin = new QDoubleSpinBox(ticketCard);
    m_priceSpin->setDecimals(2);
    m_priceSpin->setRange(0.01, 99.99);
    m_priceSpin->setSingleStep(0.25);
    m_priceSpin->setSuffix(QStringLiteral("%"));

    m_amountSpin = new QDoubleSpinBox(ticketCard);
    m_amountSpin->setDecimals(6);
    m_amountSpin->setRange(0.000001, 1000000.0);
    m_amountSpin->setSingleStep(1.0);
    m_amountSpin->setValue(1.0);

    form->addRow(QStringLiteral("Variant"), m_selectedVariantValue);
    form->addRow(QStringLiteral("Price"), m_priceSpin);
    form->addRow(QStringLiteral("Amount"), m_amountSpin);

    ticketLayout->addLayout(form);

    auto *amountTitle = new QLabel(QStringLiteral("Amount"), ticketCard);
    amountTitle->setStyleSheet(QStringLiteral(
        "color: white; font-size: 16px; font-weight: 600;"));
    ticketLayout->addWidget(amountTitle);

    m_amountValueLabel = new QLabel(QStringLiteral("1.00"), ticketCard);
    m_amountValueLabel->setAlignment(Qt::AlignRight);
    m_amountValueLabel->setStyleSheet(QStringLiteral(
        "color: #5e7592; font-size: 48px; font-weight: 600;"));
    ticketLayout->addWidget(m_amountValueLabel);

    auto *quickRow = new QHBoxLayout;
    quickRow->setSpacing(8);

    auto addQuickButton = [this, quickRow, ticketCard](const QString &text, double delta) {
        auto *button = new QPushButton(text, ticketCard);
        button->setCursor(Qt::PointingHandCursor);
        button->setStyleSheet(QStringLiteral(
            "QPushButton { background: #1a2430; border: 1px solid #243447; color: white; "
            "padding: 8px 12px; border-radius: 10px; font-weight: 600; }"
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

    ticketLayout->addStretch(1);

    contentRow->addWidget(ticketCard, 0);
    root->addLayout(contentRow, 1);

    connect(backButton, &QPushButton::clicked, this, &MarketDetailsPage::backRequested);

    connect(m_buyButton, &QPushButton::clicked, this, [this]() {
        setOrderSide(QStringLiteral("BUY"));
    });
    connect(m_sellButton, &QPushButton::clicked, this, [this]() {
        setOrderSide(QStringLiteral("SELL"));
    });

    connect(m_yesChoiceButton, &QPushButton::clicked, this, [this]() {
        selectBinaryOutcome(QStringLiteral("YES"));
    });
    connect(m_noChoiceButton, &QPushButton::clicked, this, [this]() {
        selectBinaryOutcome(QStringLiteral("NO"));
    });

    connect(m_loginButton, &QPushButton::clicked, this, &MarketDetailsPage::loginRequested);
    connect(m_submitButton, &QPushButton::clicked, this, &MarketDetailsPage::submitOrder);

    connect(m_amountSpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this](double) {
        syncAmountDisplay();
        updateTicketUi();
    });

    connect(m_priceSpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this](double) {
        updateTicketUi();
    });

    syncAmountDisplay();
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

        if (m_api) {
            m_api->fetchWallet();
            if (m_api->isAuthenticated()) {
                m_api->fetchMyTrades();
            }
        }

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

QString MarketDetailsPage::formatCreatedAt(const QString &createdAt) const {
    const QDateTime dt = QDateTime::fromString(createdAt, Qt::ISODate);
    if (!dt.isValid()) {
        return QString();
    }

    return dt.date().toString(QStringLiteral("dd MMM yyyy"));
}

QString MarketDetailsPage::centsText(int pricePercent) const {
    return QStringLiteral("%1¢").arg(qBound(0, pricePercent, 100));
}

QString MarketDetailsPage::outcomeButtonText(const QString &label, int pricePercent) const {
    return QStringLiteral("%1 %2").arg(label, centsText(pricePercent));
}

bool MarketDetailsPage::isBinaryYesNoMarket() const {
    if (m_market.outcomes.size() != 2) {
        return false;
    }

    bool hasYes = false;
    bool hasNo = false;

    for (const ApiOutcome &outcome : m_market.outcomes) {
        const QString title = outcome.title.trimmed().toUpper();
        if (title == QStringLiteral("YES")) {
            hasYes = true;
        } else if (title == QStringLiteral("NO")) {
            hasNo = true;
        }
    }

    return hasYes && hasNo;
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

    const QString normalized = preferredSelection.trimmed().toUpper();

    if (normalized == QStringLiteral("BUY") || normalized == QStringLiteral("SELL")) {
        m_orderSide = normalized;
    }

    if (isBinaryYesNoMarket()) {
        if (normalized == QStringLiteral("YES") || normalized == QStringLiteral("NO")) {
            const ApiOutcome *preferred = findOutcomeByTitle(normalized);
            if (preferred) {
                m_selectedOutcomeId = preferred->id;
            }
        }

        if (m_selectedOutcomeId.isEmpty()) {
            const ApiOutcome *yesOutcome = findOutcomeByTitle(QStringLiteral("YES"));
            if (yesOutcome) {
                m_selectedOutcomeId = yesOutcome->id;
            }
        }
    } else {
        if (!m_market.outcomes.isEmpty()) {
            m_selectedOutcomeId = m_market.outcomes.first().id;
        }
    }

    const ApiOutcome *outcome = selectedOutcome();
    if (outcome) {
        m_priceSpin->setValue(static_cast<double>(outcome->pricePercent));
    }

    m_ticketStatusLabel->clear();
    m_ticketStatusLabel->setStyleSheet(QStringLiteral(
        "color: #9fb2c7; font-size: 13px;"));

    render();
}

void MarketDetailsPage::clearOutcomeRows() {
    while (m_listLayout->count()) {
        auto *item = m_listLayout->takeAt(0);
        if (item->widget()) {
            delete item->widget();
        }
        delete item;
    }
}

QWidget *MarketDetailsPage::createOutcomeRow(const ApiOutcome &outcome) {
    const bool isSelected = outcome.id == m_selectedOutcomeId;

    auto *card = new QFrame(m_listContainer);
    card->setObjectName(QStringLiteral("OutcomeRowCard"));
    card->setStyleSheet(QStringLiteral(R"(
        #OutcomeRowCard {
            background: %1;
            border: 1px solid %2;
            border-radius: 16px;
        }
        #OutcomeRowCard QLabel {
            background: transparent;
            border: none;
        }
    )")
        .arg(isSelected ? QStringLiteral("#0d1b2a") : QStringLiteral("#07111d"))
        .arg(isSelected ? QStringLiteral("#2f5f94") : QStringLiteral("#132238")));

    auto *layout = new QHBoxLayout(card);
    layout->setContentsMargins(18, 18, 18, 18);
    layout->setSpacing(14);

    auto *left = new QVBoxLayout;
    left->setSpacing(6);

    auto *title = new QLabel(outcome.title, card);
    title->setWordWrap(true);
    title->setStyleSheet(QStringLiteral(
        "color: white; font-size: 18px; font-weight: 600;"));

    auto *subtitle = new QLabel(QStringLiteral("Current price %1").arg(centsText(outcome.pricePercent)), card);
    subtitle->setWordWrap(true);
    subtitle->setStyleSheet(QStringLiteral(
        "color: #9fb2c7; font-size: 13px;"));

    left->addWidget(title);
    left->addWidget(subtitle);

    auto *selectButton = new QPushButton(isSelected ? QStringLiteral("Selected")
                                                    : QStringLiteral("Select"),
                                         card);
    selectButton->setCursor(Qt::PointingHandCursor);
    selectButton->setMinimumWidth(110);
    selectButton->setStyleSheet(QStringLiteral(
        "QPushButton { background: %1; border: 1px solid %2; color: %3; "
        "padding: 10px 14px; border-radius: 10px; font-weight: 700; }"
        "QPushButton:hover { background: %4; }")
        .arg(isSelected ? QStringLiteral("#17452d") : QStringLiteral("#1a2430"))
        .arg(isSelected ? QStringLiteral("#1f7a45") : QStringLiteral("#243447"))
        .arg(isSelected ? QStringLiteral("#7ef0a8") : QStringLiteral("white"))
        .arg(isSelected ? QStringLiteral("#1d5637") : QStringLiteral("#243243")));

    connect(selectButton, &QPushButton::clicked, this, [this, outcome]() {
        selectOutcome(outcome.id);
    });

    layout->addLayout(left, 1);
    layout->addWidget(selectButton, 0, Qt::AlignVCenter);

    return card;
}

void MarketDetailsPage::selectOutcome(const QString &outcomeId) {
    m_selectedOutcomeId = outcomeId;

    const ApiOutcome *outcome = selectedOutcome();
    if (outcome) {
        m_priceSpin->setValue(static_cast<double>(outcome->pricePercent));
    }

    m_ticketStatusLabel->clear();
    m_ticketStatusLabel->setStyleSheet(QStringLiteral(
        "color: #9fb2c7; font-size: 13px;"));

    render();
}

void MarketDetailsPage::selectBinaryOutcome(const QString &titleUpper) {
    const ApiOutcome *outcome = findOutcomeByTitle(titleUpper);
    if (!outcome) {
        return;
    }

    m_selectedOutcomeId = outcome->id;
    m_priceSpin->setValue(static_cast<double>(outcome->pricePercent));

    m_ticketStatusLabel->clear();
    m_ticketStatusLabel->setStyleSheet(QStringLiteral(
        "color: #9fb2c7; font-size: 13px;"));

    updateBinaryChoiceButtons();
    updateTicketUi();
}

void MarketDetailsPage::setOrderSide(const QString &side) {
    m_orderSide = normalizedSide(side);
    updateTicketUi();
}

void MarketDetailsPage::syncAmountDisplay() {
    m_amountValueLabel->setText(QString::number(m_amountSpin->value(), 'f', 2));
}

void MarketDetailsPage::updateBinaryChoiceButtons() {
    const bool binary = isBinaryYesNoMarket();
    m_binaryChoiceContainer->setVisible(binary);

    if (!binary) {
        return;
    }

    const ApiOutcome *yesOutcome = findOutcomeByTitle(QStringLiteral("YES"));
    const ApiOutcome *noOutcome = findOutcomeByTitle(QStringLiteral("NO"));

    if (!yesOutcome || !noOutcome) {
        m_binaryChoiceContainer->setVisible(false);
        return;
    }

    const bool yesSelected = m_selectedOutcomeId == yesOutcome->id;
    const bool noSelected = m_selectedOutcomeId == noOutcome->id;

    m_yesChoiceButton->setText(outcomeButtonText(QStringLiteral("Yes"), yesOutcome->pricePercent));
    m_noChoiceButton->setText(outcomeButtonText(QStringLiteral("No"), noOutcome->pricePercent));

    m_yesChoiceButton->setStyleSheet(QStringLiteral(
        "QPushButton { background: %1; border: 1px solid #1f7a45; color: %2; "
        "padding: 14px 16px; border-radius: 12px; font-size: 18px; font-weight: 700; }"
        "QPushButton:hover { background: #17452d; }")
        .arg(yesSelected ? QStringLiteral("#17452d") : QStringLiteral("#123222"))
        .arg(QStringLiteral("#7ef0a8")));

    m_noChoiceButton->setStyleSheet(QStringLiteral(
        "QPushButton { background: %1; border: 1px solid #8b2f39; color: %2; "
        "padding: 14px 16px; border-radius: 12px; font-size: 18px; font-weight: 700; }"
        "QPushButton:hover { background: #472126; }")
        .arg(noSelected ? QStringLiteral("#472126") : QStringLiteral("#34181c"))
        .arg(QStringLiteral("#ff8f9b")));
}

void MarketDetailsPage::updateTicketUi() {
    const ApiOutcome *outcome = selectedOutcome();
    const bool loggedIn = m_api && m_api->isAuthenticated();
    const bool hasSelection = outcome != nullptr;
    const bool isBuy = m_orderSide == QStringLiteral("BUY");

    m_selectedVariantValue->setText(
        hasSelection ? outcome->title : QStringLiteral("Choose a variant"));

    m_buyButton->setStyleSheet(QStringLiteral(
        "QPushButton { background: %1; border: 1px solid %2; color: %3; "
        "padding: 10px 14px; border-radius: 10px; font-size: 18px; font-weight: 700; }"
        "QPushButton:hover { background: %4; }")
        .arg(isBuy ? QStringLiteral("#123222") : QStringLiteral("#1a2430"))
        .arg(isBuy ? QStringLiteral("#1f7a45") : QStringLiteral("#243447"))
        .arg(isBuy ? QStringLiteral("#7ef0a8") : QStringLiteral("#8ea4bc"))
        .arg(isBuy ? QStringLiteral("#17452d") : QStringLiteral("#243243")));

    m_sellButton->setStyleSheet(QStringLiteral(
        "QPushButton { background: %1; border: 1px solid %2; color: %3; "
        "padding: 10px 14px; border-radius: 10px; font-size: 18px; font-weight: 700; }"
        "QPushButton:hover { background: %4; }")
        .arg(!isBuy ? QStringLiteral("#34181c") : QStringLiteral("#1a2430"))
        .arg(!isBuy ? QStringLiteral("#8b2f39") : QStringLiteral("#243447"))
        .arg(!isBuy ? QStringLiteral("#ff8f9b") : QStringLiteral("#8ea4bc"))
        .arg(!isBuy ? QStringLiteral("#472126") : QStringLiteral("#243243")));

    updateBinaryChoiceButtons();

    const bool canSubmit = loggedIn && hasSelection && !m_orderBusy;

    m_priceSpin->setEnabled(canSubmit);
    m_amountSpin->setEnabled(canSubmit);
    m_submitButton->setEnabled(canSubmit);

    m_loginButton->setVisible(!loggedIn);
    m_submitButton->setVisible(loggedIn);

    if (m_orderBusy) {
        m_submitButton->setText(QStringLiteral("Submitting..."));
    } else {
        m_submitButton->setText(QStringLiteral("Trade"));
    }

    if (!loggedIn) {
        m_ticketStatusLabel->setText(QStringLiteral("Log in or Sign up to place an order."));
        m_ticketStatusLabel->setStyleSheet(QStringLiteral(
            "color: #9fb2c7; font-size: 13px;"));
    } else if (!hasSelection) {
        m_ticketStatusLabel->setText(QStringLiteral("Choose a variant to continue."));
        m_ticketStatusLabel->setStyleSheet(QStringLiteral(
            "color: #9fb2c7; font-size: 13px;"));
    } else if (m_ticketStatusLabel->text().trimmed().isEmpty()) {
        m_ticketStatusLabel->setText(
            QStringLiteral("%1 %2 at %3.")
                .arg(isBuy ? QStringLiteral("Buy") : QStringLiteral("Sell"))
                .arg(outcome->title)
                .arg(formatPercentText(m_priceSpin->value())));
        m_ticketStatusLabel->setStyleSheet(QStringLiteral(
            "color: #9fb2c7; font-size: 13px;"));
    }
}

void MarketDetailsPage::submitOrder() {
    if (!m_api || !m_api->isAuthenticated()) {
        emit loginRequested();
        return;
    }

    const ApiOutcome *outcome = selectedOutcome();
    if (!outcome) {
        m_ticketStatusLabel->setText(QStringLiteral("Choose a variant first."));
        m_ticketStatusLabel->setStyleSheet(QStringLiteral(
            "color: #ff7b72; font-size: 13px;"));
        return;
    }

    const int priceBasisPoints = qRound(m_priceSpin->value() * 100.0);
    const qint64 quantityMicros = qRound64(m_amountSpin->value() * 1000000.0);

    if (priceBasisPoints <= 0 || priceBasisPoints >= 10000) {
        m_ticketStatusLabel->setText(QStringLiteral("Price must be between 0.01% and 99.99%."));
        m_ticketStatusLabel->setStyleSheet(QStringLiteral(
            "color: #ff7b72; font-size: 13px;"));
        return;
    }

    if (quantityMicros <= 0) {
        m_ticketStatusLabel->setText(QStringLiteral("Amount must be greater than zero."));
        m_ticketStatusLabel->setStyleSheet(QStringLiteral(
            "color: #ff7b72; font-size: 13px;"));
        return;
    }

    m_api->createOrder(outcome->id, m_orderSide, priceBasisPoints, quantityMicros);
}

void MarketDetailsPage::render() {
    m_questionLabel->setText(
        m_market.question.isEmpty() ? QStringLiteral("Market") : m_market.question);

    const QString status = m_market.status.isEmpty()
                               ? QStringLiteral("OPEN")
                               : m_market.status;
    const QString createdAt = formatCreatedAt(m_market.createdAt);

    QString meta = status;
    if (!createdAt.isEmpty()) {
        meta += QStringLiteral(" • %1").arg(createdAt);
    }
    meta += QStringLiteral(" • %1 variant%2")
                .arg(m_market.outcomes.size())
                .arg(m_market.outcomes.size() == 1 ? QString() : QStringLiteral("s"));

    m_metaLabel->setText(meta);

    if (m_selectedOutcomeId.isEmpty() && !m_market.outcomes.isEmpty()) {
        m_selectedOutcomeId = m_market.outcomes.first().id;
        m_priceSpin->setValue(static_cast<double>(m_market.outcomes.first().pricePercent));
    }

    clearOutcomeRows();

    if (isBinaryYesNoMarket()) {
        m_scroll->hide();
    } else {
        m_scroll->show();

        for (const ApiOutcome &outcome : m_market.outcomes) {
            m_listLayout->addWidget(createOutcomeRow(outcome));
        }

        m_listLayout->addStretch(1);
    }

    syncAmountDisplay();
    updateTicketUi();
}

void MarketApiClient::createOrder(const QString &outcomeId,
                                  const QString &side,
                                  int priceBasisPoints,
                                  qint64 quantityMicros) {
    if (!isAuthenticated()) {
        emit orderError(QStringLiteral("Log in or Sign up to place an order."));
        return;
    }

    const QString normalizedSide = side.trimmed().toUpper();

    if (outcomeId.trimmed().isEmpty()) {
        emit orderError(QStringLiteral("No outcome is selected."));
        return;
    }

    if (normalizedSide != QStringLiteral("BUY") &&
        normalizedSide != QStringLiteral("SELL")) {
        emit orderError(QStringLiteral("Order side must be BUY or SELL."));
        return;
    }

    if (priceBasisPoints <= 0 || priceBasisPoints >= 10000) {
        emit orderError(QStringLiteral("Price must be between 0.01% and 99.99%."));
        return;
    }

    if (quantityMicros <= 0) {
        emit orderError(QStringLiteral("Quantity must be greater than zero."));
        return;
    }

    emit orderBusyChanged(true);

    QJsonObject body;
    body.insert(QStringLiteral("outcome_id"), outcomeId);
    body.insert(QStringLiteral("side"), normalizedSide);
    body.insert(QStringLiteral("price_bp"), priceBasisPoints);
    body.insert(QStringLiteral("qty_micros"), QJsonValue(static_cast<double>(quantityMicros)));

    postJson(apiUrl(QStringLiteral("/orders")),
             body,
             true,
             [this](const QJsonDocument &document) {
                 emit orderBusyChanged(false);

                 if (!document.isObject()) {
                     emit orderError(QStringLiteral("/orders did not return an object."));
                     return;
                 }

                 const QJsonObject object = document.object();

                 ApiOrder order;
                 order.id = object.value(QStringLiteral("id")).toString();
                 order.userId = object.value(QStringLiteral("user_id")).toString();
                 order.outcomeId = object.value(QStringLiteral("outcome_id")).toString();
                 order.side = object.value(QStringLiteral("side")).toString();
                 order.priceBasisPoints = object.value(QStringLiteral("price_bp")).toInt();
                 order.quantityTotalMicros =
                     object.value(QStringLiteral("qty_total_micros")).toVariant().toLongLong();
                 order.quantityRemainingMicros =
                     object.value(QStringLiteral("qty_remaining_micros")).toVariant().toLongLong();
                 order.status = object.value(QStringLiteral("status")).toString();
                 order.createdAt = object.value(QStringLiteral("created_at")).toString();
                 order.updatedAt = object.value(QStringLiteral("updated_at")).toString();

                 emit orderCreated(order);
             },
             [this, outcomeId, normalizedSide, priceBasisPoints, quantityMicros](int statusCode,
                                                                                 const QString &message) {
                 if (statusCode == 401 && !m_refreshToken.trimmed().isEmpty()) {
                     refreshSession(
                         [this, outcomeId, normalizedSide, priceBasisPoints, quantityMicros]() {
                             createOrder(outcomeId, normalizedSide, priceBasisPoints, quantityMicros);
                         },
                         [this](const QString &refreshMessage) {
                             emit orderBusyChanged(false);
                             emit orderError(QStringLiteral("Session expired.\n%1").arg(refreshMessage));
                         });
                     return;
                 }

                 emit orderBusyChanged(false);
                 emit orderError(QStringLiteral("Could not place order.\n%1").arg(message));
             });
}