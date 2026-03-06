#include "ProfilePage.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>

ProfilePage::ProfilePage(QWidget *parent) : QWidget(parent) {
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(20, 20, 20, 20);
    root->setSpacing(12);

    auto *title = new QLabel("Profile", this);
    title->setAlignment(Qt::AlignCenter);

    auto *back = new QPushButton("Back", this);
    connect(back, &QPushButton::clicked, this, &ProfilePage::backRequested);

    root->addStretch(1);
    root->addWidget(title);
    root->addWidget(back, 0, Qt::AlignHCenter);
    root->addStretch(2);
}