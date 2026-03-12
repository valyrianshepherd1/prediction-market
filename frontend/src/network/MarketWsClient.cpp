#include "frontend/network/MarketWsClient.h"

#include "frontend/network/MarketApiClient.h"

#include <QAbstractSocket>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QNetworkRequest>
#include <QSslError>
#include <QTimer>
#include <QUrl>
#include <QtAssert>
#include <QtGlobal>
#include <QWebSocket>

namespace {

QString describeSocketState(bool connected, const QUrl &url) {
    if (connected) {
        return QStringLiteral("Realtime connected: %1").arg(url.toString());
    }
    return QStringLiteral("Realtime disconnected");
}

bool isReconnectWorthy(QAbstractSocket::SocketError error) {
    return error != QAbstractSocket::UnknownSocketError;
}

} // namespace

MarketWsClient::MarketWsClient(MarketApiClient *transport, QObject *parent)
    : QObject(parent),
      m_transport(transport),
      m_reconnectTimer(new QTimer(this)) {
    Q_ASSERT(m_transport != nullptr);

    m_reconnectTimer->setSingleShot(true);
    connect(m_reconnectTimer, &QTimer::timeout, this, [this]() {
        if (!m_started || m_manualStop) {
            return;
        }
        openSocket();
    });
}

void MarketWsClient::start() {
    if (m_started) {
        if (!m_connected && !m_socket) {
            openSocket();
        }
        return;
    }

    m_started = true;
    m_manualStop = false;
    openSocket();
}

void MarketWsClient::stop() {
    m_started = false;
    m_manualStop = true;
    m_reconnectTimer->stop();
    closeSocket();
}

void MarketWsClient::reconnectNow() {
    if (!m_started) {
        start();
        return;
    }

    m_manualStop = false;
    m_reconnectTimer->stop();
    closeSocket();
    openSocket();
}

void MarketWsClient::subscribeTopic(const QString &topic,
                                    bool withSnapshot,
                                    int replayLimit) {
    const QString normalized = topic.trimmed();
    if (normalized.isEmpty()) {
        return;
    }

    auto &entry = m_subscriptions[normalized];
    entry.requestTopic = normalized;
    entry.withSnapshot = withSnapshot;
    entry.replayLimit = replayLimit > 0 ? replayLimit : 100;
    if (entry.canonicalTopic.isEmpty() && normalized != QStringLiteral("user:me")) {
        entry.canonicalTopic = normalized;
    }

    if (m_connected) {
        QJsonObject command;
        command.insert(QStringLiteral("action"), QStringLiteral("subscribe"));
        command.insert(QStringLiteral("topic"), normalized);
        command.insert(QStringLiteral("with_snapshot"), entry.withSnapshot);
        command.insert(QStringLiteral("replay_limit"), entry.replayLimit);
        if (entry.lastSeq > 0) {
            command.insert(QStringLiteral("since_seq"), static_cast<qint64>(entry.lastSeq));
        }
        sendJson(command);
    }
}

void MarketWsClient::unsubscribeTopic(const QString &topic) {
    const QString normalized = topic.trimmed();
    if (normalized.isEmpty()) {
        return;
    }

    if (m_connected) {
        QJsonObject command;
        command.insert(QStringLiteral("action"), QStringLiteral("unsubscribe"));
        command.insert(QStringLiteral("topic"), normalized);
        sendJson(command);
    }

    m_subscriptions.remove(normalized);
}

void MarketWsClient::clearTopics() {
    if (m_connected) {
        const auto topics = m_subscriptions.keys();
        for (const QString &topic : topics) {
            QJsonObject command;
            command.insert(QStringLiteral("action"), QStringLiteral("unsubscribe"));
            command.insert(QStringLiteral("topic"), topic);
            sendJson(command);
        }
    }
    m_subscriptions.clear();
}

bool MarketWsClient::isConnected() const {
    return m_connected;
}

void MarketWsClient::openSocket() {
    if (!m_transport) {
        return;
    }

    if (m_socket) {
        closeSocket();
    }

    m_socket = new QWebSocket(QString(), QWebSocketProtocol::VersionLatest, this);

    const QUrl url = m_transport->websocketUrl();

    connect(m_socket, &QWebSocket::connected, this, [this, url]() {
        m_connected = true;
        m_reconnectAttempt = 0;
        emit connectedChanged(true);
        emit statusChanged(describeSocketState(true, url));
        resubscribeAll();
    });

    connect(m_socket, &QWebSocket::disconnected, this, [this]() {
        const bool wasConnected = m_connected;
        m_connected = false;
        emit connectedChanged(false);
        emit statusChanged(QStringLiteral("Realtime disconnected"));

        if (!m_manualStop && m_started) {
            scheduleReconnect();
        }

        if (m_socket) {
            m_socket->deleteLater();
            m_socket = nullptr;
        }

        if (!wasConnected && !m_manualStop && m_started) {
            emit protocolError(QStringLiteral("Realtime connection closed before subscription finished."));
        }
    });

#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    connect(m_socket, &QWebSocket::errorOccurred, this, [this](QAbstractSocket::SocketError error) {
#else
    connect(m_socket,
            QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error),
            this,
            [this](QAbstractSocket::SocketError error) {
#endif
        const QString message = m_socket ? m_socket->errorString() : QStringLiteral("Unknown websocket error.");
        emit protocolError(message);
        if (!m_manualStop && m_started && isReconnectWorthy(error)) {
            scheduleReconnect();
        }
    });

    connect(m_socket, &QWebSocket::textMessageReceived, this, &MarketWsClient::handleTextMessage);

    connect(m_socket, &QWebSocket::sslErrors, this, [this, url](const QList<QSslError> &) {
        if (!m_socket) {
            return;
        }

        const bool isLocalhost =
            url.host() == QStringLiteral("localhost") ||
            url.host() == QStringLiteral("127.0.0.1");
        if (isLocalhost) {
            m_socket->ignoreSslErrors();
        }
    });

    QNetworkRequest request(url);
    request.setRawHeader("Accept", "application/json");

    const QString accessToken = m_transport->accessToken().trimmed();
    if (!accessToken.isEmpty()) {
        request.setRawHeader("Authorization", QByteArray("Bearer ") + accessToken.toUtf8());
    }

    m_socket->open(request);
}

void MarketWsClient::closeSocket() {
    if (!m_socket) {
        m_connected = false;
        return;
    }

    disconnect(m_socket, nullptr, this, nullptr);
    m_socket->close();
    m_socket->deleteLater();
    m_socket = nullptr;
    m_connected = false;
    emit connectedChanged(false);
}

void MarketWsClient::scheduleReconnect() {
    if (m_manualStop || !m_started) {
        return;
    }

    if (m_reconnectTimer->isActive()) {
        return;
    }

    const int delayMs = qMin(5000, 500 * (1 << qMin(m_reconnectAttempt, 3)));
    ++m_reconnectAttempt;
    m_reconnectTimer->start(delayMs);
}

void MarketWsClient::resubscribeAll() {
    const auto subscriptions = m_subscriptions;
    for (auto it = subscriptions.constBegin(); it != subscriptions.constEnd(); ++it) {
        subscribeTopic(it.key(), it.value().withSnapshot, it.value().replayLimit);
    }
}

void MarketWsClient::sendJson(const QJsonObject &object) {
    if (!m_socket || !m_connected) {
        return;
    }
    m_socket->sendTextMessage(QString::fromUtf8(QJsonDocument(object).toJson(QJsonDocument::Compact)));
}

void MarketWsClient::handleTextMessage(const QString &message) {
    QJsonParseError error;
    const QJsonDocument document = QJsonDocument::fromJson(message.toUtf8(), &error);
    if (error.error != QJsonParseError::NoError || !document.isObject()) {
        emit protocolError(QStringLiteral("Invalid websocket JSON: %1").arg(error.errorString()));
        return;
    }

    const QJsonObject envelope = document.object();
    const QString event = envelope.value(QStringLiteral("event")).toString().trimmed();
    const QString topic = envelope.value(QStringLiteral("topic")).toString().trimmed();
    const QJsonObject payload = envelope.value(QStringLiteral("payload")).toObject();
    const QJsonObject meta = envelope.value(QStringLiteral("meta")).toObject();
    const quint64 seq = static_cast<quint64>(meta.value(QStringLiteral("seq")).toVariant().toULongLong());
    const bool snapshot = meta.value(QStringLiteral("snapshot")).toBool(false);

    if (event == QStringLiteral("system.subscribed")) {
        applySubscribeAck(topic, payload);
        emit systemMessageReceived(event, topic, payload);
        return;
    }

    if (event == QStringLiteral("system.replayed")) {
        applyReplayAck(topic, payload);
        emit systemMessageReceived(event, topic, payload);
        return;
    }

    if (event.startsWith(QStringLiteral("system."))) {
        emit systemMessageReceived(event, topic, payload);
        return;
    }

    if (!topic.isEmpty() && seq > 0) {
        updateSequenceForTopic(topic, seq);
    }

    emit topicMessageReceived(topic, event, payload, seq, snapshot);
}

void MarketWsClient::applySubscribeAck(const QString &requestTopic,
                                       const QJsonObject &payload) {
    auto *subscription = subscriptionByRequestTopic(requestTopic);
    if (!subscription) {
        return;
    }

    if (requestTopic != QStringLiteral("user:me")) {
        subscription->canonicalTopic = requestTopic;
        return;
    }

    const QJsonArray topics = payload.value(QStringLiteral("topics")).toArray();
    for (const QJsonValue &value : topics) {
        const QString candidate = value.toString().trimmed();
        if (candidate.startsWith(QStringLiteral("user:")) && candidate != QStringLiteral("user:me")) {
            subscription->canonicalTopic = candidate;
            return;
        }
    }
}

void MarketWsClient::applyReplayAck(const QString &requestTopic,
                                    const QJsonObject &payload) {
    auto *subscription = subscriptionByRequestTopic(requestTopic);
    if (!subscription) {
        return;
    }

    const quint64 currentSeq = static_cast<quint64>(payload.value(QStringLiteral("current_seq")).toVariant().toULongLong());
    if (currentSeq > subscription->lastSeq) {
        subscription->lastSeq = currentSeq;
    }
}

void MarketWsClient::updateSequenceForTopic(const QString &actualTopic,
                                            quint64 seq) {
    if (auto *subscription = subscriptionByActualTopic(actualTopic)) {
        subscription->lastSeq = qMax(subscription->lastSeq, seq);
        if (subscription->requestTopic == QStringLiteral("user:me") && subscription->canonicalTopic.isEmpty()) {
            subscription->canonicalTopic = actualTopic;
        }
        return;
    }

    if (actualTopic.startsWith(QStringLiteral("user:"))) {
        if (auto *userSubscription = subscriptionByRequestTopic(QStringLiteral("user:me"))) {
            userSubscription->canonicalTopic = actualTopic;
            userSubscription->lastSeq = qMax(userSubscription->lastSeq, seq);
        }
    }
}

MarketWsClient::SubscriptionState *MarketWsClient::subscriptionByRequestTopic(const QString &topic) {
    auto it = m_subscriptions.find(topic);
    if (it == m_subscriptions.end()) {
        return nullptr;
    }
    return &it.value();
}

MarketWsClient::SubscriptionState *MarketWsClient::subscriptionByActualTopic(const QString &topic) {
    for (auto it = m_subscriptions.begin(); it != m_subscriptions.end(); ++it) {
        if (it.value().requestTopic == topic || it.value().canonicalTopic == topic) {
            return &it.value();
        }
    }
    return nullptr;
}
