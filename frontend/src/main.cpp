#include <QApplication>
#include <QCoreApplication>

#include "../include/frontend/app/MarketWindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    QCoreApplication::setOrganizationName(QStringLiteral("prediction-market"));
    QCoreApplication::setOrganizationDomain(QStringLiteral("local.prediction-market"));
    QCoreApplication::setApplicationName(QStringLiteral("PreDgict"));

    app.setApplicationDisplayName(QStringLiteral("PreDgict"));

    MarketWindow window;
    window.showMaximized();

    return app.exec();
}