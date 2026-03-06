#pragma once
#include <QWidget>

class QLabel;

class ProfilePage : public QWidget {
    Q_OBJECT
  public:
    explicit ProfilePage(QWidget *parent = nullptr);

    void setUserName(const QString &name);
    void setBalance(double balance, const QString &currency = "USD");

    signals:
      void backRequested();

private:
    QLabel *m_nameValue = nullptr;
    QLabel *m_balanceValue = nullptr;
};