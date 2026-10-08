#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickStyle>

int main(int argc, char **argv)
{
    QGuiApplication app(argc, argv);
    QGuiApplication::setApplicationName(QStringLiteral("opal-terminal"));
    QGuiApplication::setApplicationDisplayName(QStringLiteral("Terminal"));
    QGuiApplication::setDesktopFileName(QStringLiteral("dev.opal.Terminal"));
    if (qEnvironmentVariableIsEmpty("QT_QUICK_CONTROLS_STYLE"))
        QQuickStyle::setStyle(QStringLiteral("Basic"));

    QQmlApplicationEngine engine;
    // The build tree keeps the QML modules next to the binary; installed builds use the normal import path.
    engine.addImportPath(QStringLiteral(OPAL_QML_DIR));
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed, &app, [] { QCoreApplication::exit(1); }, Qt::QueuedConnection);
    engine.loadFromModule("OpalTerminalApp", "Main");
    return app.exec();
}
