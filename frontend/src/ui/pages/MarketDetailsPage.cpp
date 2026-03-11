#include "MarketDetailsPage.h"

#include <QDateTime>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QWidget>

MarketDetailsPage::MarketDetailsPage(QWidget *parent)
    : QWidget(parent) {
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(24, 24, 24, 24);
    root->setSpacing(14);

    auto *topRow = new QHBoxLayout;
    auto *backButton = new QPushButton(QStringLiteral("Back"), this);
    backButton->setCursor(Qt::PointingHandCursor);
    backButton->setFixedWidth(120);

    topRow->addWidget(backButton, 0, Qt::AlignLeft);
    topRow->addStretch(1);
    root->addLayout(topRow);

    m_questionLabel = new QLabel(this);
    m_questionLabel->setWordWrap(true);
    m_questionLabel->setStyleSheet(QStringLiteral("color: white; font-size: 28px; font-weight: 700;"));
    root->addWidget(m_questionLabel);

    m_metaLabel = new QLabel(this);
    m_metaLabel->setWordWrap(true);
    m_metaLabel->setStyleSheet(QStringLiteral("color: #9fb2c7; font-size: 14px;"));
    root->addWidget(m_metaLabel);

    m_hintLabel = new QLabel(QStringLiteral("Choose a variant and side."), this);
    m_hintLabel->setWordWrap(true);
    m_hintLabel->setStyleSheet(QStringLiteral("color: #c8d7e6; font-size: 14px;"));
    root->addWidget(m_hintLabel);

    m_scroll = new QScrollArea(this);
    m_scroll->setWidgetResizable(true);
    m_scroll->setFrameShape(QFrame::NoFrame);

    m_listContainer = new QWidget(this);
    m_listLayout = new QVBoxLayout(m_listContainer);
    m_listLayout->setContentsMargins(0, 0, 0, 0);
    m_listLayout->setSpacing(12);

    m_scroll->setWidget(m_listContainer);
    root->addWidget(m_scroll, 1);

    connect(backButton, &QPushButton::clicked, this, &MarketDetailsPage::backRequested);
}

QString MarketDetailsPage::formatCreatedAt(const QString &createdAt) const {
    const QDateTime dt = QDateTime::fromString(createdAt, Qt::ISODate);
    if (!dt.isValid()) {
        return QString();
    }

    return dt.date().toString(QStringLiteral("dd MMM yyyy"));
}

QString MarketDetailsPage::noTextFor(int yesPercent) const {
    return QStringLiteral("%1%").arg(qBound(0, 100 - yesPercent, 100));
}

void MarketDetailsPage::setMarket(const ApiMarket &market, const QString &preferredSide) {
    m_market = market;
    m_preferredSide = preferredSide.trimmed().toUpper();
    render();
}

void MarketDetailsPage::clearOutcomeRows() {
    while (m_listLayout->count()) {
        auto *item = m_listLayout->takeAt(0);
        delete item->widget();
        delete item;
    }
}

QWidget *MarketDetailsPage::createOutcomeRow(const ApiOutcome &outcome) {
    auto *card = new QFrame(m_listContainer);
    card->setStyleSheet(QStringLiteral(
        "QFrame { background: #07111d; border: 1px solid #132238; border-radius: 16px; }"));

    auto *layout = new QHBoxLayout(card);
    layout->setContentsMargins(18, 18, 18, 18);
    layout->setSpacing(16);

    auto *left = new QVBoxLayout;
    left->setSpacing(8);

    auto *title = new QLabel(outcome.title, card);
    title->setWordWrap(true);
    title->setStyleSheet(QStringLiteral("color: white; font-size: 18px; font-weight: 600;"));

    auto *prices = new QLabel(
        QStringLiteral("YES %1%   •   NO %2")
            .arg(outcome.pricePercent)
            .arg(noTextFor(outcome.pricePercent)),
        card);
    prices->setStyleSheet(QStringLiteral("color: #9fb2c7; font-size: 14px;"));

    left->addWidget(title);
    left->addWidget(prices);

    auto *right = new QHBoxLayout;
    right->setSpacing(10);

    auto *yesButton = new QPushButton(QStringLiteral("Yes"), card);
    auto *noButton = new QPushButton(QStringLiteral("No"), card);

    yesButton->setCursor(Qt::PointingHandCursor);
    noButton->setCursor(Qt::PointingHandCursor);

    const bool preferYes = (m_preferredSide == QStringLiteral("YES"));
    const bool preferNo = (m_preferredSide == QStringLiteral("NO"));

    yesButton->setStyleSheet(QStringLiteral(
        "QPushButton { background: %1; border: 1px solid #1f7a45; color: #7ef0a8; "
        "padding: 10px 18px; border-radius: 10px; font-weight: 700; }"
        "QPushButton:hover { background: #17452d; }")
        .arg(preferYes ? QStringLiteral("#17452d") : QStringLiteral("#123222")));

    noButton->setStyleSheet(QStringLiteral(
        "QPushButton { background: %1; border: 1px solid #8b2f39; color: #ff8f9b; "
        "padding: 10px 18px; border-radius: 10px; font-weight: 700; }"
        "QPushButton:hover { background: #472126; }")
        .arg(preferNo ? QStringLiteral("#472126") : QStringLiteral("#34181c")));

    connect(yesButton, &QPushButton::clicked, this, [this, outcome]() {
        m_hintLabel->setText(QStringLiteral("Ready to bet YES on: %1").arg(outcome.title));
    });

    connect(noButton, &QPushButton::clicked, this, [this, outcome]() {
        m_hintLabel->setText(QStringLiteral("Ready to bet NO on: %1").arg(outcome.title));
    });

    right->addWidget(yesButton);
    right->addWidget(noButton);

    layout->addLayout(left, 1);
    layout->addLayout(right, 0);

    return card;
}

void MarketDetailsPage::render() {
    m_questionLabel->setText(m_market.question);

    const QString status = m_market.status.isEmpty() ? QStringLiteral("OPEN") : m_market.status;
    const QString createdAt = formatCreatedAt(m_market.createdAt);

    QString meta = status;
    if (!createdAt.isEmpty()) {
        meta += QStringLiteral(" • Created %1").arg(createdAt);
    }
    meta += QStringLiteral(" • %1 variant%2")
        .arg(m_market.outcomes.size())
        .arg(m_market.outcomes.size() == 1 ? QString() : QStringLiteral("s"));

    m_metaLabel->setText(meta);

    if (m_preferredSide == QStringLiteral("YES")) {
        m_hintLabel->setText(QStringLiteral("Choose which variant you want to bet YES on."));
    } else if (m_preferredSide == QStringLiteral("NO")) {
        m_hintLabel->setText(QStringLiteral("Choose which variant you want to bet NO on."));
    } else {
        m_hintLabel->setText(QStringLiteral("Choose a variant and side."));
    }

    clearOutcomeRows();

    for (const ApiOutcome &outcome : m_market.outcomes) {
        m_listLayout->addWidget(createOutcomeRow(outcome));
    }

    m_listLayout->addStretch(1);
}