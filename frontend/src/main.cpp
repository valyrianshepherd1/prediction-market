#include <QApplication>

#include "app/MarketWindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("Prediction Market"));

    MarketWindow window;
    window.resize(1180, 760);
    window.show();

    return app.exec();
}