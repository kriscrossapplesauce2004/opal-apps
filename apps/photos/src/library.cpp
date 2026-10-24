#include "library.h"
#include "starred.h"
#include "thumbnailer.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QLocale>
#include <QPointer>
#include <QQmlEngine>
#include <QSaveFile>
#include <QSettings>
#include <QStandardPaths>
#include <QStorageInfo>
#include <QtConcurrent>
#include <algorithm>
#include <unistd.h>

namespace {

constexpr quint32 CacheMagic = 0x4f50484f; // "OPHO"
constexpr quint32 CacheVersion = 1;

QString cachePath()
{
    return QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation) + QStringLiteral("/opal/photos/index.bin");
}

QSettings *settings()
{
    static QSettings *s = new QSettings(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + QStringLiteral("/opal/photos.ini"),
                                        QSettings::IniFormat, QCoreApplication::instance());
    return s;
}

void sortNewestFirst(QList<Photo> &list)
{
    std::sort(list.begin(), list.end(), [](const Photo &a, const Photo &b) { return a.taken != b.taken ? a.taken > b.taken : a.path > b.path; });
}

}

PhotoLibrary::PhotoLibrary(QObject *parent)
    : QObject(parent)
{
    m_rescanTimer.setSingleShot(true);
    m_rescanTimer.setInterval(1200);
    connect(&m_rescanTimer, &QTimer::timeout, this, &PhotoLibrary::rescan);
    m_saveTimer.setSingleShot(true);
    m_saveTimer.setInterval(4000);
    connect(&m_saveTimer, &QTimer::timeout, this, &PhotoLibrary::saveCache);
    connect(&m_watcher, &QFileSystemWatcher::directoryChanged, this, [this] { m_rescanTimer.start(); });
    connect(Starred::instance(), &Starred::changed, this, &PhotoLibrary::favouritesChanged);
    if (qEnvironmentVariable("OPAL_PHOTOS_NO_OCR") != QLatin1String("1")) {
        m_tesseract = QStandardPaths::findExecutable(QStringLiteral("tesseract"));
        m_nice = QStandardPaths::findExecutable(QStringLiteral("nice"));
    }
    loadSettings();
    loadCache();
}

PhotoLibrary::~PhotoLibrary()
{
    if (m_cancel)
        m_cancel->store(true);
    if (m_saveTimer.isActive())
        saveCache();
    if (m_ocr) {
        m_ocr->disconnect(this);
        m_ocr->kill();
        m_ocr->waitForFinished(500);
    }
}

PhotoLibrary *PhotoLibrary::instance()
{
    static PhotoLibrary *l = new PhotoLibrary(QCoreApplication::instance());
    return l;
}

PhotoLibrary *PhotoLibrary::create(QQmlEngine *engine, QJSEngine *)
{
    auto *l = instance();
    QJSEngine::setObjectOwnership(l, QJSEngine::CppOwnership);
    Thumbnailer::attach(engine);
    return l;
}

QString PhotoLibrary::picturesPath() const
{
    const QString p = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
    return p.isEmpty() ? QDir::homePath() + QStringLiteral("/Pictures") : p;
}

QStringList PhotoLibrary::roots() const
{
    return m_roots;
}

void PhotoLibrary::loadSettings()
{
    const QString env = qEnvironmentVariable("OPAL_PHOTOS_ROOTS");
    if (!env.isEmpty()) {
        m_envRoots = true;
        m_roots = env.split(QLatin1Char(':'), Qt::SkipEmptyParts);
        m_ocrEnabled = true;
        return;
    }
    QSettings *s = settings();
    m_roots = s->value(QStringLiteral("library/roots")).toStringList();
    if (m_roots.isEmpty()) {
        // The camera folders that phones and Android-style tools use sit next to Pictures.
        m_roots << picturesPath();
        for (const QString &extra : {QDir::homePath() + QStringLiteral("/DCIM"), QStandardPaths::writableLocation(QStandardPaths::MoviesLocation) + QStringLiteral("/Screencasts")})
            if (QFileInfo(extra).isDir())
                m_roots << extra;
    }
    m_excluded = s->value(QStringLiteral("library/excluded")).toStringList();
    m_skipSoftware = s->value(QStringLiteral("library/skipSoftware"), true).toBool();
    m_videos = s->value(QStringLiteral("library/videos"), true).toBool();
    m_ocrEnabled = s->value(QStringLiteral("library/ocr"), true).toBool();
}

void PhotoLibrary::saveSettings()
{
    emit settingsChanged();
    if (m_envRoots)
        return;
    QSettings *s = settings();
    s->setValue(QStringLiteral("library/roots"), m_roots);
    s->setValue(QStringLiteral("library/excluded"), m_excluded);
    s->setValue(QStringLiteral("library/skipSoftware"), m_skipSoftware);
    s->setValue(QStringLiteral("library/videos"), m_videos);
    s->setValue(QStringLiteral("library/ocr"), m_ocrEnabled);
    s->sync();
}

void PhotoLibrary::setSkipSoftware(bool on)
{
    if (m_skipSoftware == on)
        return;
    m_skipSoftware = on;
    saveSettings();
    rescan();
}

void PhotoLibrary::setIncludeVideos(bool on)
{
    if (m_videos == on)
        return;
    m_videos = on;
    saveSettings();
    rescan();
}

void PhotoLibrary::setOcrEnabled(bool on)
{
    if (m_ocrEnabled == on)
        return;
    m_ocrEnabled = on;
    saveSettings();
    if (on)
        queueOcr();
    else {
        m_ocrQueue.clear();
        emit ocrChanged();
    }
}

void PhotoLibrary::addRoot(const QString &urlOrPath)
{
    const QString path = QDir::cleanPath(urlOrPath.startsWith(QLatin1String("file:")) ? QUrl(urlOrPath).toLocalFile() : urlOrPath);
    if (path.isEmpty() || m_roots.contains(path) || !QFileInfo(path).isDir())
        return;
    m_roots << path;
    m_excluded.removeAll(path);
    saveSettings();
    rescan();
}

void PhotoLibrary::removeRoot(const QString &path)
{
    if (m_roots.size() < 2 || !m_roots.removeAll(path))
        return;
    saveSettings();
    rescan();
}

void PhotoLibrary::exclude(const QString &path)
{
    if (m_excluded.contains(path))
        return;
    m_excluded << path;
    saveSettings();
    rescan();
}

void PhotoLibrary::include(const QString &path)
{
    if (!m_excluded.removeAll(path))
        return;
    saveSettings();
    rescan();
}

QStringList PhotoLibrary::deviceRoots() const
{
    // Phones and cameras: removable volumes and MTP mounts that hold a DCIM folder.
    QStringList out;
    for (const QStorageInfo &v : QStorageInfo::mountedVolumes()) {
        const QString root = v.rootPath();
        if (!(root.startsWith(QLatin1String("/run/media/")) || root.startsWith(QLatin1String("/media/"))))
            continue;
        if (QFileInfo(root + QStringLiteral("/DCIM")).isDir())
            out << root + QStringLiteral("/DCIM");
    }
    const QDir gvfs(QStringLiteral("/run/user/%1/gvfs").arg(getuid()));
    for (const QString &mount : gvfs.entryList({QStringLiteral("mtp:*"), QStringLiteral("gphoto2:*")}, QDir::Dirs | QDir::NoDotAndDotDot)) {
        const QDir m(gvfs.filePath(mount));
        // MTP puts each storage (internal, SD card) in its own folder.
        for (const QString &storage : m.entryList(QDir::Dirs | QDir::NoDotAndDotDot))
            if (QFileInfo(m.filePath(storage) + QStringLiteral("/DCIM")).isDir())
                out << m.filePath(storage) + QStringLiteral("/DCIM");
        if (QFileInfo(m.filePath(QStringLiteral("DCIM"))).isDir())
            out << m.filePath(QStringLiteral("DCIM"));
    }
    return out;
}

QString PhotoLibrary::sourceOf(const QString &path) const
{
    for (const QString &d : m_devices)
        if (path.startsWith(d + QLatin1Char('/')))
            return d;
    return {};
}

QVariantList PhotoLibrary::sources() const
{
    QVariantList out;
    QHash<QString, int> counts;
    int local = 0;
    for (const Photo &p : m_photos) {
        const QString s = sourceOf(p.path);
        if (s.isEmpty())
            ++local;
        else
            ++counts[s];
    }
    out << QVariantMap{{QStringLiteral("id"), QString()}, {QStringLiteral("name"), tr("On this PC")}, {QStringLiteral("icon"), QStringLiteral("computer")},
                       {QStringLiteral("count"), local}};
    for (const QString &d : m_devices) {
        // "/run/user/1000/gvfs/mtp:host=OnePlus_7_xxx/Internal shared storage/DCIM" names the phone after "host=".
        QString name = d.section(QLatin1Char('/'), -2, -2);
        const int host = d.indexOf(QLatin1String("host="));
        if (host >= 0)
            name = d.mid(host + 5).section(QLatin1Char('/'), 0, 0).section(QLatin1Char('_'), 0, -2).replace(QLatin1Char('_'), QLatin1Char(' '));
        const bool phone = d.contains(QLatin1String("/gvfs/mtp:"));
        out << QVariantMap{{QStringLiteral("id"), d}, {QStringLiteral("name"), name.isEmpty() ? tr("Device") : name},
                           {QStringLiteral("icon"), phone ? QStringLiteral("smartphone") : QStringLiteral("sd_card")}, {QStringLiteral("count"), counts.value(d)}};
    }
    return out;
}

void PhotoLibrary::loadCache()
{
    // The cached index shows the timeline at once; the scan then corrects it.
    auto future = QtConcurrent::run([] {
        QList<Photo> list;
        QFile f(cachePath());
        if (!f.open(QIODevice::ReadOnly))
            return list;
        QDataStream s(&f);
        s.setVersion(QDataStream::Qt_6_5);
        quint32 magic = 0, version = 0;
        qint32 n = 0;
        s >> magic >> version >> n;
        if (magic != CacheMagic || version != CacheVersion || n < 0 || n > 5'000'000)
            return list;
        list.reserve(n);
        for (int i = 0; i < n && s.status() == QDataStream::Ok; ++i) {
            Photo p;
            s >> p;
            list.append(p);
        }
        if (s.status() != QDataStream::Ok)
            list.clear();
        return list;
    });
    m_scanning = true;
    future.then(this, [this](QList<Photo> list) {
        if (!list.isEmpty()) {
            sortNewestFirst(list);
            m_photos = list;
            rebuildIndex();
            m_loaded = true;
            ++m_revision;
            emit changed();
        }
        // The scan uses the cache, so only new and changed files are probed.
        startScan();
    });
}

void PhotoLibrary::scheduleSave()
{
    m_saveTimer.start();
}

void PhotoLibrary::saveCache()
{
    m_saveTimer.stop();
    const QList<Photo> copy = m_photos;
    (void)QtConcurrent::run([copy] {
        QDir().mkpath(QFileInfo(cachePath()).absolutePath());
        QSaveFile f(cachePath());
        if (!f.open(QIODevice::WriteOnly))
            return;
        QDataStream s(&f);
        s.setVersion(QDataStream::Qt_6_5);
        s << CacheMagic << CacheVersion << qint32(copy.size());
        for (const Photo &p : copy)
            s << p;
        f.commit();
    });
}

void PhotoLibrary::rescan()
{
    if (m_scanning) {
        m_rescanPending = true;
        return;
    }
    startScan();
}

void PhotoLibrary::startScan()
{
    m_scanning = true;
    m_scanFound = 0;
    emit scanningChanged();
    m_devices = m_envRoots ? QStringList() : deviceRoots();
    Scanner::Options options;
    options.roots = m_roots + m_devices;
    options.excluded = m_excluded;
    options.skipSoftware = m_skipSoftware;
    options.videos = m_videos;
    QHash<QString, Photo> cache;
    cache.reserve(m_photos.size());
    for (const Photo &p : m_photos)
        cache.insert(p.path, p);
    m_cancel = std::make_shared<std::atomic_bool>(false);
    auto cancel = m_cancel;
    QPointer<PhotoLibrary> self(this);
    auto future = QtConcurrent::run([options, cache, cancel, self] {
        return Scanner::scan(options, cache, cancel.get(), [self](int found) {
            QMetaObject::invokeMethod(self, [self, found] {
                if (self) {
                    self->m_scanFound = found;
                    emit self->scanningChanged();
                }
            }, Qt::QueuedConnection);
        });
    });
    future.then(this, [this, cancel](const Scanner::Result &result) {
        if (cancel->load())
            return;
        applyScan(result);
    });
}

void PhotoLibrary::applyScan(const Scanner::Result &result)
{
    // Keep OCR text and captions from records the scan had to probe again before the cache arrived.
    QHash<QString, const Photo *> old;
    for (const Photo &p : m_photos)
        old.insert(p.path, &p);
    QList<Photo> list = result.photos;
    for (Photo &p : list) {
        const Photo *o = old.value(p.path);
        if (o && o->mtime == p.mtime && o->size == p.size && !p.textDone && o->textDone) {
            p.text = o->text;
            p.textDone = true;
        }
    }
    sortNewestFirst(list);
    const bool same = list.size() == m_photos.size() && std::equal(list.cbegin(), list.cend(), m_photos.cbegin(), [](const Photo &a, const Photo &b) {
                          return a.path == b.path && a.mtime == b.mtime && a.taken == b.taken && a.width == b.width && a.height == b.height;
                      });
    m_photos = list;
    m_skipped = result.skipped;
    rebuildIndex();
    watch(result.folders);
    m_loaded = true;
    m_scanning = false;
    if (!same)
        ++m_revision;
    emit scanningChanged();
    emit changed();
    scheduleSave();
    queueOcr();
    if (m_rescanPending) {
        m_rescanPending = false;
        startScan();
    }
}

void PhotoLibrary::rebuildIndex()
{
    m_index.clear();
    m_haystack.clear();
    m_screenshots = m_videoCount = 0;
    for (int i = 0; i < m_photos.size(); ++i) {
        m_index.insert(m_photos[i].path, i);
        m_screenshots += m_photos[i].screenshot;
        m_videoCount += m_photos[i].video;
    }
}

void PhotoLibrary::watch(const QStringList &folders)
{
    const QStringList old = m_watcher.directories();
    QSet<QString> want(folders.cbegin(), folders.cend());
    // Device mount points, so a phone that is plugged in appears without a manual rescan.
    for (const QString &m : {QStringLiteral("/run/media/") + qEnvironmentVariable("USER"), QStringLiteral("/run/user/%1/gvfs").arg(getuid())})
        if (!m_envRoots && QFileInfo(m).isDir())
            want.insert(m);
    QStringList remove, add;
    for (const QString &d : old)
        if (!want.contains(d))
            remove << d;
    const QSet<QString> have(old.cbegin(), old.cend());
    for (const QString &d : want)
        if (!have.contains(d))
            add << d;
    if (!remove.isEmpty())
        m_watcher.removePaths(remove);
    if (!add.isEmpty())
        m_watcher.addPaths(add);
}

const Photo *PhotoLibrary::find(const QString &path) const
{
    const int i = indexOf(path);
    return i >= 0 ? &m_photos[i] : nullptr;
}

const QString &PhotoLibrary::haystack(int i) const
{
    auto it = m_haystack.find(i);
    if (it != m_haystack.end())
        return *it;
    const Photo &p = m_photos[i];
    const QDateTime t = QDateTime::fromMSecsSinceEpoch(p.taken);
    const QLocale c = QLocale::c(), sys;
    QStringList words{p.name(), p.folder.section(QLatin1Char('/'), -1), t.toString(QStringLiteral("yyyy MMMM MMM d")), c.toString(t, QStringLiteral("MMMM MMM dddd")),
                      sys.toString(t, QStringLiteral("MMMM MMM dddd")), p.camera, p.caption, p.text};
    if (p.screenshot)
        words << QStringLiteral("screenshot");
    if (p.video)
        words << QStringLiteral("video");
    return *m_haystack.insert(i, words.join(QLatin1Char(' ')).toLower());
}

QVariantMap PhotoLibrary::toMap(const Photo &p)
{
    return {{QStringLiteral("path"), p.path},
            {QStringLiteral("uri"), uri(p.path)},
            {QStringLiteral("name"), p.name()},
            {QStringLiteral("folder"), p.folder},
            {QStringLiteral("folderName"), p.folder.section(QLatin1Char('/'), -1)},
            {QStringLiteral("width"), p.width},
            {QStringLiteral("height"), p.height},
            {QStringLiteral("aspect"), p.aspect()},
            {QStringLiteral("size"), p.size},
            {QStringLiteral("mtime"), p.mtime},
            {QStringLiteral("taken"), QDateTime::fromMSecsSinceEpoch(p.taken)},
            {QStringLiteral("dateSource"), int(p.dateSource)},
            {QStringLiteral("video"), p.video},
            {QStringLiteral("screenshot"), p.screenshot},
            {QStringLiteral("camera"), p.camera},
            {QStringLiteral("caption"), p.caption},
            {QStringLiteral("text"), p.text},
            {QStringLiteral("thumb"), thumb(p.path, p.mtime)}};
}

QVariantMap PhotoLibrary::photo(const QString &path) const
{
    const Photo *p = find(path);
    if (!p)
        return {};
    QVariantMap m = toMap(*p);
    m.insert(QStringLiteral("favourite"), isFavourite(path));
    return m;
}

QString PhotoLibrary::uri(const QString &path)
{
    return QUrl::fromLocalFile(path).toString(QUrl::FullyEncoded);
}

QUrl PhotoLibrary::thumb(const QString &path, qint64 mtime)
{
    return Thumbnailer::url(uri(path), mtime);
}

int PhotoLibrary::favouriteCount() const
{
    int n = 0;
    for (const QString &u : Starred::instance()->uris())
        if (m_index.contains(QUrl(u).toLocalFile()))
            ++n;
    return n;
}

bool PhotoLibrary::isFavourite(const QString &path) const
{
    return Starred::instance()->isStarred(uri(path));
}

void PhotoLibrary::toggleFavourite(const QString &path)
{
    Starred::instance()->toggle(uri(path));
}

void PhotoLibrary::setFavourite(const QStringList &paths, bool on)
{
    for (const QString &p : paths) {
        if (on)
            Starred::instance()->star(uri(p));
        else
            Starred::instance()->unstar(uri(p));
    }
}

void PhotoLibrary::setCaption(const QString &path, const QString &caption)
{
    const int i = indexOf(path);
    if (i < 0 || m_photos[i].caption == caption)
        return;
    m_photos[i].caption = caption;
    m_haystack.remove(i);
    emit textChanged(path);
    scheduleSave();
    QtConcurrent::run([path, caption] { return Scanner::writeCaption(path, caption); }).then(this, [this, path](bool ok) {
        if (!ok)
            emit captionFailed(path);
    });
}

void PhotoLibrary::queueOcr()
{
    if (!ocrEnabled())
        return;
    QStringList q;
    for (const Photo &p : m_photos)
        if (p.screenshot && !p.textDone && !p.video)
            q << p.path;
    m_ocrQueue = q;
    emit ocrChanged();
    if (!m_ocr)
        QTimer::singleShot(2000, this, &PhotoLibrary::runOcr);
}

void PhotoLibrary::runOcr()
{
    if (m_ocr || !ocrEnabled() || m_ocrQueue.isEmpty())
        return;
    m_ocrPath = m_ocrQueue.takeFirst();
    m_ocr = new QProcess(this);
    m_ocr->setProcessChannelMode(QProcess::SeparateChannels);
    // Sparse-text mode suits screenshots; nice keeps the background work out of the way.
    QStringList args{m_ocrPath, QStringLiteral("stdout"), QStringLiteral("--psm"), QStringLiteral("11")};
    if (!m_nice.isEmpty())
        m_ocr->start(m_nice, QStringList{QStringLiteral("-n"), QStringLiteral("19"), m_tesseract} + args);
    else
        m_ocr->start(m_tesseract, args);
    connect(m_ocr, &QProcess::finished, this, [this](int code, QProcess::ExitStatus status) {
        const QString text = QString::fromUtf8(m_ocr->readAllStandardOutput()).simplified();
        m_ocr->deleteLater();
        m_ocr = nullptr;
        const int i = indexOf(m_ocrPath);
        if (i >= 0) {
            // A failed run also counts as done, so a broken file is not tried at every start.
            m_photos[i].text = status == QProcess::NormalExit && code == 0 ? text.left(4000) : QString();
            m_photos[i].textDone = true;
            m_haystack.remove(i);
            emit textChanged(m_ocrPath);
            scheduleSave();
        }
        emit ocrChanged();
        QTimer::singleShot(50, this, &PhotoLibrary::runOcr);
    });
    connect(m_ocr, &QProcess::errorOccurred, this, [this](QProcess::ProcessError e) {
        if (e != QProcess::FailedToStart)
            return;
        m_ocr->deleteLater();
        m_ocr = nullptr;
        m_tesseract.clear();
        m_ocrQueue.clear();
        emit ocrChanged();
    });
}

bool PhotoLibrary::waitForScan(int msecs)
{
    QElapsedTimer t;
    t.start();
    while ((m_scanning || !m_loaded) && t.elapsed() < msecs)
        QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
    return !m_scanning && m_loaded;
}
