#include "mpris.h"
#include "player.h"

#include <QCommandLineParser>
#include <QDir>
#include <QFileInfo>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QUrl>

int main(int argc, char **argv)
{
    QGuiApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("opal-media"));
    app.setOrganizationName(QStringLiteral("opal"));
    app.setDesktopFileName(QStringLiteral("dev.opal.Media"));
    QQuickStyle::setStyle(QStringLiteral("Basic"));

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("Opal Media"));
    parser.addHelpOption();
    parser.addPositionalArgument(QStringLiteral("files"), QStringLiteral("Songs or streams to play."), QStringLiteral("[files…]"));
    parser.process(app);

    QQmlApplicationEngine engine;
#ifdef OPAL_QML_DIR
    engine.addImportPath(QStringLiteral(OPAL_QML_DIR));
#endif
    Player *player = Player::instance();
    Mpris mpris(player);
    engine.load(QUrl(QStringLiteral("qrc:/qt/qml/Opal/Media/qml/Main.qml")));
    if (engine.rootObjects().isEmpty())
        return 1;
    auto *window = qobject_cast<QQuickWindow *>(engine.rootObjects().first());
    QObject::connect(&mpris, &Mpris::raise, window, [window] { window->show(); window->raise(); window->requestActivate(); });

    // Files from a file manager play at once; a video opens in the video player.
    QVariantList tracks;
    for (const QString &arg : parser.positionalArguments()) {
        const QUrl url = QUrl::fromUserInput(arg, QDir::currentPath(), QUrl::AssumeLocalFile);
        if (url.isLocalFile() && QFileInfo(url.toLocalFile()).isFile())
            tracks << QVariantMap{{QStringLiteral("url"), url.toString()}, {QStringLiteral("path"), url.toLocalFile()}, {QStringLiteral("kind"), QStringLiteral("music")},
                                  {QStringLiteral("title"), QFileInfo(url.toLocalFile()).completeBaseName()}};
        else if (url.scheme().startsWith(QLatin1String("http")))
            player->playRadio({{QStringLiteral("url"), url.toString()}, {QStringLiteral("title"), url.host()}});
    }
    if (!tracks.isEmpty())
        QMetaObject::invokeMethod(window, "openFiles", Q_ARG(QVariant, tracks));
    return app.exec();
}
