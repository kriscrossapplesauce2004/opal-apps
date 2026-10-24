#include "medialibrary.h"

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QDirIterator>
#include <QElapsedTimer>
#include <QRegularExpression>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QJSEngine>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPointer>
#include <QProcess>
#include <QSaveFile>
#include <QSet>
#include <QSettings>
#include <QStandardPaths>
#include <QTextStream>
#include <QUrl>
#include <QtConcurrent>
#include <algorithm>

#include <taglib/fileref.h>
#include <taglib/tag.h>
#include <taglib/tpropertymap.h>
#include <taglib/audioproperties.h>

namespace {

constexpr quint32 CacheMagic = 0x4f4d4544; // "OMED"
constexpr quint32 CacheVersion = 4;

QString cachePath()
{
    return QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation) + QStringLiteral("/opal/media/index.bin");
}

QSettings *settings()
{
    static QSettings *s = new QSettings(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + QStringLiteral("/opal/media.ini"),
                                        QSettings::IniFormat, QCoreApplication::instance());
    return s;
}

QString home(const QString &sub)
{
    return QDir::homePath() + QLatin1Char('/') + sub;
}

QString hashKey(const QString &s)
{
    return QString::fromLatin1(QCryptographicHash::hash(s.toUtf8(), QCryptographicHash::Sha1).toHex().left(20));
}

QString tagText(const TagLib::PropertyMap &props, const char *key)
{
    const auto it = props.find(key);
    if (it == props.end() || it->second.isEmpty())
        return {};
    return QString::fromStdString(it->second.front().to8Bit(true)).trimmed();
}

int leadingNumber(const QString &s)
{
    int n = 0;
    for (const QChar c : s) {
        if (!c.isDigit())
            break;
        n = n * 10 + c.digitValue();
    }
    return n;
}

// "05 - Title [videoId]" and "Artist - Title" names give a readable title when tags are missing.
QString titleFromName(const QString &file)
{
    QString base = QFileInfo(file).completeBaseName();
    static const QRegularExpression ytId(QStringLiteral("\\s*\\[[A-Za-z0-9_-]{11}\\]$"));
    base.remove(ytId);
    static const QRegularExpression lead(QStringLiteral("^\\d{1,3}\\s*[-._)]\\s*"));
    base.remove(lead);
    return base.replace(QLatin1Char('_'), QLatin1Char(' ')).trimmed();
}

// Downloads often carry "Title (Album) - Artist" in the title and no artist tag: split the artist off and drop the repeated album.
void tidyTitle(Track &t, const QString &folder)
{
    static const QRegularExpression lead(QStringLiteral("^\\d{1,3}\\s*[.)]\\s+"));
    t.title.remove(lead);
    if (t.artist.isEmpty()) {
        const int dash = t.title.lastIndexOf(QStringLiteral(" - "));
        if (dash > 0 && t.title.size() - dash - 3 <= 40) {
            t.artist = t.title.mid(dash + 3).trimmed();
            t.title = t.title.left(dash).trimmed();
        }
    }
    const QString album = (t.album.isEmpty() ? folder : t.album).toLower();
    static const QRegularExpression paren(QStringLiteral("\\s*[(\\[]([^)\\]]+)[)\\]]\\s*$"));
    const auto m = paren.match(t.title);
    if (m.hasMatch() && m.capturedStart() > 0 && album.contains(m.captured(1).toLower().remove(QStringLiteral(" soundtrack")).trimmed()))
        t.title = t.title.left(m.capturedStart()).trimmed();
}

QString settingsKey(const QString &s)
{
    return hashKey(s);
}

}

MediaLibrary::MediaLibrary(QObject *parent)
    : QObject(parent)
{
    m_rescanTimer.setSingleShot(true);
    m_rescanTimer.setInterval(1500);
    connect(&m_rescanTimer, &QTimer::timeout, this, &MediaLibrary::rescan);
    m_saveTimer.setSingleShot(true);
    m_saveTimer.setInterval(3000);
    connect(&m_saveTimer, &QTimer::timeout, this, &MediaLibrary::saveCache);
    connect(&m_watcher, &QFileSystemWatcher::directoryChanged, this, [this] { m_rescanTimer.start(); });
    loadSettings();
    loadCache();
}

MediaLibrary::~MediaLibrary()
{
    if (m_cancel)
        m_cancel->store(true);
    if (m_saveTimer.isActive())
        saveCache();
}

MediaLibrary *MediaLibrary::instance()
{
    static MediaLibrary *l = new MediaLibrary(QCoreApplication::instance());
    return l;
}

MediaLibrary *MediaLibrary::create(QQmlEngine *, QJSEngine *)
{
    auto *l = instance();
    QJSEngine::setObjectOwnership(l, QJSEngine::CppOwnership);
    return l;
}

QString MediaLibrary::coverDir()
{
    return QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation) + QStringLiteral("/opal/media/covers");
}

QString MediaLibrary::thumbDir()
{
    return QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation) + QStringLiteral("/opal/media/video-thumbs");
}

// ---------- settings ----------

void MediaLibrary::loadSettings()
{
    auto envList = [](const char *name) { return qEnvironmentVariable(name).split(QLatin1Char(':'), Qt::SkipEmptyParts); };
    if (qEnvironmentVariableIsSet("OPAL_MEDIA_MUSIC") || qEnvironmentVariableIsSet("OPAL_MEDIA_VIDEOS") || qEnvironmentVariableIsSet("OPAL_MEDIA_BOOKS")) {
        m_envRoots = true;
        m_music = envList("OPAL_MEDIA_MUSIC");
        m_books = envList("OPAL_MEDIA_BOOKS");
        m_videoRoots = envList("OPAL_MEDIA_VIDEOS");
    } else {
        QSettings *s = settings();
        const QString music = QStandardPaths::writableLocation(QStandardPaths::MusicLocation);
        const QString videos = QStandardPaths::writableLocation(QStandardPaths::MoviesLocation);
        m_music = s->value(QStringLiteral("library/music"), QStringList{music.isEmpty() ? home(QStringLiteral("Music")) : music}).toStringList();
        QStringList books;
        for (const QString &b : {home(QStringLiteral("Audiobooks")), m_music.value(0) + QStringLiteral("/Audiobooks")})
            if (QFileInfo(b).isDir())
                books << b;
        m_books = s->value(QStringLiteral("library/books"), books).toStringList();
        m_videoRoots = s->value(QStringLiteral("library/videos"), QStringList{videos.isEmpty() ? home(QStringLiteral("Videos")) : videos}).toStringList();
    }
    QSettings *s = settings();
    const QStringList liked = s->value(QStringLiteral("music/liked")).toStringList();
    m_liked = QSet<QString>(liked.cbegin(), liked.cend());
    m_recent = s->value(QStringLiteral("music/recent")).toStringList();
}

void MediaLibrary::saveSettings()
{
    emit settingsChanged();
    if (m_envRoots)
        return;
    QSettings *s = settings();
    s->setValue(QStringLiteral("library/music"), m_music);
    s->setValue(QStringLiteral("library/books"), m_books);
    s->setValue(QStringLiteral("library/videos"), m_videoRoots);
    s->sync();
}

void MediaLibrary::addRoot(const QString &kind, const QString &urlOrPath)
{
    const QUrl url(urlOrPath);
    const QString path = QDir::cleanPath(url.isLocalFile() ? url.toLocalFile() : urlOrPath);
    if (!QFileInfo(path).isDir())
        return;
    QStringList &list = kind == QLatin1String("books") ? m_books : kind == QLatin1String("videos") ? m_videoRoots : m_music;
    if (list.contains(path))
        return;
    list << path;
    saveSettings();
    rescan();
}

void MediaLibrary::removeRoot(const QString &kind, const QString &path)
{
    QStringList &list = kind == QLatin1String("books") ? m_books : kind == QLatin1String("videos") ? m_videoRoots : m_music;
    if (list.removeAll(path)) {
        saveSettings();
        rescan();
    }
}

// ---------- cache ----------

void MediaLibrary::loadCache()
{
    m_scanning = true;
    QtConcurrent::run([] {
        QList<Track> list;
        QFile f(cachePath());
        if (!f.open(QIODevice::ReadOnly))
            return list;
        QDataStream s(&f);
        s.setVersion(QDataStream::Qt_6_5);
        quint32 magic = 0, version = 0;
        qint32 n = 0;
        s >> magic >> version >> n;
        if (magic != CacheMagic || version != CacheVersion || n < 0 || n > 2'000'000)
            return list;
        list.reserve(n);
        for (int i = 0; i < n && s.status() == QDataStream::Ok; ++i) {
            Track t;
            s >> t;
            list.append(t);
        }
        if (s.status() != QDataStream::Ok)
            list.clear();
        return list;
    }).then(this, [this](const QList<Track> &list) {
        if (!list.isEmpty()) {
            m_tracks = list;
            rebuild();
            m_loaded = true;
            ++m_revision;
            emit changed();
        }
        startScan();
    });
}

void MediaLibrary::saveCache()
{
    m_saveTimer.stop();
    const QList<Track> copy = m_tracks;
    (void)QtConcurrent::run([copy] {
        QDir().mkpath(QFileInfo(cachePath()).absolutePath());
        QSaveFile f(cachePath());
        if (!f.open(QIODevice::WriteOnly))
            return;
        QDataStream s(&f);
        s.setVersion(QDataStream::Qt_6_5);
        s << CacheMagic << CacheVersion << qint32(copy.size());
        for (const Track &t : copy)
            s << t;
        f.commit();
    });
}

// ---------- scanning ----------

bool MediaLibrary::isAudioName(const QString &name)
{
    static const QStringList ext{QStringLiteral("mp3"), QStringLiteral("flac"), QStringLiteral("ogg"), QStringLiteral("oga"), QStringLiteral("opus"),
                                 QStringLiteral("m4a"), QStringLiteral("m4b"), QStringLiteral("aac"), QStringLiteral("wav"), QStringLiteral("wma"),
                                 QStringLiteral("ape"), QStringLiteral("wv"), QStringLiteral("aiff"), QStringLiteral("aif"), QStringLiteral("mka")};
    return ext.contains(name.section(QLatin1Char('.'), -1).toLower());
}

bool MediaLibrary::isVideoName(const QString &name)
{
    static const QStringList ext{QStringLiteral("mp4"), QStringLiteral("mkv"), QStringLiteral("webm"), QStringLiteral("mov"), QStringLiteral("avi"),
                                 QStringLiteral("m4v"), QStringLiteral("wmv"), QStringLiteral("flv"), QStringLiteral("mpg"), QStringLiteral("mpeg"),
                                 QStringLiteral("ts"), QStringLiteral("ogv")};
    return ext.contains(name.section(QLatin1Char('.'), -1).toLower());
}

bool MediaLibrary::isSoftwareFolder(const QString &folder)
{
    // Game, app and source folders keep sound effects and clips that do not belong in a music library.
    static const QStringList names{QStringLiteral(".git"), QStringLiteral("CMakeLists.txt"), QStringLiteral("package.json"), QStringLiteral("Cargo.toml"),
                                   QStringLiteral("meson.build"), QStringLiteral("Makefile"), QStringLiteral("data.win"), QStringLiteral(".nomedia"),
                                   QStringLiteral("steam_appid.txt"), QStringLiteral("pyproject.toml"), QStringLiteral("build.gradle")};
    static const QStringList suffixes{QStringLiteral(".exe"), QStringLiteral(".dll"), QStringLiteral(".so"), QStringLiteral(".x86_64"), QStringLiteral(".AppImage"),
                                      QStringLiteral(".pck"), QStringLiteral(".unity3d"), QStringLiteral(".qml"), QStringLiteral(".py"), QStringLiteral(".jar")};
    const QDir dir(folder);
    for (const QString &n : dir.entryList(QDir::AllEntries | QDir::Hidden | QDir::NoDotAndDotDot)) {
        if (names.contains(n))
            return true;
        for (const QString &s : suffixes)
            if (n.endsWith(s, Qt::CaseInsensitive))
                return true;
    }
    return false;
}

QString MediaLibrary::folderCover(const QString &folder)
{
    const QDir dir(folder);
    const QStringList images = dir.entryList({QStringLiteral("*.jpg"), QStringLiteral("*.jpeg"), QStringLiteral("*.png"), QStringLiteral("*.webp")}, QDir::Files, QDir::Name);
    for (const QString &want : {QStringLiteral("cover"), QStringLiteral("folder"), QStringLiteral("front"), QStringLiteral("albumart"), QStringLiteral("album")})
        for (const QString &img : images)
            if (img.section(QLatin1Char('.'), 0, 0).toLower() == want)
                return dir.filePath(img);
    return images.size() == 1 ? dir.filePath(images.first()) : QString();
}

bool MediaLibrary::probeAudio(const QString &path, Track &out, const QString &coverDir, bool single)
{
    const QFileInfo fi(path);
    TagLib::FileRef f(QFile::encodeName(path).constData(), true, TagLib::AudioProperties::Fast);
    if (f.isNull())
        return false;
    out.path = path;
    out.mtime = fi.lastModified().toSecsSinceEpoch();
    out.size = fi.size();
    if (f.audioProperties())
        out.duration = f.audioProperties()->lengthInMilliseconds();
    if (f.tag()) {
        const TagLib::PropertyMap props = f.properties();
        out.title = tagText(props, "TITLE");
        out.artist = tagText(props, "ARTIST");
        out.album = tagText(props, "ALBUM");
        out.albumArtist = tagText(props, "ALBUMARTIST");
        out.genre = tagText(props, "GENRE");
        out.year = leadingNumber(tagText(props, "DATE"));
        out.number = leadingNumber(tagText(props, "TRACKNUMBER"));
        out.disc = leadingNumber(tagText(props, "DISCNUMBER"));
    }
    if (out.title.isEmpty())
        out.title = titleFromName(path);
    if (!out.number)
        out.number = leadingNumber(fi.completeBaseName());
    tidyTitle(out, fi.dir().dirName());
    // Embedded art is written once per album; a cover file next to the music wins when there is one.
    out.cover = single ? QString() : folderCover(fi.absolutePath());
    if (out.cover.isEmpty() && !coverDir.isEmpty()) {
        const QString key = single ? path : out.kind == Track::Book ? fi.absolutePath() : out.albumKey() + fi.absolutePath();
        const QString dest = coverDir + QLatin1Char('/') + hashKey(key) + QStringLiteral(".jpg");
        if (QFileInfo::exists(dest)) {
            out.cover = dest;
        } else {
            const auto pictures = f.complexProperties("PICTURE");
            for (const auto &pic : pictures) {
                const auto it = pic.find("data");
                if (it == pic.end())
                    continue;
                const TagLib::ByteVector bytes = it->second.toByteVector();
                QImage img = QImage::fromData(reinterpret_cast<const uchar *>(bytes.data()), int(bytes.size()));
                if (img.isNull())
                    continue;
                if (img.width() > 800)
                    img = img.scaledToWidth(800, Qt::SmoothTransformation);
                QDir().mkpath(coverDir);
                if (img.save(dest, "JPG", 88))
                    out.cover = dest;
                break;
            }
        }
    }
    return true;
}

bool MediaLibrary::probeVideo(const QString &path, Track &out, const QString &thumbDir)
{
    const QFileInfo fi(path);
    out.path = path;
    out.kind = Track::Video;
    out.mtime = fi.lastModified().toSecsSinceEpoch();
    out.size = fi.size();
    out.title = titleFromName(path);
    const QString ffprobe = QStandardPaths::findExecutable(QStringLiteral("ffprobe"));
    if (!ffprobe.isEmpty()) {
        QProcess p;
        p.start(ffprobe, {QStringLiteral("-v"), QStringLiteral("quiet"), QStringLiteral("-print_format"), QStringLiteral("json"), QStringLiteral("-show_format"),
                          QStringLiteral("-show_streams"), QStringLiteral("-select_streams"), QStringLiteral("v:0"), path});
        if (p.waitForFinished(8000)) {
            const QJsonObject root = QJsonDocument::fromJson(p.readAllStandardOutput()).object();
            out.duration = qint64(root.value(QStringLiteral("format")).toObject().value(QStringLiteral("duration")).toString().toDouble() * 1000);
            const QJsonArray streams = root.value(QStringLiteral("streams")).toArray();
            if (!streams.isEmpty()) {
                out.width = streams.first().toObject().value(QStringLiteral("width")).toInt();
                out.height = streams.first().toObject().value(QStringLiteral("height")).toInt();
            }
            const QString tagTitle = root.value(QStringLiteral("format")).toObject().value(QStringLiteral("tags")).toObject().value(QStringLiteral("title")).toString();
            if (!tagTitle.trimmed().isEmpty())
                out.title = tagTitle.trimmed();
        } else {
            p.kill();
        }
    }
    // A frame a tenth of the way in, as the poster.
    const QString ffmpeg = QStandardPaths::findExecutable(QStringLiteral("ffmpeg"));
    if (!ffmpeg.isEmpty() && !thumbDir.isEmpty()) {
        const QString dest = thumbDir + QLatin1Char('/') + hashKey(path + QString::number(out.mtime)) + QStringLiteral(".jpg");
        if (!QFileInfo::exists(dest)) {
            QDir().mkpath(thumbDir);
            const double at = out.duration > 0 ? std::min(out.duration / 10000.0, 60.0) : 1.0;
            QProcess p;
            p.start(ffmpeg, {QStringLiteral("-v"), QStringLiteral("quiet"), QStringLiteral("-ss"), QString::number(at, 'f', 2), QStringLiteral("-i"), path,
                             QStringLiteral("-frames:v"), QStringLiteral("1"), QStringLiteral("-vf"), QStringLiteral("scale=480:-2"), QStringLiteral("-y"), dest});
            if (!p.waitForFinished(15000))
                p.kill();
        }
        if (QFileInfo::exists(dest))
            out.cover = dest;
    }
    return true;
}

MediaLibrary::Result MediaLibrary::scan(const Options &options, const QHash<QString, Track> &cache, const QString &coverDirPath, const std::atomic_bool *cancel,
                                        const std::function<void(int)> &progress)
{
    Result result;
    QSet<QString> seenFolders, seenFiles;
    const qint64 now = QDateTime::currentSecsSinceEpoch();
    QSet<QString> bookRoots;
    for (const QString &b : options.books)
        bookRoots.insert(QDir::cleanPath(b));
    int found = 0;
    auto walk = [&](const QString &root, quint8 kind) {
        QList<QString> stack{QDir::cleanPath(root)};
        while (!stack.isEmpty()) {
            if (cancel && cancel->load())
                return;
            const QString folder = stack.takeLast();
            const QString canonical = QFileInfo(folder).canonicalFilePath();
            if (canonical.isEmpty() || seenFolders.contains(canonical))
                continue;
            // Book folders under the music folder belong to the book scan.
            if (kind == Track::Music && bookRoots.contains(folder))
                continue;
            seenFolders.insert(canonical);
            if (folder != QDir::cleanPath(root) && isSoftwareFolder(folder)) {
                result.skipped << folder;
                continue;
            }
            result.folders << folder;
            const QFileInfoList entries = QDir(folder).entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
            for (const QFileInfo &e : entries) {
                if (e.isDir()) {
                    if (!e.isSymLink())
                        stack << e.absoluteFilePath();
                    continue;
                }
                const QString name = e.fileName();
                if (kind == Track::Video ? !isVideoName(name) : !isAudioName(name))
                    continue;
                const QString path = e.absoluteFilePath();
                if (seenFiles.contains(path))
                    continue;
                seenFiles.insert(path);
                const auto hit = cache.constFind(path);
                if (hit != cache.constEnd() && hit->kind == kind && hit->mtime == e.lastModified().toSecsSinceEpoch() && hit->size == e.size()) {
                    result.tracks << *hit;
                } else {
                    Track t;
                    t.kind = kind;
                    const bool ok = kind == Track::Video ? probeVideo(path, t, thumbDir()) : probeAudio(path, t, coverDirPath, kind == Track::Music && folder == QDir::cleanPath(root));
                    if (!ok)
                        continue;
                    // Untagged albums are their folder, which is how soundtrack downloads come; loose files at the top are singles.
                    if (t.album.isEmpty() && kind == Track::Music)
                        t.album = folder == QDir::cleanPath(root) ? t.title : QFileInfo(folder).fileName();
                    t.added = hit != cache.constEnd() && hit->added ? hit->added : now;
                    result.tracks << t;
                }
                if (progress && ++found % 25 == 0)
                    progress(found);
            }
        }
    };
    for (const QString &r : options.books)
        walk(r, Track::Book);
    for (const QString &r : options.music)
        walk(r, Track::Music);
    for (const QString &r : options.videos)
        walk(r, Track::Video);
    return result;
}

void MediaLibrary::rescan()
{
    if (m_scanning) {
        m_rescanPending = true;
        return;
    }
    startScan();
}

void MediaLibrary::startScan()
{
    m_scanning = true;
    m_scanFound = 0;
    emit scanningChanged();
    Options options{m_music, m_books, m_videoRoots};
    QHash<QString, Track> cache;
    cache.reserve(m_tracks.size());
    for (const Track &t : m_tracks)
        cache.insert(t.path, t);
    m_cancel = std::make_shared<std::atomic_bool>(false);
    auto cancel = m_cancel;
    QPointer<MediaLibrary> self(this);
    const QString covers = coverDir();
    QtConcurrent::run([options, cache, cancel, self, covers] {
        return scan(options, cache, covers, cancel.get(), [self](int n) {
            QMetaObject::invokeMethod(self, [self, n] {
                if (self) {
                    self->m_scanFound = n;
                    emit self->scanningChanged();
                }
            }, Qt::QueuedConnection);
        });
    }).then(this, [this, cancel](const Result &r) {
        if (!cancel->load())
            apply(r);
    });
}

void MediaLibrary::apply(const Result &r)
{
    m_tracks = r.tracks;
    m_skipped = r.skipped;
    rebuild();
    const QStringList old = m_watcher.directories();
    if (!old.isEmpty())
        m_watcher.removePaths(old);
    if (!r.folders.isEmpty())
        m_watcher.addPaths(r.folders.mid(0, 4000));
    m_loaded = true;
    m_scanning = false;
    ++m_revision;
    emit scanningChanged();
    emit changed();
    m_saveTimer.start();
    if (m_rescanPending) {
        m_rescanPending = false;
        startScan();
    }
}

bool MediaLibrary::waitForScan(int msecs)
{
    QElapsedTimer t;
    t.start();
    while ((m_scanning || !m_loaded) && t.elapsed() < msecs)
        QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
    return !m_scanning && m_loaded;
}

// ---------- index ----------

void MediaLibrary::rebuild()
{
    m_index.clear();
    m_albums.clear();
    m_albumOrder.clear();
    m_bookInfo.clear();
    m_bookOrder.clear();
    m_songs = m_videos = 0;
    QSet<QString> genres;
    // Books group by the folder under a book root; a single file is a book by itself.
    QHash<QString, QString> bookRootOf;
    for (int i = 0; i < m_tracks.size(); ++i) {
        const Track &t = m_tracks[i];
        m_index.insert(t.path, i);
        if (t.kind == Track::Video) {
            ++m_videos;
            continue;
        }
        if (t.kind == Track::Book) {
            QString key;
            for (const QString &root : m_books)
                if (t.path.startsWith(QDir::cleanPath(root) + QLatin1Char('/'))) {
                    const QString rest = t.path.mid(QDir::cleanPath(root).size() + 1);
                    key = rest.contains(QLatin1Char('/')) ? QDir::cleanPath(root) + QLatin1Char('/') + rest.section(QLatin1Char('/'), 0, 0) : t.path;
                    break;
                }
            if (key.isEmpty())
                key = t.folder();
            BookInfo &b = m_bookInfo[key];
            if (b.key.isEmpty()) {
                b.key = key;
                b.title = key == t.path ? (t.album.isEmpty() ? t.title : t.album) : (t.album.isEmpty() ? key.section(QLatin1Char('/'), -1) : t.album);
                b.author = t.albumArtist.isEmpty() ? t.artist : t.albumArtist;
                m_bookOrder << key;
            }
            if (b.cover.isEmpty())
                b.cover = t.cover;
            b.duration += t.duration;
            b.tracks << i;
            continue;
        }
        ++m_songs;
        if (!t.genre.isEmpty())
            genres.insert(t.genre);
        const QString key = t.albumKey();
        AlbumInfo &a = m_albums[key];
        if (a.key.isEmpty()) {
            a.key = key;
            a.title = t.album;
            a.artist = t.albumArtist.isEmpty() ? t.artist : t.albumArtist;
            m_albumOrder << key;
        }
        if (a.cover.isEmpty())
            a.cover = t.cover;
        if (a.genre.isEmpty())
            a.genre = t.genre;
        a.year = std::max(a.year, t.year);
        a.duration += t.duration;
        a.added = std::max(a.added, t.added);
        a.tracks << i;
    }
    auto byNumber = [this](int x, int y) {
        const Track &a = m_tracks[x], &b = m_tracks[y];
        if (a.disc != b.disc)
            return a.disc < b.disc;
        if (a.number != b.number)
            return a.number < b.number;
        return a.path < b.path;
    };
    for (AlbumInfo &a : m_albums) {
        std::sort(a.tracks.begin(), a.tracks.end(), byNumber);
        // Without an album artist, the artist on most tracks names the album; many different artists make a compilation.
        if (m_tracks[a.tracks.first()].albumArtist.isEmpty()) {
            QHash<QString, int> votes;
            for (int i : a.tracks)
                votes[m_tracks[i].artist]++;
            QString top;
            int best = 0;
            for (auto it = votes.cbegin(); it != votes.cend(); ++it)
                if (it.value() > best) {
                    top = it.key();
                    best = it.value();
                }
            a.artist = votes.size() > 2 && best * 2 < a.tracks.size() ? tr("Various artists") : top;
        }
        if (a.artist.isEmpty())
            a.artist = tr("Unknown artist");
    }
    for (BookInfo &b : m_bookInfo)
        std::sort(b.tracks.begin(), b.tracks.end(), [this](int x, int y) { return m_tracks[x].path.localeAwareCompare(m_tracks[y].path) < 0; });
    m_genres = QStringList(genres.cbegin(), genres.cend());
    m_genres.sort(Qt::CaseInsensitive);
}

const Track *MediaLibrary::find(const QString &path) const
{
    const int i = m_index.value(path, -1);
    return i >= 0 ? &m_tracks[i] : nullptr;
}

QString MediaLibrary::formatDuration(qint64 ms)
{
    const qint64 s = std::max<qint64>(0, ms / 1000);
    if (s >= 3600)
        return QStringLiteral("%1:%2:%3").arg(s / 3600).arg((s / 60) % 60, 2, 10, QLatin1Char('0')).arg(s % 60, 2, 10, QLatin1Char('0'));
    return QStringLiteral("%1:%2").arg(s / 60).arg(s % 60, 2, 10, QLatin1Char('0'));
}

QVariantMap MediaLibrary::toMap(const Track &t) const
{
    return {{QStringLiteral("path"), t.path},
            {QStringLiteral("url"), QUrl::fromLocalFile(t.path).toString()},
            {QStringLiteral("kind"), t.kind == Track::Video ? QStringLiteral("video") : t.kind == Track::Book ? QStringLiteral("book") : QStringLiteral("music")},
            {QStringLiteral("title"), t.title},
            {QStringLiteral("artist"), t.artist.isEmpty() ? (t.albumArtist.isEmpty() ? tr("Unknown artist") : t.albumArtist) : t.artist},
            {QStringLiteral("album"), t.album},
            {QStringLiteral("albumKey"), t.kind == Track::Music ? t.albumKey() : QString()},
            {QStringLiteral("genre"), t.genre},
            {QStringLiteral("year"), t.year},
            {QStringLiteral("number"), t.number},
            {QStringLiteral("disc"), t.disc},
            {QStringLiteral("duration"), t.duration},
            {QStringLiteral("length"), formatDuration(t.duration)},
            {QStringLiteral("cover"), t.cover.isEmpty() ? QString() : QUrl::fromLocalFile(t.cover).toString()},
            {QStringLiteral("liked"), m_liked.contains(t.path)},
            {QStringLiteral("width"), t.width},
            {QStringLiteral("height"), t.height}};
}

QVariantMap MediaLibrary::track(const QString &path) const
{
    const Track *t = find(path);
    return t ? toMap(*t) : QVariantMap();
}

QVariantList MediaLibrary::trackList(const QList<int> &indices) const
{
    QVariantList out;
    out.reserve(indices.size());
    for (int i : indices)
        out << toMap(m_tracks[i]);
    return out;
}

bool MediaLibrary::matches(const Track &t, const QString &q) const
{
    if (q.isEmpty())
        return true;
    return t.title.contains(q, Qt::CaseInsensitive) || t.artist.contains(q, Qt::CaseInsensitive) || t.album.contains(q, Qt::CaseInsensitive)
        || t.albumArtist.contains(q, Qt::CaseInsensitive) || t.genre.contains(q, Qt::CaseInsensitive);
}

QVariantMap MediaLibrary::albumMap(const AlbumInfo &a, bool withTracks) const
{
    QVariantMap m{{QStringLiteral("key"), a.key},
                  {QStringLiteral("title"), a.title},
                  {QStringLiteral("artist"), a.artist},
                  {QStringLiteral("year"), a.year},
                  {QStringLiteral("genre"), a.genre},
                  {QStringLiteral("cover"), a.cover.isEmpty() ? QString() : QUrl::fromLocalFile(a.cover).toString()},
                  {QStringLiteral("count"), int(a.tracks.size())},
                  {QStringLiteral("duration"), a.duration},
                  {QStringLiteral("length"), formatDuration(a.duration)},
                  {QStringLiteral("added"), a.added}};
    if (withTracks)
        m.insert(QStringLiteral("tracks"), trackList(a.tracks));
    return m;
}

QVariantList MediaLibrary::albums(const QString &query, const QString &genre, const QString &sort) const
{
    QList<const AlbumInfo *> list;
    for (const QString &k : m_albumOrder) {
        const AlbumInfo &a = *m_albums.constFind(k);
        if (!genre.isEmpty() && a.genre != genre)
            continue;
        if (!query.isEmpty() && !a.title.contains(query, Qt::CaseInsensitive) && !a.artist.contains(query, Qt::CaseInsensitive))
            continue;
        list << &a;
    }
    std::sort(list.begin(), list.end(), [&sort](const AlbumInfo *x, const AlbumInfo *y) {
        if (sort == QLatin1String("title"))
            return x->title.localeAwareCompare(y->title) < 0;
        if (sort == QLatin1String("artist"))
            return x->artist.localeAwareCompare(y->artist) != 0 ? x->artist.localeAwareCompare(y->artist) < 0 : x->year < y->year;
        if (sort == QLatin1String("year"))
            return x->year > y->year;
        return x->added != y->added ? x->added > y->added : x->title.localeAwareCompare(y->title) < 0;
    });
    QVariantList out;
    for (const AlbumInfo *a : list)
        out << albumMap(*a, false);
    return out;
}

QVariantMap MediaLibrary::album(const QString &key) const
{
    const auto it = m_albums.constFind(key);
    return it == m_albums.constEnd() ? QVariantMap() : albumMap(*it, true);
}

QVariantList MediaLibrary::artists(const QString &query) const
{
    struct A { QString name; QSet<QString> albums; int songs = 0; QString cover; };
    QHash<QString, A> map;
    for (const QString &k : m_albumOrder) {
        const AlbumInfo &al = *m_albums.constFind(k);
        for (int i : al.tracks) {
            const Track &t = m_tracks[i];
            const QString name = t.artist.isEmpty() ? al.artist : t.artist;
            if (!query.isEmpty() && !name.contains(query, Qt::CaseInsensitive))
                continue;
            A &a = map[name.toLower()];
            a.name = name;
            a.albums.insert(k);
            a.songs++;
            if (a.cover.isEmpty())
                a.cover = al.cover;
        }
    }
    QList<A> list = map.values();
    std::sort(list.begin(), list.end(), [](const A &x, const A &y) { return x.name.localeAwareCompare(y.name) < 0; });
    QVariantList out;
    for (const A &a : list)
        out << QVariantMap{{QStringLiteral("name"), a.name}, {QStringLiteral("albums"), int(a.albums.size())}, {QStringLiteral("songs"), a.songs},
                           {QStringLiteral("cover"), a.cover.isEmpty() ? QString() : QUrl::fromLocalFile(a.cover).toString()}};
    return out;
}

QVariantMap MediaLibrary::artist(const QString &name) const
{
    QVariantList albumList;
    QList<int> tracks;
    for (const QString &k : m_albumOrder) {
        const AlbumInfo &al = *m_albums.constFind(k);
        bool hit = false;
        for (int i : al.tracks) {
            const Track &t = m_tracks[i];
            if ((t.artist.isEmpty() ? al.artist : t.artist).compare(name, Qt::CaseInsensitive) == 0) {
                tracks << i;
                hit = true;
            }
        }
        if (hit || al.artist.compare(name, Qt::CaseInsensitive) == 0)
            albumList << albumMap(al, false);
    }
    return {{QStringLiteral("name"), name}, {QStringLiteral("albums"), albumList}, {QStringLiteral("tracks"), trackList(tracks)}};
}

QVariantList MediaLibrary::songs(const QString &query, const QString &sort) const
{
    QList<int> list;
    for (int i = 0; i < m_tracks.size(); ++i)
        if (m_tracks[i].kind == Track::Music && matches(m_tracks[i], query))
            list << i;
    std::sort(list.begin(), list.end(), [this, &sort](int x, int y) {
        const Track &a = m_tracks[x], &b = m_tracks[y];
        if (sort == QLatin1String("added"))
            return a.added != b.added ? a.added > b.added : a.path < b.path;
        if (sort == QLatin1String("artist"))
            return a.artist.localeAwareCompare(b.artist) != 0 ? a.artist.localeAwareCompare(b.artist) < 0 : a.title.localeAwareCompare(b.title) < 0;
        return a.title.localeAwareCompare(b.title) < 0;
    });
    return trackList(list);
}

// ---------- playlists: .m3u8 files under the first music folder ----------

static QString playlistDir(const QStringList &music)
{
    return (music.isEmpty() ? QDir::homePath() + QStringLiteral("/Music") : music.first()) + QStringLiteral("/Playlists");
}

QVariantList MediaLibrary::playlists() const
{
    QVariantList out;
    out << QVariantMap{{QStringLiteral("name"), tr("Liked songs")}, {QStringLiteral("path"), QStringLiteral("liked:")}, {QStringLiteral("count"), int(m_liked.size())}};
    QStringList files;
    for (const QString &root : m_music) {
        QDirIterator it(root, {QStringLiteral("*.m3u"), QStringLiteral("*.m3u8")}, QDir::Files, QDirIterator::Subdirectories);
        int budget = 2000;
        while (it.hasNext() && --budget > 0)
            files << it.next();
    }
    files.removeDuplicates();
    for (const QString &f : files)
        out << QVariantMap{{QStringLiteral("name"), QFileInfo(f).completeBaseName()}, {QStringLiteral("path"), f}, {QStringLiteral("count"), int(playlist(f).size())}};
    return out;
}

QVariantList MediaLibrary::playlist(const QString &path) const
{
    if (path == QLatin1String("liked:"))
        return likedSongs();
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return {};
    const QDir base = QFileInfo(path).absoluteDir();
    QVariantList out;
    QTextStream in(&f);
    while (!in.atEnd()) {
        const QString line = in.readLine().trimmed();
        if (line.isEmpty() || line.startsWith(QLatin1Char('#')))
            continue;
        const QString file = QUrl(line).isLocalFile() ? QUrl(line).toLocalFile() : QDir::cleanPath(base.absoluteFilePath(line));
        if (const Track *t = find(file))
            out << toMap(*t);
    }
    return out;
}

static bool writePlaylist(const QString &path, const QStringList &files)
{
    QDir().mkpath(QFileInfo(path).absolutePath());
    QSaveFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;
    QTextStream out(&f);
    out << "#EXTM3U\n";
    const QDir base = QFileInfo(path).absoluteDir();
    for (const QString &file : files)
        out << base.relativeFilePath(file) << '\n';
    out.flush();
    return f.commit();
}

QString MediaLibrary::createPlaylist(const QString &name, const QStringList &paths)
{
    QString clean = name.trimmed();
    clean.replace(QLatin1Char('/'), QLatin1Char('-'));
    if (clean.isEmpty())
        return {};
    const QString path = playlistDir(m_music) + QLatin1Char('/') + clean + QStringLiteral(".m3u8");
    if (QFileInfo::exists(path) || !writePlaylist(path, paths))
        return {};
    ++m_likedRevision;
    emit likedChanged();
    return path;
}

bool MediaLibrary::addToPlaylist(const QString &path, const QStringList &paths)
{
    if (path == QLatin1String("liked:")) {
        for (const QString &p : paths)
            m_liked.insert(p);
        settings()->setValue(QStringLiteral("music/liked"), QStringList(m_liked.cbegin(), m_liked.cend()));
        ++m_likedRevision;
        emit likedChanged();
        return true;
    }
    QStringList files;
    for (const QVariant &v : playlist(path))
        files << v.toMap().value(QStringLiteral("path")).toString();
    files << paths;
    const bool ok = writePlaylist(path, files);
    ++m_likedRevision;
    emit likedChanged();
    return ok;
}

bool MediaLibrary::removeFromPlaylist(const QString &path, int index)
{
    QStringList files;
    for (const QVariant &v : playlist(path))
        files << v.toMap().value(QStringLiteral("path")).toString();
    if (index < 0 || index >= files.size())
        return false;
    if (path == QLatin1String("liked:")) {
        toggleLiked(files.at(index));
        return true;
    }
    files.removeAt(index);
    const bool ok = writePlaylist(path, files);
    ++m_likedRevision;
    emit likedChanged();
    return ok;
}

bool MediaLibrary::deletePlaylist(const QString &path)
{
    if (!path.endsWith(QLatin1String(".m3u8")) && !path.endsWith(QLatin1String(".m3u")))
        return false;
    QFile f(path);
    const bool ok = f.moveToTrash();
    ++m_likedRevision;
    emit likedChanged();
    return ok;
}

void MediaLibrary::toggleLiked(const QString &path)
{
    if (!m_liked.remove(path))
        m_liked.insert(path);
    settings()->setValue(QStringLiteral("music/liked"), QStringList(m_liked.cbegin(), m_liked.cend()));
    ++m_likedRevision;
    emit likedChanged();
}

QVariantList MediaLibrary::likedSongs() const
{
    QList<int> list;
    for (const QString &p : m_liked) {
        const int i = m_index.value(p, -1);
        if (i >= 0)
            list << i;
    }
    std::sort(list.begin(), list.end(), [this](int x, int y) { return m_tracks[x].title.localeAwareCompare(m_tracks[y].title) < 0; });
    return trackList(list);
}

// ---------- books ----------

QVariantMap MediaLibrary::bookMap(const BookInfo &b, bool withTracks) const
{
    QSettings *s = settings();
    const QString k = QStringLiteral("books/") + settingsKey(b.key);
    const int chapter = s->value(k + QStringLiteral("/chapter"), 0).toInt();
    const qint64 position = s->value(k + QStringLiteral("/position"), 0).toLongLong();
    qint64 done = 0;
    for (int i = 0; i < std::min<qsizetype>(chapter, b.tracks.size()); ++i)
        done += m_tracks[b.tracks[i]].duration;
    done += position;
    QVariantMap m{{QStringLiteral("key"), b.key},
                  {QStringLiteral("title"), b.title},
                  {QStringLiteral("author"), b.author.isEmpty() ? tr("Unknown author") : b.author},
                  {QStringLiteral("cover"), b.cover.isEmpty() ? QString() : QUrl::fromLocalFile(b.cover).toString()},
                  {QStringLiteral("chapters"), int(b.tracks.size())},
                  {QStringLiteral("duration"), b.duration},
                  {QStringLiteral("length"), formatDuration(b.duration)},
                  {QStringLiteral("chapter"), chapter},
                  {QStringLiteral("position"), position},
                  {QStringLiteral("progress"), b.duration > 0 ? double(done) / b.duration : 0.0},
                  {QStringLiteral("left"), formatDuration(std::max<qint64>(0, b.duration - done))},
                  {QStringLiteral("started"), done > 0},
                  {QStringLiteral("finished"), b.duration > 0 && done >= b.duration - 30000}};
    if (withTracks)
        m.insert(QStringLiteral("tracks"), trackList(b.tracks));
    return m;
}

QVariantList MediaLibrary::books() const
{
    QVariantList out;
    for (const QString &k : m_bookOrder)
        out << bookMap(*m_bookInfo.constFind(k), false);
    std::stable_sort(out.begin(), out.end(), [](const QVariant &a, const QVariant &b) {
        return a.toMap().value(QStringLiteral("title")).toString().localeAwareCompare(b.toMap().value(QStringLiteral("title")).toString()) < 0;
    });
    return out;
}

QVariantMap MediaLibrary::book(const QString &key) const
{
    const auto it = m_bookInfo.constFind(key);
    return it == m_bookInfo.constEnd() ? QVariantMap() : bookMap(*it, true);
}

void MediaLibrary::setBookProgress(const QString &key, int chapter, qint64 position)
{
    const QString k = QStringLiteral("books/") + settingsKey(key);
    settings()->setValue(k + QStringLiteral("/chapter"), chapter);
    settings()->setValue(k + QStringLiteral("/position"), position);
    settings()->setValue(k + QStringLiteral("/played"), QDateTime::currentSecsSinceEpoch());
    ++m_progressRevision;
    emit progressChanged();
}

// ---------- videos ----------

QVariantList MediaLibrary::videos(const QString &query) const
{
    QList<int> list;
    for (int i = 0; i < m_tracks.size(); ++i)
        if (m_tracks[i].kind == Track::Video && (query.isEmpty() || m_tracks[i].title.contains(query, Qt::CaseInsensitive) || m_tracks[i].path.contains(query, Qt::CaseInsensitive)))
            list << i;
    std::sort(list.begin(), list.end(), [this](int x, int y) { return m_tracks[x].mtime > m_tracks[y].mtime; });
    QSettings *s = settings();
    QVariantList out;
    for (int i : list) {
        const Track &t = m_tracks[i];
        QVariantMap m = toMap(t);
        m.insert(QStringLiteral("folder"), t.folder().section(QLatin1Char('/'), -1));
        m.insert(QStringLiteral("thumb"), t.cover.isEmpty() ? QString() : QUrl::fromLocalFile(t.cover).toString());
        const qint64 pos = s->value(QStringLiteral("videos/") + settingsKey(t.path), 0).toLongLong();
        m.insert(QStringLiteral("position"), pos);
        m.insert(QStringLiteral("progress"), t.duration > 0 ? double(pos) / t.duration : 0.0);
        out << m;
    }
    return out;
}

void MediaLibrary::setVideoPosition(const QString &path, qint64 position)
{
    const Track *t = find(path);
    // The end of a video counts as watched, so it does not offer to resume.
    if (t && t->duration > 0 && position > t->duration - 20000)
        position = 0;
    settings()->setValue(QStringLiteral("videos/") + settingsKey(path), position);
    ++m_progressRevision;
    emit progressChanged();
}

// ---------- search and history ----------

QVariantMap MediaLibrary::search(const QString &query) const
{
    if (query.trimmed().isEmpty())
        return {};
    QVariantList bookList;
    for (const QString &k : m_bookOrder) {
        const BookInfo &b = *m_bookInfo.constFind(k);
        if (b.title.contains(query, Qt::CaseInsensitive) || b.author.contains(query, Qt::CaseInsensitive))
            bookList << bookMap(b, false);
    }
    return {{QStringLiteral("albums"), albums(query).mid(0, 12)},
            {QStringLiteral("artists"), artists(query).mid(0, 12)},
            {QStringLiteral("songs"), songs(query).mid(0, 40)},
            {QStringLiteral("videos"), videos(query).mid(0, 12)},
            {QStringLiteral("books"), bookList}};
}

void MediaLibrary::noteAlbumPlayed(const QString &key)
{
    if (key.isEmpty())
        return;
    m_recent.removeAll(key);
    m_recent.prepend(key);
    while (m_recent.size() > 24)
        m_recent.removeLast();
    settings()->setValue(QStringLiteral("music/recent"), m_recent);
    emit historyChanged();
}
