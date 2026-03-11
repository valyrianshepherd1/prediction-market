#pragma once

#include <QWidget>

class QLabel;

class ProfilePage : public QWidget {
    Q_OBJECT
public:
    explicit ProfilePage(QWidget *parent = nullptr);

    void setUserName(const QString &name);
    void setEmail(const QString &email);
    void setWalletAmounts(qint64 available,
                          qint64 reserved,
                          const QString &unitLabel = QStringLiteral("units"));
    void setStatusMessage(const QString &message, bool isError = false);

    signals:
        void backRequested();
    void logoutRequested();

private:
    QLabel *m_nameValue = nullptr;
    QLabel *m_emailValue = nullptr;
    QLabel *m_availableValue = nullptr;
    QLabel *m_reservedValue = nullptr;
    QLabel *m_statusValue = nullptr;
};