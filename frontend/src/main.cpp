#include <QApplication>
#include "app/MarketWindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    MarketWindow window;
    window.resize(520, 360);
    window.show();

    return app.exec();
}