#pragma once

#include <QDialog>

class QLabel;
class QLineEdit;
class QPushButton;
class QTabWidget;

class AuthDialog : public QDialog {
    Q_OBJECT
public:
    enum class Mode {
        Login,
        SignUp
    };

    explicit AuthDialog(QWidget *parent = nullptr);

    void setMode(Mode mode);

public slots:
    void setBusy(bool busy);
    void setError(const QString &message);
    void clearStatus();
    void acceptSuccess();

    signals:
        void loginSubmitted(const QString &login, const QString &password);
    void signUpSubmitted(const QString &email, const QString &username, const QString &password);

private:
    QTabWidget *m_tabs = nullptr;

    QLineEdit *m_loginField = nullptr;
    QLineEdit *m_loginPasswordField = nullptr;
    QPushButton *m_loginButton = nullptr;

    QLineEdit *m_signupEmailField = nullptr;
    QLineEdit *m_signupUsernameField = nullptr;
    QLineEdit *m_signupPasswordField = nullptr;
    QPushButton *m_signupButton = nullptr;

    QLabel *m_statusLabel = nullptr;
};