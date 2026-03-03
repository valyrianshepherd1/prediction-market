#pragma once
#include <QWidget>

class QPushButton;
class QToolButton;
class QLabel;

class HeaderBar : public QWidget {
    Q_OBJECT
public:
    explicit HeaderBar(QWidget *parent = nullptr);

    signals:
        void homeClicked();
    void profileClicked();

public:
    void setTitle(const QString &t);

private:
    QLabel *m_title = nullptr;
};