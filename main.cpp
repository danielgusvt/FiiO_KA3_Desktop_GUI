#include "devicebackend.h"
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    app.setOrganizationName("ka3");
    app.setApplicationName("ka3-control");

    DeviceBackend device;

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("device", &device);
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);
    engine.loadFromModule("KA3_QML", "Main");

    return app.exec();
}
