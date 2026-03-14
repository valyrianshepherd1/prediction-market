#pragma once

#include <QCoreApplication>
#include <QEventLoop>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QProcess>
#include <QQueue>
#include <QSslError>
#include <QThread>
#include <QTimer>
#include <QUrl>
#include <QWebSocket>
#include <QUuid>

#include <functional>
#include <stdexcept>
#include <string>

namespace pmtest {

// исключение для  падения теста.
class Failure : public std::runtime_error {
public:
    // ереводит QString в текст обычного исключения.
    explicit Failure(const QString &message)
        : std::runtime_error(message.toStdString()) {}
};

// останавливает тест, если условие не выполнено.
inline void require(bool condition, const QString &message) {
    if (!condition) {
        throw Failure(message);
    }
}

// хранит самый простой результат HTTP-запроса.
struct HttpResponse {
    int status = 0;
    QByteArray body;
    QNetworkReply::NetworkError networkError = QNetworkReply::NoError;

    //пытается разобрать тело ответа как JSON.
    [[nodiscard]] QJsonDocument json() const {
        QJsonParseError err{};
        const QJsonDocument doc = QJsonDocument::fromJson(body, &err);
        require(err.error == QJsonParseError::NoError,
                QStringLiteral("Не удалось разобрать JSON: %1\nBody: %2")
                    .arg(err.errorString(), QString::fromUtf8(body)));
        return doc;
    }
};

// читает переменную окружения или возвращает запасное значение.
inline QString envOrDefault(const char *name, const QString &fallback) {
    const QString value = qEnvironmentVariable(name).trimmed();
    return value.isEmpty() ? fallback : value;
}

// делает случайный хвост для логина и почты.
inline QString randomTag() {
    return QUuid::createUuid().toString(QUuid::WithoutBraces).remove('-');
}

// достаёт строковое поле из JSON-объекта.
inline QString jsonString(const QJsonObject &obj, const QString &key) {
    return obj.value(key).toString();
}

// собирает заголовок Authorization с Bearer-токеном.
inline QString bearer(const QString &token) {
    return QStringLiteral("Bearer %1").arg(token);
}

// проверяет код ответа и возвращает JSON-объект.
inline QJsonObject expectObject(const HttpResponse &response, int expectedStatus, const QString &label) {
    require(response.status == expectedStatus,
            QStringLiteral("%1: ожидали HTTP %2, получили %3. Body: %4")
                .arg(label)
                .arg(expectedStatus)
                .arg(response.status)
                .arg(QString::fromUtf8(response.body)));
    const QJsonDocument doc = response.json();
    require(doc.isObject(), QStringLiteral("%1: ожидали JSON object").arg(label));
    return doc.object();
}

// проверяет код ответа и возвращает JSON-массив.
inline QJsonArray expectArray(const HttpResponse &response, int expectedStatus, const QString &label) {
    require(response.status == expectedStatus,
            QStringLiteral("%1: ожидали HTTP %2, получили %3. Body: %4")
                .arg(label)
                .arg(expectedStatus)
                .arg(response.status)
                .arg(QString::fromUtf8(response.body)));
    const QJsonDocument doc = response.json();
    require(doc.isArray(), QStringLiteral("%1: ожидали JSON array").arg(label));
    return doc.array();
}

class BackendHarness {
public:
    BackendHarness() = default;

    // завершает backend, если тест запускал его сам.
    ~BackendHarness() {
        if (started_ && process_.state() != QProcess::NotRunning) {
            process_.kill();
            process_.waitForFinished(3000);
        }
    }

    // озвращает базовый URL backend из env или по умолчанию.
    [[nodiscard]] QString baseUrl() const {
        return envOrDefault("PM_BASE_URL", QStringLiteral("https://127.0.0.1:8443"));
    }

    // проверяет, что backend отвечает на /healthz.
    bool isHealthy() {
        QNetworkRequest request{QUrl(baseUrl() + QStringLiteral("/healthz"))};
        QNetworkReply *reply = manager_.get(request);

        QObject::connect(reply, &QNetworkReply::sslErrors, reply,
                         [reply](const QList<QSslError> &) { reply->ignoreSslErrors(); });

        QEventLoop loop;
        QTimer timer;
        timer.setSingleShot(true);
        QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
        timer.start(1000);
        loop.exec();

        if (!timer.isActive()) {
            reply->abort();
            reply->deleteLater();
            return false;
        }

        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        reply->deleteLater();
        return status == 200;
    }

    // однимает backend только если он ещё не запущен.
    void ensureBackendRunning() {
        if (isHealthy()) {
            return;
        }

        const QString backendBin = envOrDefault("PM_BACKEND_BIN", QStringLiteral(PM_BUILD_ROOT) + QStringLiteral("/backend/backend_app"));
        const QString configPath = envOrDefault("PM_CONFIG_PATH", QStringLiteral(PM_REPO_ROOT) + QStringLiteral("/backend/config/config.json"));

        require(QFileInfo::exists(backendBin), QStringLiteral("Не найден backend_app: %1").arg(backendBin));
        require(QFileInfo::exists(configPath), QStringLiteral("Не найден config.json: %1").arg(configPath));

        process_.setProgram(backendBin);
        process_.setArguments({QStringLiteral("--config"), configPath});
        process_.setWorkingDirectory(QFileInfo(backendBin).absolutePath());
        process_.setProcessChannelMode(QProcess::MergedChannels);
        process_.start();

        require(process_.waitForStarted(5000), QStringLiteral("Не удалось запустить backend_app"));
        started_ = true;

        for (int i = 0; i < 20; ++i) {
            if (isHealthy()) {
                return;
            }
            QThread::msleep(500);
        }

        throw Failure(QStringLiteral("Backend не поднялся. Output:\n%1").arg(QString::fromUtf8(process_.readAllStandardOutput())));
    }

    // отправляет простой HTTP-запрос и ждёт ответ.
    HttpResponse request(const QString &method,
                         const QString &path,
                         const QJsonValue &body = QJsonValue(),
                         const QList<QPair<QString, QString>> &headers = {}) {
        QNetworkRequest request{QUrl(baseUrl() + path)};
        request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));

        for (const auto &header : headers) {
            request.setRawHeader(header.first.toUtf8(), header.second.toUtf8());
        }

        const QByteArray payload = body.isObject()
                                       ? QJsonDocument(body.toObject()).toJson(QJsonDocument::Compact)
                                       : QByteArray();

        QNetworkReply *reply = nullptr;
        if (method == QStringLiteral("GET")) {
            reply = manager_.get(request);
        } else if (method == QStringLiteral("POST")) {
            reply = manager_.post(request, payload);
        } else {
            throw Failure(QStringLiteral("Неподдерживаемый HTTP-метод: %1").arg(method));
        }

        QObject::connect(reply, &QNetworkReply::sslErrors, reply,
                         [reply](const QList<QSslError> &) { reply->ignoreSslErrors(); });

        QEventLoop loop;
        QTimer timer;
        timer.setSingleShot(true);
        QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
        timer.start(15000);
        loop.exec();

        if (!timer.isActive()) {
            reply->abort();
            reply->deleteLater();
            throw Failure(QStringLiteral("Таймаут HTTP-запроса: %1 %2").arg(method, path));
        }

        HttpResponse result;
        result.status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        result.body = reply->readAll();
        result.networkError = reply->error();
        reply->deleteLater();
        return result;
    }

private:
    QNetworkAccessManager manager_;
    QProcess process_;
    bool started_ = false;
};

class WsClient {
public:
    // переводит http(s) адрес backend в ws(s) адрес.
    explicit WsClient(const QString &baseUrl)
        : url_(baseUrl.startsWith(QStringLiteral("https://"))
                   ? QStringLiteral("wss://") + baseUrl.mid(QStringLiteral("https://").size()) + QStringLiteral("/ws")
                   : QStringLiteral("ws://") + baseUrl.mid(QStringLiteral("http://").size()) + QStringLiteral("/ws")) {}

    // открывает WebSocket-соединение.
    void connectToServer(const QString &authorization = QString()) {
        QObject::connect(&socket_, &QWebSocket::textMessageReceived, &socket_, [this](const QString &message) {
            QJsonParseError err{};
            const QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8(), &err);
            if (err.error == QJsonParseError::NoError && doc.isObject()) {
                messages_.enqueue(doc.object());
            }
        });

        QNetworkRequest request{QUrl(url_)};
        if (!authorization.trimmed().isEmpty()) {
            request.setRawHeader("Authorization", authorization.toUtf8());
        }

        QObject::connect(&socket_, &QWebSocket::sslErrors, &socket_,
                         [this](const QList<QSslError> &) { socket_.ignoreSslErrors(); });

        QEventLoop loop;
        QTimer timer;
        timer.setSingleShot(true);
        QObject::connect(&socket_, &QWebSocket::connected, &loop, &QEventLoop::quit);
        QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
        timer.start(10000);
        socket_.open(request);
        loop.exec();

        require(timer.isActive(), QStringLiteral("Не удалось подключиться к WebSocket"));
    }

    // отправляет JSON-сообщение в сокет.
    void sendJson(const QJsonObject &obj) {
        socket_.sendTextMessage(QString::fromUtf8(QJsonDocument(obj).toJson(QJsonDocument::Compact)));
    }

    // ждёт нужное сообщение по простому условию.
    QJsonObject waitFor(const std::function<bool(const QJsonObject &)> &predicate, int timeoutMs, const QString &label) {
        const int stepMs = 50;
        int waited = 0;

        while (waited < timeoutMs) {
            while (!messages_.isEmpty()) {
                const QJsonObject obj = messages_.dequeue();
                if (predicate(obj)) {
                    return obj;
                }
            }

            QEventLoop loop;
            QTimer timer;
            timer.setSingleShot(true);
            QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
            timer.start(stepMs);
            loop.exec();
            waited += stepMs;
        }

        throw Failure(QStringLiteral("Таймаут ожидания WS-сообщения: %1").arg(label));
    }

    // закрывает соединение.
    void close() {
        socket_.close();
    }

private:
    QString url_;
    QWebSocket socket_;
    QQueue<QJsonObject> messages_;
};

} // namespace pmtest
