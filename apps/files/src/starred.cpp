#include "starred.h"

#include <QCoreApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPointer>
#include <QSaveFile>
#include <QThreadPool>
#include <QStandardPaths>
#include <QtConcurrent/QtConcurrentRun>
#include <QQmlEngine>

Starred::Starred(const QString &path, QObject *parent)
    : QObject(parent)
{
    m_path = path.isEmpty() ? QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QStringLiteral("/opal/files/starred.json") : path;
    load();
}

Starred *Starred::instance()
{
    static Starred *s = new Starred(QString(), QCoreApplication::instance());
    return s;
}

Starred *Starred::create(QQmlEngine *, QJSEngine *)
{
    auto *s = instance();
    QJSEngine::setObjectOwnership(s, QJSEngine::CppOwnership);
    return s;
}

void Starred::load()
{
    QPointer<Starred> self(this);
    const QString path = m_path;
    // Reading happens on a worker so a slow home directory never blocks the GUI.
    (void)QtConcurrent::run([path]() -> QStringList {
        QFile f(path);
        QStringList out;
        if (f.open(QIODevice::ReadOnly)) {
            const auto arr = QJsonDocument::fromJson(f.readAll()).object().value(QStringLiteral("starred")).toArray();
            for (const auto &v : arr)
                out << v.toString();
        }
        return out;
    }).then(this, [self](const QStringList &list) {
        if (!self)
            return;
        // Stars added before the load finished stay.
        for (const QString &u : list) {
            if (!self->m_set.contains(u)) {
                self->m_set.insert(u);
                self->m_uris.append(u);
            }
        }
        self->m_loaded = true;
        emit self->changed();
        if (self->m_dirtyBeforeLoad)
            self->save();
    });
}

void Starred::save()
{
    if (!m_loaded) {
        m_dirtyBeforeLoad = true;
        return;
    }
    const QString path = m_path;
    const QStringList list = m_uris;
    const int gen = ++m_saveGeneration;
    // One writer thread keeps the newest list last; QSaveFile makes each write atomic.
    static QThreadPool *writer = [] { auto *p = new QThreadPool(QCoreApplication::instance()); p->setMaxThreadCount(1); return p; }();
    (void)QtConcurrent::run(writer, [path, list, gen]() {
        QDir().mkpath(QFileInfo(path).absolutePath());
        QSaveFile f(path);
        if (!f.open(QIODevice::WriteOnly))
            return;
        QJsonArray arr;
        for (const QString &u : list)
            arr.append(u);
        QJsonObject o;
        o.insert(QStringLiteral("starred"), arr);
        f.write(QJsonDocument(o).toJson(QJsonDocument::Indented));
        f.commit();
        Q_UNUSED(gen)
    });
}

void Starred::star(const QString &uri)
{
    if (uri.isEmpty() || m_set.contains(uri))
        return;
    m_set.insert(uri);
    m_uris.append(uri);
    emit changed();
    save();
}

void Starred::unstar(const QString &uri)
{
    if (!m_set.remove(uri))
        return;
    m_uris.removeOne(uri);
    emit changed();
    save();
}

void Starred::toggle(const QString &uri)
{
    if (isStarred(uri))
        unstar(uri);
    else
        star(uri);
}

bool Starred::waitForLoaded(int msecs) const
{
    QElapsedTimer t;
    t.start();
    while (!m_loaded && t.elapsed() < msecs)
        QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
    return m_loaded;
}
