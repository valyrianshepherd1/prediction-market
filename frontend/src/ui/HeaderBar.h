#pragma once
#include <QWidget>

class QPushButton;
class QToolButton;

class HeaderBar : public QWidget {
    Q_OBJECT
public:
    explicit HeaderBar(QWidget *parent = nullptr);

    signals:
        void homeClicked();
    void profileClicked();

private:
    static QIcon makeProfileIcon(int size);

    QPushButton *m_homeBtn = nullptr;
    QToolButton *m_profileBtn = nullptr;
};