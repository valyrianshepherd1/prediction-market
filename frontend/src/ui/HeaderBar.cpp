#include "HeaderBar.h"
#include <QHBoxLayout>
#include <QPushButton>
#include <QToolButton>
#include <QPainter>
#include <QPixmap>
#include <QIcon>

HeaderBar::HeaderBar(QWidget *parent) : QWidget(parent) {
    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);

    m_homeBtn = new QPushButton("Home", this);
    m_homeBtn->setCursor(Qt::PointingHandCursor);
    connect(m_homeBtn, &QPushButton::clicked, this, &HeaderBar::homeClicked);

    m_profileBtn = new QToolButton(this);
    m_profileBtn->setCursor(Qt::PointingHandCursor);
    m_profileBtn->setToolTip("Profile");
    m_profileBtn->setAutoRaise(true);
    m_profileBtn->setIcon(makeProfileIcon(28));
    m_profileBtn->setIconSize(QSize(28, 28));
    connect(m_profileBtn, &QToolButton::clicked, this, &HeaderBar::profileClicked);

    layout->addWidget(m_homeBtn);
    layout->addStretch(1);
    layout->addWidget(m_profileBtn);
}

QIcon HeaderBar::makeProfileIcon(int size) {
    QPixmap pix(size, size);
    pix.fill(Qt::transparent);

    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing, true);

    p.setBrush(QColor(220, 220, 220));
    p.setPen(Qt::NoPen);
    p.drawEllipse(0, 0, size, size);

    p.setBrush(QColor(160, 160, 160));
    const int headR = size / 5;
    p.drawEllipse(QPoint(size / 2, size / 2 - headR), headR, headR);

    QRect bodyRect(size / 2 - size / 4, size / 2, size / 2, size / 3);
    p.drawRoundedRect(bodyRect, size / 8, size / 8);

    return QIcon(pix);
}