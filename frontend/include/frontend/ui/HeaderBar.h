#pragma once

#include <QWidget>

class QLabel;
class QPushButton;

class HeaderBar : public QWidget {
    Q_OBJECT
public:
    explicit HeaderBar(QWidget *parent = nullptr);

    void setTitle(const QString &title);
    void setBalanceText(const QString &text);
    void setAvatarText(const QString &text);

    signals:
        void profileClicked();

private:
    QLabel *m_titleLabel = nullptr;
    QLabel *m_balanceLabel = nullptr;
    QPushButton *m_avatarButton = nullptr;
};