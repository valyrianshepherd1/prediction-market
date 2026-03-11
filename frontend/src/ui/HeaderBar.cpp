#include "HeaderBar.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>

HeaderBar::HeaderBar(QWidget *parent) : QWidget(parent) {
    setObjectName(QStringLiteral("HeaderBar"));

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(18, 12, 18, 12);
    layout->setSpacing(14);

    m_titleLabel = new QLabel(QStringLiteral("Markets"), this);
    m_titleLabel->setObjectName(QStringLiteral("HeaderTitle"));
    m_titleLabel->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);

    m_balanceLabel = new QLabel(QStringLiteral("Balance: —"), this);
    m_balanceLabel->setObjectName(QStringLiteral("HeaderBalance"));
    m_balanceLabel->setAlignment(Qt::AlignVCenter | Qt::AlignRight);

    m_avatarButton = new QPushButton(QStringLiteral("A"), this);
    m_avatarButton->setObjectName(QStringLiteral("HeaderAvatar"));
    m_avatarButton->setFixedSize(40, 40);
    m_avatarButton->setCursor(Qt::PointingHandCursor);

    layout->addWidget(m_titleLabel, 1);
    layout->addWidget(m_balanceLabel, 0);
    layout->addWidget(m_avatarButton, 0);

    connect(m_avatarButton, &QPushButton::clicked, this, &HeaderBar::profileClicked);
}

void HeaderBar::setTitle(const QString &title) {
    if (m_titleLabel) {
        m_titleLabel->setText(title);
    }
}

void HeaderBar::setBalanceText(const QString &text) {
    if (m_balanceLabel) {
        m_balanceLabel->setText(text);
    }
}

void HeaderBar::setAvatarText(const QString &text) {
    if (m_avatarButton) {
        m_avatarButton->setText(text);
    }
}