#include "glibloop.h"
#include "thumbnailer.h"

#include <QCommandLineParser>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QUrl>

int main(int argc, char **argv)
{
    QGuiApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("opal-files"));
    app.setOrganizationName(QStringLiteral("opal"));
    app.setDesktopFileName(QStringLiteral("dev.opal.Files"));
    QQuickStyle::setStyle(QStringLiteral("Basic"));

    QCommandLineParser parser;
    parser.addHelpOption();
    QCommandLineOption debug(QStringLiteral("debug"), QStringLiteral("Load the debug interface."));
    parser.addOption(debug);
    parser.process(app);

    // Async GIO callbacks need the GLib dispatcher; this checks it and falls back to a timer.
    GlibLoop::checkDispatcher();

    QQmlApplicationEngine engine;
#ifdef OPAL_QML_DIR
    engine.addImportPath(QStringLiteral(OPAL_QML_DIR));
#endif
    Thumbnailer::attach(&engine);
    engine.load(QUrl(parser.isSet(debug) ? QStringLiteral("qrc:/qt/qml/Opal/Files/qml/DebugMain.qml") : QStringLiteral("qrc:/qt/qml/Opal/Files/qml/Main.qml")));
    if (engine.rootObjects().isEmpty())
        return 1;
    return app.exec();
}
