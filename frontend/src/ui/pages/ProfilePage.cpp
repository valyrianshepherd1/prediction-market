#include "ProfilePage.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QString>

ProfilePage::ProfilePage(QWidget *parent) : QWidget(parent) {
  auto *root = new QVBoxLayout(this);
  root->setContentsMargins(24, 24, 24, 24);
  root->setSpacing(16);

  // Title
  auto *title = new QLabel("Profile", this);
  title->setAlignment(Qt::AlignCenter);
  title->setStyleSheet("color: white; font-size: 22px; font-weight: 700;");
  root->addWidget(title);

  // Card container
  auto *card = new QWidget(this);
  card->setObjectName("ProfileCard");
  card->setStyleSheet(R"(
    #ProfileCard {
      background: #07111d;
      border: 1px solid #132238;
      border-radius: 16px;
    }
    QLabel { color: white; }
    .k { color: #9fb2c7; font-size: 14px; }
    .v { font-size: 18px; font-weight: 600; }
  )");

  auto *cardLayout = new QVBoxLayout(card);
  cardLayout->setContentsMargins(18, 18, 18, 18);
  cardLayout->setSpacing(12);

  // Name row
  {
    auto *row = new QHBoxLayout;
    auto *k = new QLabel("Name", card);
    k->setProperty("class", "k");
    m_nameValue = new QLabel("—", card);
    m_nameValue->setProperty("class", "v");
    row->addWidget(k);
    row->addStretch(1);
    row->addWidget(m_nameValue);
    cardLayout->addLayout(row);
  }

  // Balance row
  {
    auto *row = new QHBoxLayout;
    auto *k = new QLabel("Balance", card);
    k->setProperty("class", "k");
    m_balanceValue = new QLabel("—", card);
    m_balanceValue->setProperty("class", "v");
    row->addWidget(k);
    row->addStretch(1);
    row->addWidget(m_balanceValue);
    cardLayout->addLayout(row);
  }

  root->addSpacing(8);
  root->addWidget(card, 0, Qt::AlignHCenter);

  // Back button
  auto *back = new QPushButton("Back", this);
  back->setFixedWidth(120);
  connect(back, &QPushButton::clicked, this, &ProfilePage::backRequested);

  root->addSpacing(8);
  root->addWidget(back, 0, Qt::AlignHCenter);
  root->addStretch(1);

  // defaults
  setUserName("Guest");
  setBalance(0.0, "USD");
}

void ProfilePage::setUserName(const QString &name) {
  if (m_nameValue) m_nameValue->setText(name);
}

void ProfilePage::setBalance(double balance, const QString &currency) {
  // simple formatting
  const QString text = QString("%1 %2").arg(currency).arg(balance, 0, 'f', 2);
  if (m_balanceValue) m_balanceValue->setText(text);
}