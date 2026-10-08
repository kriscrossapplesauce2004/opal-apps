#include <QGuiApplication>
#include <QQmlEngine>
#include <QQuickView>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    QGuiApplication::setApplicationName(QStringLiteral(OPAL_APP_NAME));
    QGuiApplication::setDesktopFileName(QStringLiteral(OPAL_DESKTOP_ID));

    const QString posture = qEnvironmentVariable("OPAL_POSTURE").toLower();
    QString source = QStringLiteral(OPAL_POINTER_QML);
    QSize size(1440, 900);
    if (posture == QStringLiteral("touch")) {
        source = QStringLiteral(OPAL_TOUCH_QML);
        size = QSize(420, 860);
    } else if (posture == QStringLiteral("couch")) {
        source = QStringLiteral(OPAL_COUCH_QML);
        size = QSize(1920, 1080);
    }

    QQuickView view;
    view.engine()->addImportPath(QStringLiteral(OPAL_QML_DIR));
    view.setResizeMode(QQuickView::SizeRootObjectToView);
    view.resize(size);
    view.setSource(QUrl(source));
    if (view.status() == QQuickView::Error)
        return 1;
    view.show();
    return app.exec();
}
