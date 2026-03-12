#pragma once

#include "frontend/network/ApiTypes.h"

#include <QObject>

class MarketApiClient;

class SessionStore : public QObject {
    Q_OBJECT
public:
    explicit SessionStore(MarketApiClient *transport, QObject *parent = nullptr);

    void restoreSession();
    void login(const QString &login, const QString &password);
    void registerUser(const QString &email, const QString &username, const QString &password);
    void logout();

    [[nodiscard]] bool isAuthenticated() const;
    [[nodiscard]] ApiSession currentSession() const;

signals:
    void authBusyChanged(bool busy);
    void sessionReady(const ApiSession &session);
    void sessionCleared();
    void authError(const QString &message);
    void loggedOut();

private:
    MarketApiClient *m_transport = nullptr;
    ApiSession m_session;
    bool m_authenticated = false;
};
