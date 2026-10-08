#include <QDir>
#include <QGuiApplication>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickView>
#include <QTimer>

// Renders one QML file to a PNG: opal-render <file.qml> <out.png> <width>x<height> [delay-ms].
// The posture comes from OPAL_POSTURE and the colours from the usual Palette sources, so a render matches the real app.
int main(int argc, char **argv)
{
    QGuiApplication app(argc, argv);
    if (argc < 4) { qCritical("usage: opal-render file.qml out.png WxH [delay]"); return 2; }
    const QStringList size = QString::fromLocal8Bit(argv[3]).split(QLatin1Char('x'));
    const int delay = argc > 4 ? atoi(argv[4]) : 900;
    QQuickView view;
    view.engine()->addImportPath(QStringLiteral(OPAL_QML_DIR));
    view.setResizeMode(QQuickView::SizeRootObjectToView);
    view.resize(size.value(0).toInt(), size.value(1).toInt());
    view.setSource(QUrl::fromLocalFile(QDir::current().absoluteFilePath(QString::fromLocal8Bit(argv[1]))));
    if (view.status() != QQuickView::Ready) { for (const auto &e : view.errors()) qCritical("%s", qPrintable(e.toString())); return 1; }
    view.show();
    const QString out = QString::fromLocal8Bit(argv[2]);
    QTimer::singleShot(delay, &app, [&] {
        const bool ok = view.grabWindow().save(out);
        qInfo("%s %s", ok ? "wrote" : "FAILED", qPrintable(out));
        app.exit(ok ? 0 : 1);
    });
    return app.exec();
}
