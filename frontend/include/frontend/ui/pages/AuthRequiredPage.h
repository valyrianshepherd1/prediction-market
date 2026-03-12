#pragma once

#include <QWidget>

class QLabel;

class AuthRequiredPage : public QWidget {
    Q_OBJECT
public:
    explicit AuthRequiredPage(const QString &description, QWidget *parent = nullptr);

    void setStatusMessage(const QString &message, bool isError = false);

    signals:
        void loginRequested();
    void signUpRequested();

private:
    QLabel *m_statusLabel = nullptr;
};