#include "MarketCardWidget.h"

#include <QDateTime>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

namespace {

QString formatCreatedAt(const QString &createdAt) {
    const QDateTime dt = QDateTime::fromString(createdAt, Qt::ISODate);
    if (!dt.isValid()) {
        return QString();
    }

    return dt.date().toString(QStringLiteral("dd MMM yyyy"));
}

}

MarketCardWidget::MarketCardWidget(QWidget *parent)
    : QFrame(parent) {
    setObjectName(QStringLiteral("MarketCard"));
    setFrameShape(QFrame::NoFrame);

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(16, 16, 16, 16);
    root->setSpacing(12);

    m_meta = new QLabel(this);
    m_meta->setObjectName(QStringLiteral("MarketVolume"));
    m_meta->setStyleSheet(QStringLiteral("color: #7f93aa; font-size: 12px;"));
    root->addWidget(m_meta);

    m_question = new QLabel(this);
    m_question->setWordWrap(true);
    m_question->setObjectName(QStringLiteral("MarketQuestion"));
    m_question->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    m_question->setMinimumHeight(56);
    root->addWidget(m_question);

    m_variantsContainer = new QWidget(this);
    m_variantsLayout = new QVBoxLayout(m_variantsContainer);
    m_variantsLayout->setContentsMargins(0, 0, 0, 0);
    m_variantsLayout->setSpacing(8);
    root->addWidget(m_variantsContainer);

    m_moreLabel = new QLabel(this);
    m_moreLabel->setStyleSheet(QStringLiteral("color: #8ca0b8; font-size: 12px;"));
    m_moreLabel->hide();
    root->addWidget(m_moreLabel);

    root->addStretch(1);

    auto *buttons = new QHBoxLayout;
    buttons->setSpacing(10);

    m_yesButton = new QPushButton(QStringLiteral("Yes"), this);
    m_noButton = new QPushButton(QStringLiteral("No"), this);

    m_yesButton->setCursor(Qt::PointingHandCursor);
    m_noButton->setCursor(Qt::PointingHandCursor);

    m_yesButton->setStyleSheet(QStringLiteral(
        "QPushButton { background: #123222; border: 1px solid #1f7a45; color: #7ef0a8; "
        "padding: 10px 14px; border-radius: 10px; font-weight: 600; }"
        "QPushButton:hover { background: #17452d; }"));

    m_noButton->setStyleSheet(QStringLiteral(
        "QPushButton { background: #34181c; border: 1px solid #8b2f39; color: #ff8f9b; "
        "padding: 10px 14px; border-radius: 10px; font-weight: 600; }"
        "QPushButton:hover { background: #472126; }"));

    buttons->addWidget(m_yesButton);
    buttons->addWidget(m_noButton);

    root->addLayout(buttons);

    connect(m_yesButton, &QPushButton::clicked, this, &MarketCardWidget::yesClicked);
    connect(m_noButton, &QPushButton::clicked, this, &MarketCardWidget::noClicked);
}

QString MarketCardWidget::metaText(const ApiMarket &market) const {
    const QString created = formatCreatedAt(market.createdAt);
    const QString status = market.status.isEmpty() ? QStringLiteral("OPEN") : market.status;
    const int variantCount = market.outcomes.size();

    if (created.isEmpty()) {
        return QStringLiteral("%1 • %2 variant%3")
            .arg(status)
            .arg(variantCount)
            .arg(variantCount == 1 ? QString() : QStringLiteral("s"));
    }

    return QStringLiteral("%1 • %2 • %3 variant%4")
        .arg(status)
        .arg(created)
        .arg(variantCount)
        .arg(variantCount == 1 ? QString() : QStringLiteral("s"));
}

QString MarketCardWidget::noTextFor(int yesPercent) const {
    return QStringLiteral("%1%").arg(qBound(0, 100 - yesPercent, 100));
}

void MarketCardWidget::clearVariantRows() {
    while (m_variantsLayout->count()) {
        auto *item = m_variantsLayout->takeAt(0);
        delete item->widget();
        delete item;
    }
}

void MarketCardWidget::addVariantPreview(const ApiOutcome &outcome) {
    auto *row = new QWidget(m_variantsContainer);
    auto *h = new QHBoxLayout(row);
    h->setContentsMargins(0, 0, 0, 0);
    h->setSpacing(8);

    auto *name = new QLabel(outcome.title, row);
    name->setStyleSheet(QStringLiteral("color: #dbe7f3; font-size: 13px;"));
    name->setWordWrap(true);

    auto *yesBadge = new QLabel(QStringLiteral("Y %1%").arg(outcome.pricePercent), row);
    yesBadge->setStyleSheet(QStringLiteral(
        "color: #7ef0a8; background: #123222; border: 1px solid #1f7a45; "
        "border-radius: 8px; padding: 3px 8px; font-size: 12px; font-weight: 600;"));

    auto *noBadge = new QLabel(QStringLiteral("N %1").arg(noTextFor(outcome.pricePercent)), row);
    noBadge->setStyleSheet(QStringLiteral(
        "color: #ff8f9b; background: #34181c; border: 1px solid #8b2f39; "
        "border-radius: 8px; padding: 3px 8px; font-size: 12px; font-weight: 600;"));

    h->addWidget(name, 1);
    h->addWidget(yesBadge);
    h->addWidget(noBadge);

    m_variantsLayout->addWidget(row);
}

void MarketCardWidget::setMarket(const ApiMarket &market) {
    m_question->setText(market.question);
    m_meta->setText(metaText(market));

    clearVariantRows();

    const int previewCount = qMin(3, market.outcomes.size());
    for (int i = 0; i < previewCount; ++i) {
        addVariantPreview(market.outcomes[i]);
    }

    const int hiddenCount = market.outcomes.size() - previewCount;
    if (hiddenCount > 0) {
        m_moreLabel->setText(QStringLiteral("+ %1 more variant%2")
                                 .arg(hiddenCount)
                                 .arg(hiddenCount == 1 ? QString() : QStringLiteral("s")));
        m_moreLabel->show();
    } else {
        m_moreLabel->hide();
    }
}