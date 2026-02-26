#include <QApplication>
#include <QLabel>

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    QLabel label("Hello Qt (MSYS2)");
    label.show();
    return app.exec();
}