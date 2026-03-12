#include "../../../include/frontend/ui/pages/AuthRequiredPage.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

AuthRequiredPage::AuthRequiredPage(const QString &description, QWidget *parent)
    : QWidget(parent) {
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(32, 32, 32, 32);
    root->setSpacing(0);

    root->addStretch(1);

    auto *card = new QWidget(this);
    card->setObjectName(QStringLiteral("AuthRequiredCard"));
    card->setMinimumWidth(420);
    card->setMaximumWidth(520);

    card->setStyleSheet(R"(
        #AuthRequiredCard {
            background: #07111d;
            border: 1px solid #132238;
            border-radius: 18px;
        }
        #AuthRequiredCard QLabel {
            border: none;
            background: transparent;
        }
        #AuthRequiredCard QPushButton {
            min-width: 120px;
            padding: 10px 16px;
            border-radius: 10px;
        }
    )");

    auto *cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(28, 28, 28, 28);
    cardLayout->setSpacing(14);

    auto *titleLabel = new QLabel(description, card);
    titleLabel->setWordWrap(true);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet(QStringLiteral(
        "color: white; font-size: 24px; font-weight: 700;"));
    cardLayout->addWidget(titleLabel);

    auto *subtextLabel = new QLabel(QStringLiteral("Log in or create an account to continue."), card);
    subtextLabel->setWordWrap(true);
    subtextLabel->setAlignment(Qt::AlignCenter);
    subtextLabel->setStyleSheet(QStringLiteral(
        "color: #9fb2c7; font-size: 15px;"));
    cardLayout->addWidget(subtextLabel);

    auto *buttons = new QHBoxLayout;
    buttons->setSpacing(12);

    auto *loginButton = new QPushButton(QStringLiteral("Log in"), card);
    auto *signUpButton = new QPushButton(QStringLiteral("Sign up"), card);

    loginButton->setCursor(Qt::PointingHandCursor);
    signUpButton->setCursor(Qt::PointingHandCursor);

    buttons->addStretch(1);
    buttons->addWidget(loginButton);
    buttons->addWidget(signUpButton);
    buttons->addStretch(1);

    cardLayout->addSpacing(4);
    cardLayout->addLayout(buttons);

    m_statusLabel = new QLabel(card);
    m_statusLabel->setWordWrap(true);
    m_statusLabel->setAlignment(Qt::AlignCenter);
    m_statusLabel->setVisible(false);
    m_statusLabel->setStyleSheet(QStringLiteral("color: #9fb2c7; font-size: 13px;"));
    cardLayout->addWidget(m_statusLabel);

    root->addWidget(card, 0, Qt::AlignHCenter);
    root->addStretch(1);

    connect(loginButton, &QPushButton::clicked, this, &AuthRequiredPage::loginRequested);
    connect(signUpButton, &QPushButton::clicked, this, &AuthRequiredPage::signUpRequested);
}

void AuthRequiredPage::setStatusMessage(const QString &message, bool isError) {
    if (!m_statusLabel) {
        return;
    }

    const bool hasMessage = !message.trimmed().isEmpty();
    m_statusLabel->setVisible(hasMessage);
    m_statusLabel->setText(message);
    m_statusLabel->setStyleSheet(
        isError
            ? QStringLiteral("color: #ff7b72; font-size: 13px;")
            : QStringLiteral("color: #9fb2c7; font-size: 13px;"));
}