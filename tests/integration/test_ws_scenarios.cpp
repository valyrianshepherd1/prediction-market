#include "ScenarioSupport.h"

#include <iostream>

using namespace pmtest;

// проверяет, что после подключения приходит приветственное событие от сервера.
static void testWsWelcome(WsClient &ws) {
    const QJsonObject message = ws.waitFor(
        [](const QJsonObject &obj) {
            return obj.value(QStringLiteral("event")).toString() == QStringLiteral("system.welcome");
        },
        10000,
        QStringLiteral("system.welcome"));

    require(message.value(QStringLiteral("event")).toString() == QStringLiteral("system.welcome"),
            QStringLiteral("После подключения должно прийти system.welcome"));
}

// проверяет, что сервер отвечает на простую команду describe.
static void testWsDescribe(WsClient &ws) {
    ws.sendJson(QJsonObject{{QStringLiteral("action"), QStringLiteral("describe")}});

    const QJsonObject message = ws.waitFor(
        [](const QJsonObject &obj) {
            return obj.value(QStringLiteral("event")).toString() == QStringLiteral("system.describe");
        },
        10000,
        QStringLiteral("system.describe"));

    require(message.value(QStringLiteral("event")).toString() == QStringLiteral("system.describe"),
            QStringLiteral("На action=describe должно прийти system.describe"));
}

int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);

    try {
        BackendHarness h;
        h.ensureBackendRunning();

        WsClient ws(h.baseUrl());
        ws.connectToServer();

        testWsWelcome(ws);
        testWsDescribe(ws);

        ws.close();
        std::cout << "pm_ws_scenarios_tests passed" << std::endl;
        return 0;
    } catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
}
