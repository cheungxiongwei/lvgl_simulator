#include <QApplication>
#include <QQmlApplicationEngine>
#include "main.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    QQmlApplicationEngine engine;
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed, &app, []() { QCoreApplication::exit(-1); }, Qt::QueuedConnection);
    engine.loadFromModule("LVGLQML", "Main");
    return app.exec();
}
