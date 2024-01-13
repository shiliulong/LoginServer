#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include "ClientManager.h"
#include <QQmlContext.h>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    
    ClientManager clientManager;
    clientManager.Initialize();
    
    QQmlApplicationEngine engine;
    const QUrl url(u"qrc:/Client/main.qml"_qs);
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl)
            QCoreApplication::exit(-1);
    }, Qt::QueuedConnection);
    
    clientManager.setQmlEngine(&engine);
    engine.rootContext()->setContextProperty("clientManager", &clientManager);
    
    engine.load(url);
    
    return app.exec();
}
