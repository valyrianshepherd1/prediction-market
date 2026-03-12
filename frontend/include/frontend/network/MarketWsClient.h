#pragma once

#include <QHash>
#include <QJsonObject>
#include <QObject>
#include <QString>

class MarketApiClient;
class QTimer;
class QWebSocket;

class MarketWsClient : public QObject {
    Q_OBJECT
public:
    explicit MarketWsClient(MarketApiClient *transport, QObject *parent = nullptr);

    void start();
    void stop();
    void reconnectNow();

    void subscribeTopic(const QString &topic,
                        bool withSnapshot = true,
                        int replayLimit = 100);
    void unsubscribeTopic(const QString &topic);
    void clearTopics();

    [[nodiscard]] bool isConnected() const;

signals:
    void connectedChanged(bool connected);
    void statusChanged(const QString &message);
    void topicMessageReceived(const QString &topic,
                              const QString &event,
                              const QJsonObject &payload,
                              quint64 seq,
                              bool snapshot);
    void systemMessageReceived(const QString &event,
                               const QString &topic,
                               const QJsonObject &payload);
    void protocolError(const QString &message);

private:
    struct SubscriptionState {
        QString requestTopic;
        QString canonicalTopic;
        quint64 lastSeq = 0;
        bool withSnapshot = true;
        int replayLimit = 100;
    };

    void openSocket();
    void closeSocket();
    void scheduleReconnect();
    void resubscribeAll();
    void sendJson(const QJsonObject &object);
    void handleTextMessage(const QString &message);
    void applySubscribeAck(const QString &requestTopic,
                           const QJsonObject &payload);
    void applyReplayAck(const QString &requestTopic,
                        const QJsonObject &payload);
    void updateSequenceForTopic(const QString &actualTopic,
                                quint64 seq);
    SubscriptionState *subscriptionByRequestTopic(const QString &topic);
    SubscriptionState *subscriptionByActualTopic(const QString &topic);

    MarketApiClient *m_transport = nullptr;
    QWebSocket *m_socket = nullptr;
    QTimer *m_reconnectTimer = nullptr;
    bool m_started = false;
    bool m_manualStop = false;
    bool m_connected = false;
    int m_reconnectAttempt = 0;
    QHash<QString, SubscriptionState> m_subscriptions;
};
