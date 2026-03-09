#include "ProfilePage.h"

#include <QLocale>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

namespace {
QString formatUnits(qint64 amount, const QString &unitLabel) {
    return QStringLiteral("%1 %2")
        .arg(QLocale().toString(amount))
        .arg(unitLabel);
}
} // namespace

ProfilePage::ProfilePage(QWidget *parent) : QWidget(parent) {
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(24, 24, 24, 24);
    root->setSpacing(16);

    auto *title = new QLabel(QStringLiteral("Profile"), this);
    title->setAlignment(Qt::AlignCenter);
    title->setStyleSheet(QStringLiteral("color: white; font-size: 22px; font-weight: 700;"));
    root->addWidget(title);

    auto *card = new QWidget(this);
    card->setObjectName(QStringLiteral("ProfileCard"));
    card->setStyleSheet(R"(
        #ProfileCard {
            background: #07111d;
            border: 1px solid #132238;
            border-radius: 16px;
        }
        QLabel { color: white; }
    )");

    auto *cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(18, 18, 18, 18);
    cardLayout->setSpacing(12);

    auto addRow = [card, cardLayout](const QString &label, QLabel **valueLabel) {
        auto *row = new QHBoxLayout;
        auto *key = new QLabel(label, card);
        key->setStyleSheet(QStringLiteral("color: #9fb2c7; font-size: 14px;"));

        *valueLabel = new QLabel(QStringLiteral("—"), card);
        (*valueLabel)->setStyleSheet(QStringLiteral("font-size: 18px; font-weight: 600;"));
        (*valueLabel)->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

        row->addWidget(key);
        row->addStretch(1);
        row->addWidget(*valueLabel);
        cardLayout->addLayout(row);
    };

    addRow(QStringLiteral("Name"), &m_nameValue);
    addRow(QStringLiteral("Available"), &m_availableValue);
    addRow(QStringLiteral("Reserved"), &m_reservedValue);

    m_statusValue = new QLabel(card);
    m_statusValue->setWordWrap(true);
    m_statusValue->setStyleSheet(QStringLiteral("color: #9fb2c7; font-size: 13px;"));
    cardLayout->addWidget(m_statusValue);

    root->addWidget(card, 0, Qt::AlignHCenter);

    auto *back = new QPushButton(QStringLiteral("Back"), this);
    back->setFixedWidth(120);
    connect(back, &QPushButton::clicked, this, &ProfilePage::backRequested);

    root->addWidget(back, 0, Qt::AlignHCenter);
    root->addStretch(1);

    setUserName(QStringLiteral("Guest"));
    setWalletAmounts(0, 0);
    setStatusMessage(
        QStringLiteral("Set PM_FRONTEND_USER_ID to a valid backend user UUID to load a real wallet."));
}

void ProfilePage::setUserName(const QString &name) {
    if (m_nameValue) {
        m_nameValue->setText(name);
    }
}

void ProfilePage::setWalletAmounts(qint64 available, qint64 reserved, const QString &unitLabel) {
    if (m_availableValue) {
        m_availableValue->setText(formatUnits(available, unitLabel));
    }
    if (m_reservedValue) {
        m_reservedValue->setText(formatUnits(reserved, unitLabel));
    }
}

void ProfilePage::setStatusMessage(const QString &message, bool isError) {
    if (!m_statusValue) {
        return;
    }

    m_statusValue->setText(message);
    m_statusValue->setStyleSheet(
        isError ? QStringLiteral("color: #ff7b72; font-size: 13px;")
                : QStringLiteral("color: #9fb2c7; font-size: 13px;"));
}