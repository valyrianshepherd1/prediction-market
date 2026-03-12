#include "../../include/frontend/ui/Sidebar.h"

#include <QButtonGroup>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

Sidebar::Sidebar(QWidget *parent) : QWidget(parent) {
    setObjectName(QStringLiteral("Sidebar"));

    m_root = new QVBoxLayout(this);
    m_root->setContentsMargins(18, 18, 18, 18);
    m_root->setSpacing(14);

    m_logoLabel = new QLabel(QStringLiteral("PreDgict"), this);
    m_logoLabel->setObjectName(QStringLiteral("SidebarLogo"));
    m_logoLabel->setAlignment(Qt::AlignCenter);
    m_root->addWidget(m_logoLabel);

    auto *dividerTop = new QWidget(this);
    dividerTop->setFixedHeight(1);
    dividerTop->setStyleSheet(QStringLiteral("background: #1b2430;"));
    m_root->addWidget(dividerTop);

    m_group = new QButtonGroup(this);
    m_group->setExclusive(true);

    m_marketsButton = createNavButton(QStringLiteral("Markets"), QStringLiteral("markets"));
    m_portfolioButton = createNavButton(QStringLiteral("Portfolio"), QStringLiteral("portfolio"));
    m_ordersButton = createNavButton(QStringLiteral("Orders"), QStringLiteral("orders"));
    m_tradesButton = createNavButton(QStringLiteral("Trades"), QStringLiteral("trades"));
    m_profileButton = createNavButton(QStringLiteral("Profile"), QStringLiteral("profile"));

    m_root->addSpacing(6);
    m_root->addWidget(m_marketsButton);
    m_root->addWidget(m_portfolioButton);
    m_root->addWidget(m_ordersButton);
    m_root->addWidget(m_tradesButton);
    m_root->addWidget(m_profileButton);
    m_root->addStretch(1);

    auto *dividerBottom = new QWidget(this);
    dividerBottom->setFixedHeight(1);
    dividerBottom->setStyleSheet(QStringLiteral("background: #1b2430;"));
    m_root->addWidget(dividerBottom);

    m_statusLabel = new QLabel(QStringLiteral("Status: idle"), this);
    m_statusLabel->setObjectName(QStringLiteral("SidebarStatus"));
    m_statusLabel->setWordWrap(true);
    m_root->addWidget(m_statusLabel);

    setCurrentPage(QStringLiteral("markets"));
}

QPushButton *Sidebar::createNavButton(const QString &text, const QString &pageId) {
    auto *button = new QPushButton(text, this);
    button->setCheckable(true);
    button->setCursor(Qt::PointingHandCursor);
    button->setProperty("pageId", pageId);
    m_group->addButton(button);

    connect(button, &QPushButton::clicked, this, [this, pageId]() {
        emit pageRequested(pageId);
    });

    return button;
}

void Sidebar::setCurrentPage(const QString &pageId) {
    const auto buttons = m_group->buttons();
    for (auto *button : buttons) {
        if (button->property("pageId").toString() == pageId) {
            button->setChecked(true);
            return;
        }
    }
}

void Sidebar::setStatusText(const QString &text) {
    if (m_statusLabel) {
        m_statusLabel->setText(text);
    }
}