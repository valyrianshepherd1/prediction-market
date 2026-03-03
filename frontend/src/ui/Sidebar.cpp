#include "Sidebar.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QListWidget>

Sidebar::Sidebar(QWidget *parent) : QWidget(parent) {
    setObjectName("Sidebar");
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(10, 10, 10, 10);
    root->setSpacing(8);

    auto *title = new QLabel("Categories", this);
    title->setObjectName("SidebarTitle");

    m_list = new QListWidget(this);
    m_list->addItems({"Trending", "Politics", "Sports", "Crypto", "Tech"});
    m_list->setCurrentRow(0);

    connect(m_list, &QListWidget::currentTextChanged,
            this, &Sidebar::categoryChanged);

    root->addWidget(title);
    root->addWidget(m_list, 1);
}