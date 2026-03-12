#include "../../../include/frontend/ui/dialogs/AuthDialog.h"

#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QWidget>

AuthDialog::AuthDialog(QWidget *parent)
    : QDialog(parent) {
    setModal(true);
    resize(560, 380);
    setMinimumSize(560, 380);
    setWindowTitle(QStringLiteral("Account"));

    setStyleSheet(R"(
        QDialog { background: #0d141d; }
        QTabWidget::pane { border: 1px solid #132238; border-radius: 12px; background: #07111d; top: -1px; }
        QTabBar::tab { background: #141c26; color: #c8d7e6; padding: 10px 18px; border: 1px solid #132238; border-bottom: none; min-width: 120px; }
        QTabBar::tab:selected { background: #182331; color: white; }
        QLabel { color: #c8d7e6; }
        QLineEdit { background: #09131f; border: 1px solid #223349; border-radius: 10px; padding: 10px 12px; color: white; min-width: 320px; }
        QPushButton { background: #1d7dfa; border: 1px solid #3b8ef9; color: white; padding: 10px 18px; border-radius: 10px; font-weight: 700; min-width: 120px; }
        QPushButton:hover { background: #2c87fb; }
    )");

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(18, 18, 18, 18);
    root->setSpacing(12);

    auto *intro = new QLabel(QStringLiteral("Log in or create an account to access personal data."), this);
    intro->setWordWrap(true);
    intro->setStyleSheet(QStringLiteral("color: #c8d7e6; font-size: 13px;"));
    root->addWidget(intro);

    m_tabs = new QTabWidget(this);
    root->addWidget(m_tabs, 1);

    auto *loginTab = new QWidget(m_tabs);
    auto *loginLayout = new QVBoxLayout(loginTab);
    loginLayout->setContentsMargins(12, 12, 12, 12);
    loginLayout->setSpacing(12);

    auto *loginForm = new QFormLayout;
    loginForm->setContentsMargins(0, 0, 0, 0);
    loginForm->setSpacing(14);
    loginForm->setLabelAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    loginForm->setFormAlignment(Qt::AlignTop | Qt::AlignLeft);

    m_loginField = new QLineEdit(loginTab);
    m_loginField->setMinimumWidth(320);
    m_loginField->setPlaceholderText(QStringLiteral("username or email"));

    m_loginPasswordField = new QLineEdit(loginTab);
    m_loginPasswordField->setMinimumWidth(320);
    m_loginPasswordField->setEchoMode(QLineEdit::Password);
    m_loginPasswordField->setPlaceholderText(QStringLiteral("password"));

    loginForm->addRow(QStringLiteral("Login"), m_loginField);
    loginForm->addRow(QStringLiteral("Password"), m_loginPasswordField);

    loginLayout->addLayout(loginForm);

    m_loginButton = new QPushButton(QStringLiteral("Log in"), loginTab);
    m_loginButton->setCursor(Qt::PointingHandCursor);
    loginLayout->addWidget(m_loginButton, 0, Qt::AlignRight);
    loginLayout->addStretch(1);

    auto *signupTab = new QWidget(m_tabs);
    auto *signupLayout = new QVBoxLayout(signupTab);
    signupLayout->setContentsMargins(12, 12, 12, 12);
    signupLayout->setSpacing(12);

    auto *signupForm = new QFormLayout;
    signupForm->setContentsMargins(0, 0, 0, 0);
    signupForm->setSpacing(14);
    signupForm->setLabelAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    signupForm->setFormAlignment(Qt::AlignTop | Qt::AlignLeft);

    m_signupEmailField = new QLineEdit(signupTab);
    m_signupEmailField->setMinimumWidth(320);
    m_signupEmailField->setPlaceholderText(QStringLiteral("name@example.com"));

    m_signupUsernameField = new QLineEdit(signupTab);
    m_signupUsernameField->setMinimumWidth(320);
    m_signupUsernameField->setPlaceholderText(QStringLiteral("username"));

    m_signupPasswordField = new QLineEdit(signupTab);
    m_signupPasswordField->setMinimumWidth(320);
    m_signupPasswordField->setEchoMode(QLineEdit::Password);
    m_signupPasswordField->setPlaceholderText(QStringLiteral("min 8 characters"));

    signupForm->addRow(QStringLiteral("Email"), m_signupEmailField);
    signupForm->addRow(QStringLiteral("Username"), m_signupUsernameField);
    signupForm->addRow(QStringLiteral("Password"), m_signupPasswordField);

    signupLayout->addLayout(signupForm);

    m_signupButton = new QPushButton(QStringLiteral("Sign up"), signupTab);
    m_signupButton->setCursor(Qt::PointingHandCursor);
    signupLayout->addWidget(m_signupButton, 0, Qt::AlignRight);
    signupLayout->addStretch(1);

    m_tabs->addTab(loginTab, QStringLiteral("Log in"));
    m_tabs->addTab(signupTab, QStringLiteral("Sign up"));

    m_statusLabel = new QLabel(this);
    m_statusLabel->setWordWrap(true);
    m_statusLabel->setStyleSheet(QStringLiteral("color: #9fb2c7; font-size: 13px;"));
    root->addWidget(m_statusLabel);

    connect(m_loginButton, &QPushButton::clicked, this, [this]() {
        clearStatus();

        const QString loginValue = m_loginField->text().trimmed();
        const QString password = m_loginPasswordField->text();

        if (loginValue.isEmpty() || password.isEmpty()) {
            setError(QStringLiteral("Enter both login and password."));
            return;
        }

        emit loginSubmitted(loginValue, password);
    });

    connect(m_signupButton, &QPushButton::clicked, this, [this]() {
        clearStatus();

        const QString email = m_signupEmailField->text().trimmed();
        const QString username = m_signupUsernameField->text().trimmed();
        const QString password = m_signupPasswordField->text();

        if (email.isEmpty() || username.isEmpty() || password.isEmpty()) {
            setError(QStringLiteral("Fill all sign-up fields."));
            return;
        }

        emit signUpSubmitted(email, username, password);
    });
}

void AuthDialog::setMode(Mode mode) {
    if (!m_tabs) {
        return;
    }

    m_tabs->setCurrentIndex(mode == Mode::Login ? 0 : 1);
}

void AuthDialog::setBusy(bool busy) {
    if (m_tabs) {
        m_tabs->setEnabled(!busy);
    }

    if (m_loginButton) {
        m_loginButton->setEnabled(!busy);
        m_loginButton->setText(busy ? QStringLiteral("Working...") : QStringLiteral("Log in"));
    }

    if (m_signupButton) {
        m_signupButton->setEnabled(!busy);
        m_signupButton->setText(busy ? QStringLiteral("Working...") : QStringLiteral("Sign up"));
    }
}

void AuthDialog::setError(const QString &message) {
    if (!m_statusLabel) {
        return;
    }

    m_statusLabel->setText(message);
    m_statusLabel->setStyleSheet(QStringLiteral("color: #ff7b72; font-size: 13px;"));
}

void AuthDialog::clearStatus() {
    if (!m_statusLabel) {
        return;
    }

    m_statusLabel->clear();
    m_statusLabel->setStyleSheet(QStringLiteral("color: #9fb2c7; font-size: 13px;"));
}

void AuthDialog::acceptSuccess() {
    setBusy(false);
    clearStatus();
    accept();
}