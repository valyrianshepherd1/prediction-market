#include "HomePage.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>

HomePage::HomePage(QWidget *parent) : QWidget(parent) {
    auto *layout = new QVBoxLayout(this);

    auto *title = new QLabel("<h2>Home</h2>", this);
    auto *text  = new QLabel("Welcome! Use the button below to view markets.", this);

    auto *btn = new QPushButton("Go to Markets", this);
    btn->setCursor(Qt::PointingHandCursor);
    connect(btn, &QPushButton::clicked, this, &HomePage::goToMarkets);

    layout->addWidget(title);
    layout->addWidget(text);
    layout->addSpacing(10);
    layout->addWidget(btn);
    layout->addStretch(1);
}