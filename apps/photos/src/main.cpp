#include "glibloop.h"
#include "thumbnailer.h"

#include <QCommandLineParser>
#include <QDir>
#include <QFileInfo>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QUrl>

int main(int argc, char **argv)
{
    QGuiApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("opal-photos"));
    app.setOrganizationName(QStringLiteral("opal"));
    app.setDesktopFileName(QStringLiteral("dev.opal.Photos"));
    QQuickStyle::setStyle(QStringLiteral("Basic"));

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("Opal Photos"));
    parser.addHelpOption();
    parser.addPositionalArgument(QStringLiteral("file"), QStringLiteral("A photo to open in the viewer."), QStringLiteral("[file]"));
    parser.process(app);

    // A file from a file manager opens straight in the viewer, next to the other photos of its folder.
    QString open;
    if (!parser.positionalArguments().isEmpty()) {
        const QString arg = parser.positionalArguments().first();
        const QUrl url = QUrl::fromUserInput(arg, QDir::currentPath(), QUrl::AssumeLocalFile);
        if (url.isLocalFile() && QFileInfo(url.toLocalFile()).isFile())
            open = QFileInfo(url.toLocalFile()).absoluteFilePath();
    }

    GlibLoop::checkDispatcher();

    QQmlApplicationEngine engine;
#ifdef OPAL_QML_DIR
    engine.addImportPath(QStringLiteral(OPAL_QML_DIR));
#endif
    Thumbnailer::attach(&engine);
    engine.setInitialProperties({{QStringLiteral("openPath"), open}});
    engine.load(QUrl(QStringLiteral("qrc:/qt/qml/Opal/Photos/qml/Main.qml")));
    if (engine.rootObjects().isEmpty())
        return 1;
    return app.exec();
}
