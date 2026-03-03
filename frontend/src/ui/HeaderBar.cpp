#include "HeaderBar.h"
#include <QHBoxLayout>
#include <QPushButton>
#include <QToolButton>
#include <QLabel>
#include <QPainter>
#include <QPixmap>

static QIcon makeProfileIcon(int size) {
    QPixmap pix(size, size);
    pix.fill(Qt::transparent);
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setBrush(QColor(220, 220, 220));
    p.setPen(Qt::NoPen);
    p.drawEllipse(0, 0, size, size);
    p.setBrush(QColor(160, 160, 160));
    p.drawEllipse(QPoint(size/2, size/2 - size/6), size/6, size/6);
    p.drawRoundedRect(QRect(size/2 - size/4, size/2, size/2, size/3), size/8, size/8);
    return QIcon(pix);
}

HeaderBar::HeaderBar(QWidget *parent) : QWidget(parent) {
    setObjectName("HeaderBar");
    auto *h = new QHBoxLayout(this);
    h->setContentsMargins(10, 10, 10, 10);
    h->setSpacing(10);

    auto *home = new QPushButton("Home", this);
    home->setCursor(Qt::PointingHandCursor);
    connect(home, &QPushButton::clicked, this, &HeaderBar::homeClicked);

    m_title = new QLabel("Trending", this);
    m_title->setObjectName("HeaderTitle");

    auto *profile = new QToolButton(this);
    profile->setAutoRaise(true);
    profile->setCursor(Qt::PointingHandCursor);
    profile->setIcon(makeProfileIcon(26));
    profile->setIconSize(QSize(26, 26));
    connect(profile, &QToolButton::clicked, this, &HeaderBar::profileClicked);

    h->addWidget(home);
    h->addStretch(1);
    h->addWidget(m_title);
    h->addStretch(1);
    h->addWidget(profile);
}

void HeaderBar::setTitle(const QString &t) {
    m_title->setText(t);
}