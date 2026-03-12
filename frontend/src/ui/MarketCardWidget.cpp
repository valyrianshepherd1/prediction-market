#include "../../include/frontend/ui/MarketCardWidget.h"

#include <QDateTime>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPushButton>
#include <QVBoxLayout>

namespace {

QString formatCreatedAt(const QString &createdAt) {
    const QDateTime dt = QDateTime::fromString(createdAt, Qt::ISODate);
    if (!dt.isValid()) {
        return QString();
    }

    return dt.date().toString(QStringLiteral("dd MMM yyyy"));
}

QString percentText(int value) {
    return QStringLiteral("%1%").arg(qBound(0, value, 100));
}

} // namespace

MarketCardWidget::MarketCardWidget(QWidget *parent)
    : QFrame(parent) {
    setObjectName(QStringLiteral("MarketCard"));
    setFrameShape(QFrame::NoFrame);
    setCursor(Qt::PointingHandCursor);

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(18, 18, 18, 18);
    root->setSpacing(16);

    m_meta = new QLabel(this);
    m_meta->setObjectName(QStringLiteral("MarketVolume"));
    m_meta->setAlignment(Qt::AlignCenter);
    m_meta->setAttribute(Qt::WA_TransparentForMouseEvents);
    m_meta->setAttribute(Qt::WA_StyledBackground, true);
    m_meta->setStyleSheet(QStringLiteral(
        "QLabel {"
        "  color: #d7e3ef;"
        "  font-size: 12px;"
        "  background-color: rgba(0, 0, 0, 40);"
        "  border: none;"
        "  border-radius: 10px;"
        "  padding: 6px 10px;"
        "}"));
    root->addWidget(m_meta);

    m_question = new QLabel(this);
    m_question->setObjectName(QStringLiteral("MarketQuestion"));
    m_question->setAlignment(Qt::AlignCenter);
    m_question->setWordWrap(true);
    m_question->setMinimumHeight(84);
    m_question->setAttribute(Qt::WA_TransparentForMouseEvents);
    m_question->setAttribute(Qt::WA_StyledBackground, true);
    m_question->setStyleSheet(QStringLiteral(
        "QLabel {"
        "  color: white;"
        "  font-size: 18px;"
        "  font-weight: 700;"
        "  background-color: rgba(0, 0, 0, 40);"
        "  border: none;"
        "  border-radius: 16px;"
        "  padding: 16px 18px;"
        "}"));
    root->addWidget(m_question);

    root->addStretch(1);

    auto *percentRow = new QHBoxLayout;
    percentRow->setSpacing(12);

    m_yesValue = new QPushButton(this);
    m_noValue = new QPushButton(this);

    m_yesValue->setCursor(Qt::PointingHandCursor);
    m_noValue->setCursor(Qt::PointingHandCursor);

    m_yesValue->setFlat(true);
    m_noValue->setFlat(true);

    m_yesValue->setStyleSheet(QStringLiteral(
        "QPushButton {"
        "  color: #7ef0a8;"
        "  background: #123222;"
        "  border: 1px solid #1f7a45;"
        "  padding: 12px 16px;"
        "  border-radius: 14px;"
        "  font-size: 18px;"
        "  font-weight: 700;"
        "}"
        "QPushButton:hover {"
        "  background: #17452d;"
        "}"));

    m_noValue->setStyleSheet(QStringLiteral(
        "QPushButton {"
        "  color: #ff8f9b;"
        "  background: #34181c;"
        "  border: 1px solid #8b2f39;"
        "  padding: 12px 16px;"
        "  border-radius: 14px;"
        "  font-size: 18px;"
        "  font-weight: 700;"
        "}"
        "QPushButton:hover {"
        "  background: #472126;"
        "}"));

    percentRow->addWidget(m_yesValue);
    percentRow->addWidget(m_noValue);

    root->addLayout(percentRow);

    connect(m_yesValue, &QPushButton::clicked, this, [this]() {
        emit openRequested(QStringLiteral("YES"));
    });

    connect(m_noValue, &QPushButton::clicked, this, [this]() {
        emit openRequested(QStringLiteral("NO"));
    });
}

bool MarketCardWidget::isBinaryYesNoMarket(const ApiMarket &market) const {
    if (market.outcomes.size() != 2) {
        return false;
    }

    bool hasYes = false;
    bool hasNo = false;

    for (const ApiOutcome &outcome : market.outcomes) {
        const QString title = outcome.title.trimmed().toUpper();
        if (title == QStringLiteral("YES")) {
            hasYes = true;
        } else if (title == QStringLiteral("NO")) {
            hasNo = true;
        }
    }

    return hasYes && hasNo;
}

QString MarketCardWidget::metaText(const ApiMarket &market) const {
    const QString created = formatCreatedAt(market.createdAt);
    const QString status = market.status.isEmpty() ? QStringLiteral("OPEN") : market.status;
    const int variantCount = market.outcomes.size();

    if (created.isEmpty()) {
        return QStringLiteral("%1 • %2 variants")
            .arg(status)
            .arg(variantCount);
    }

    return QStringLiteral("%1 • %2 • %3 variants")
        .arg(status)
        .arg(created)
        .arg(variantCount);
}

void MarketCardWidget::setMarket(const ApiMarket &market) {
    m_meta->setText(metaText(market));
    m_question->setText(market.question);

    int yesPercent = 50;
    int noPercent = 50;

    if (isBinaryYesNoMarket(market)) {
        for (const ApiOutcome &outcome : market.outcomes) {
            const QString title = outcome.title.trimmed().toUpper();
            if (title == QStringLiteral("YES")) {
                yesPercent = outcome.pricePercent;
            } else if (title == QStringLiteral("NO")) {
                noPercent = outcome.pricePercent;
            }
        }
    } else if (!market.outcomes.isEmpty()) {
        yesPercent = market.outcomes.first().pricePercent;
        noPercent = 100 - yesPercent;
    }

    m_yesValue->setText(QStringLiteral("Yes %1").arg(percentText(yesPercent)));
    m_noValue->setText(QStringLiteral("No %1").arg(percentText(noPercent)));
}

void MarketCardWidget::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        emit openRequested(QString());
    }
    QFrame::mousePressEvent(event);
}