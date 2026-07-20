#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include "EarbudController.hpp"

int main(int argc, char *argv[]) {
    QGuiApplication app(argc, argv);

    qmlRegisterType<EarbudController>("OppoApp", 1, 0, "EarbudController");

    QQmlApplicationEngine engine;

    engine.loadFromModule("OppoApp", "Main");
    if (engine.rootObjects().isEmpty()) {
        const QUrl url(u"qrc:/OppoApp/qml/Main.qml"_qs);
        engine.load(url);
    }

    if (engine.rootObjects().isEmpty()) {
        return -1;
    }

    return app.exec();
}
