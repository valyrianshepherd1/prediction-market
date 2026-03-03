#pragma once
#include <QWidget>

class QListWidget;

class Sidebar : public QWidget {
    Q_OBJECT
public:
    explicit Sidebar(QWidget *parent = nullptr);

    signals:
        void categoryChanged(const QString &category);

private:
    QListWidget *m_list = nullptr;
};