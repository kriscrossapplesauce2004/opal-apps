#include "glibloop.h"
#include "thumbnailer.h"

#include <QDir>
#include <QFile>
#include <QQmlContext>
#include <QQmlEngine>
#include <QTemporaryDir>
#include <QtQuickTest>

// Runs tests/qml/tst_filesui.qml against a fixture folder: two files, a picture and a sub folder.
class Setup : public QObject
{
    Q_OBJECT
public slots:
    void qmlEngineAvailable(QQmlEngine *engine)
    {
        GlibLoop::checkDispatcher();
        Thumbnailer::attach(engine);
        engine->addImportPath(QStringLiteral(OPAL_QML_DIR));
        QDir d(m_dir.path());
        d.mkpath(QStringLiteral("Folder"));
        auto write = [&](const QString &name, const QByteArray &data) { QFile f(d.filePath(name)); f.open(QIODevice::WriteOnly); f.write(data); };
        write(QStringLiteral("alpha.txt"), "alpha");
        write(QStringLiteral("beta.txt"), "beta beta");
        write(QStringLiteral("Folder/inside.txt"), "inside");
        engine->rootContext()->setContextProperty(QStringLiteral("fixtureDir"), QUrl::fromLocalFile(m_dir.path()).toString());
    }
private:
    QTemporaryDir m_dir;
};

QUICK_TEST_MAIN_WITH_SETUP(filesui, Setup)
#include "tst_filesui.moc"
