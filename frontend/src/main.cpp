#include <QApplication>
#include <QIcon>

#include "app/MarketWindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    app.setApplicationName(QStringLiteral("PreDgict"));
    app.setApplicationDisplayName(QStringLiteral("PreDgict"));
    app.setWindowIcon(QIcon());

    MarketWindow window;
    window.showMaximized();

    return app.exec();
}