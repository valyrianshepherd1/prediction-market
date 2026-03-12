#include "frontend/state/SessionStore.h"

#include "frontend/network/MarketApiClient.h"

SessionStore::SessionStore(MarketApiClient *transport, QObject *parent)
    : QObject(parent),
      m_transport(transport) {
    Q_ASSERT(m_transport != nullptr);

    connect(m_transport, &MarketApiClient::authBusyChanged, this, &SessionStore::authBusyChanged);
    connect(m_transport, &MarketApiClient::authError, this, &SessionStore::authError);
    connect(m_transport, &MarketApiClient::loggedOut, this, &SessionStore::loggedOut);

    connect(m_transport, &MarketApiClient::sessionReady, this, [this](const ApiSession &session) {
        m_session = session;
        m_authenticated = !session.userId.trimmed().isEmpty();
        emit sessionReady(m_session);
    });

    connect(m_transport, &MarketApiClient::sessionCleared, this, [this]() {
        m_session = ApiSession{};
        m_authenticated = false;
        emit sessionCleared();
    });
}

void SessionStore::restoreSession() {
    if (m_transport) {
        m_transport->restoreSession();
    }
}

void SessionStore::login(const QString &loginValue, const QString &password) {
    if (m_transport) {
        m_transport->login(loginValue, password);
    }
}

void SessionStore::registerUser(const QString &email,
                                const QString &username,
                                const QString &password) {
    if (m_transport) {
        m_transport->registerUser(email, username, password);
    }
}

void SessionStore::logout() {
    if (m_transport) {
        m_transport->logout();
    }
}

bool SessionStore::isAuthenticated() const {
    return m_authenticated;
}

ApiSession SessionStore::currentSession() const {
    return m_session;
}
