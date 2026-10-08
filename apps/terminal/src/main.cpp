#include <QCommandLineParser>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QQuickWindow>

int main(int argc, char **argv)
{
    // An alpha buffer lets terminal.conf opacity show what is behind the window.
    QQuickWindow::setDefaultAlphaBuffer(true);
    QGuiApplication app(argc, argv);
    QGuiApplication::setApplicationName(QStringLiteral("opal-terminal"));
    QGuiApplication::setApplicationDisplayName(QStringLiteral("Terminal"));
    QGuiApplication::setDesktopFileName(QStringLiteral("dev.opal.Terminal"));
    if (qEnvironmentVariableIsEmpty("QT_QUICK_CONTROLS_STYLE"))
        QQuickStyle::setStyle(QStringLiteral("Basic"));

    QCommandLineParser parser;
    parser.addHelpOption();
    QCommandLineOption cwd({QStringLiteral("d"), QStringLiteral("working-directory")}, QStringLiteral("Start in this folder."), QStringLiteral("folder"));
    parser.addOption(cwd);
    parser.process(app);
    // The interface reads the start folder from the environment, so tests and renders can set it the same way.
    if (parser.isSet(cwd))
        qputenv("OPAL_TERMINAL_CWD", parser.value(cwd).toLocal8Bit());

    QQmlApplicationEngine engine;
    // The build tree keeps the QML modules next to the binary; installed builds use the normal import path.
    engine.addImportPath(QStringLiteral(OPAL_QML_DIR));
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed, &app, [] { QCoreApplication::exit(1); }, Qt::QueuedConnection);
    engine.loadFromModule("OpalTerminalApp", "Main");
    return app.exec();
}
