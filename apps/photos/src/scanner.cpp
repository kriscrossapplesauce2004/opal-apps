#include "scanner.h"
#include "exif.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImageReader>
#include <QProcess>
#include <QRegularExpression>
#include <QSet>
#include <QStandardPaths>

#include <cerrno>
#include <sys/xattr.h>

namespace {

const QSet<QString> &imageSuffixes()
{
    // Only the formats this Qt build can decode; HEIC, AVIF and JXL appear when kimageformats is installed.
    static const QSet<QString> set = [] {
        QSet<QString> s;
        const QSet<QString> wanted{QStringLiteral("jpg"), QStringLiteral("jpeg"), QStringLiteral("png"), QStringLiteral("webp"), QStringLiteral("gif"),
                                   QStringLiteral("bmp"), QStringLiteral("tif"), QStringLiteral("tiff"), QStringLiteral("heic"), QStringLiteral("heif"),
                                   QStringLiteral("avif"), QStringLiteral("jxl")};
        for (const QByteArray &f : QImageReader::supportedImageFormats()) {
            const QString n = QString::fromLatin1(f).toLower();
            if (wanted.contains(n))
                s.insert(n);
        }
        if (s.contains(QStringLiteral("jpeg")))
            s.insert(QStringLiteral("jpg"));
        if (s.contains(QStringLiteral("tiff")))
            s.insert(QStringLiteral("tif"));
        if (s.contains(QStringLiteral("heif")))
            s.insert(QStringLiteral("heic"));
        return s;
    }();
    return set;
}

const QSet<QString> &softwareSuffixes()
{
    static const QSet<QString> s{QStringLiteral("exe"), QStringLiteral("dll"), QStringLiteral("so"), QStringLiteral("x86_64"), QStringLiteral("pck"),
                                 QStringLiteral("apk"), QStringLiteral("jar"), QStringLiteral("appimage"), QStringLiteral("csproj"), QStringLiteral("sln"),
                                 QStringLiteral("msi"), QStringLiteral("dylib"), QStringLiteral("uproject"), QStringLiteral("unitypackage")};
    return s;
}

const QSet<QString> &softwareNames()
{
    static const QSet<QString> s{QStringLiteral(".nomedia"), QStringLiteral(".git"), QStringLiteral("node_modules"), QStringLiteral("CMakeLists.txt"),
                                 QStringLiteral("package.json"), QStringLiteral("Cargo.toml"), QStringLiteral("meson.build"), QStringLiteral("pyproject.toml"),
                                 QStringLiteral("Makefile"), QStringLiteral("data.win"), QStringLiteral("steam_api64.dll"), QStringLiteral("__pycache__")};
    return s;
}

bool markerName(const QString &name)
{
    if (softwareNames().contains(name))
        return true;
    const int dot = name.lastIndexOf(QLatin1Char('.'));
    return dot > 0 && softwareSuffixes().contains(name.mid(dot + 1).toLower());
}

void probeVideo(const QString &path, Photo &out)
{
    out.width = 16;
    out.height = 9;
    static const QString ffprobe = QStandardPaths::findExecutable(QStringLiteral("ffprobe"));
    if (ffprobe.isEmpty())
        return;
    QProcess p;
    p.start(ffprobe, {QStringLiteral("-v"), QStringLiteral("error"), QStringLiteral("-select_streams"), QStringLiteral("v:0"), QStringLiteral("-show_entries"),
                      QStringLiteral("stream=width,height:stream_side_data=rotation:stream_tags=rotate:format_tags=creation_time"), QStringLiteral("-of"),
                      QStringLiteral("default=nw=1"), path});
    if (!p.waitForFinished(5000)) {
        p.kill();
        p.waitForFinished(500);
        return;
    }
    int w = 0, h = 0, rotation = 0;
    for (const QString &line : QString::fromUtf8(p.readAllStandardOutput()).split(QLatin1Char('\n'))) {
        const QString key = line.section(QLatin1Char('='), 0, 0).trimmed(), value = line.section(QLatin1Char('='), 1).trimmed();
        if (key == QLatin1String("width"))
            w = value.toInt();
        else if (key == QLatin1String("height"))
            h = value.toInt();
        else if (key == QLatin1String("rotation") || key == QLatin1String("TAG:rotate"))
            rotation = value.toInt();
        else if (key == QLatin1String("TAG:creation_time")) {
            // Cameras write creation_time in UTC; the timeline shows local time.
            const QDateTime d = QDateTime::fromString(value, Qt::ISODateWithMs);
            if (d.isValid() && d.date().year() > 1990) {
                out.taken = d.toLocalTime().toMSecsSinceEpoch();
                out.dateSource = Photo::FromExif;
            }
        }
    }
    if (w > 0 && h > 0) {
        const bool turn = qAbs(rotation) % 180 == 90;
        out.width = turn ? h : w;
        out.height = turn ? w : h;
    }
}

}

namespace Scanner {

bool isImageName(const QString &name)
{
    const int dot = name.lastIndexOf(QLatin1Char('.'));
    return dot > 0 && imageSuffixes().contains(name.mid(dot + 1).toLower());
}

bool isVideoName(const QString &name)
{
    static const QSet<QString> s{QStringLiteral("mp4"), QStringLiteral("mov"), QStringLiteral("webm"), QStringLiteral("mkv"), QStringLiteral("m4v"),
                                 QStringLiteral("3gp"), QStringLiteral("avi")};
    const int dot = name.lastIndexOf(QLatin1Char('.'));
    return dot > 0 && s.contains(name.mid(dot + 1).toLower());
}

bool isScreenshot(const QString &path)
{
    static const QRegularExpression name(QStringLiteral("^(screenshot|screen shot|shot-|scrot|grim|spectacle|flameshot|snapshot_)|_grim\\."),
                                         QRegularExpression::CaseInsensitiveOption);
    const QString file = path.section(QLatin1Char('/'), -1);
    return path.section(QLatin1Char('/'), 0, -2).contains(QLatin1String("screenshot"), Qt::CaseInsensitive) || name.match(file).hasMatch();
}

bool hasSoftwareMarker(const QString &folder)
{
    const QDir dir(folder);
    if (dir.dirName().endsWith(QLatin1String(".x86_64")) || dir.dirName().endsWith(QLatin1String(".app")))
        return true;
    const QStringList names = dir.entryList(QDir::AllEntries | QDir::Hidden | QDir::System | QDir::NoDotAndDotDot);
    for (const QString &n : names)
        if (markerName(n))
            return true;
    return false;
}

bool looksLikeSoftware(const QString &folder, int depth)
{
    if (hasSoftwareMarker(folder))
        return true;
    if (depth <= 1)
        return false;
    // A bounded look: game folders keep their binaries a level or two down, next to the texture folders.
    const QStringList subs = QDir(folder).entryList(QDir::Dirs | QDir::NoDotAndDotDot | QDir::NoSymLinks);
    int budget = 40;
    for (const QString &s : subs) {
        if (--budget < 0)
            break;
        if (looksLikeSoftware(folder + QLatin1Char('/') + s, depth - 1))
            return true;
    }
    return false;
}

QString readCaption(const QString &path)
{
    const QByteArray p = QFile::encodeName(path);
    char buf[4096];
    const ssize_t n = getxattr(p.constData(), "user.xdg.comment", buf, sizeof buf);
    return n > 0 ? QString::fromUtf8(buf, int(n)) : QString();
}

bool writeCaption(const QString &path, const QString &caption)
{
    const QByteArray p = QFile::encodeName(path), v = caption.toUtf8();
    if (v.isEmpty())
        return removexattr(p.constData(), "user.xdg.comment") == 0 || errno == ENODATA;
    return setxattr(p.constData(), "user.xdg.comment", v.constData(), size_t(v.size()), 0) == 0;
}

bool probe(const QString &path, Photo &out)
{
    const QFileInfo fi(path);
    out.path = path;
    out.folder = fi.absolutePath();
    out.mtime = fi.lastModified().toSecsSinceEpoch();
    out.size = fi.size();
    out.screenshot = isScreenshot(path);
    out.taken = 0;
    out.camera.clear();
    out.video = isVideoName(fi.fileName());
    if (out.video) {
        probeVideo(path, out);
    } else {
        QImageReader reader(path);
        const QSize size = reader.size();
        if (!size.isValid() || size.isEmpty())
            return false;
        const ExifData exif = Exif::read(path);
        const bool turn = exif.orientation >= 5;
        out.width = turn ? size.height() : size.width();
        out.height = turn ? size.width() : size.height();
        out.camera = exif.camera();
        out.caption = readCaption(path);
        if (exif.taken.isValid()) {
            out.taken = exif.taken.toMSecsSinceEpoch();
            out.dateSource = Photo::FromExif;
        }
    }
    if (out.taken == 0) {
        const QDateTime fromName = Exif::dateFromName(fi.fileName());
        if (fromName.isValid() && fromName.date().year() <= QDate::currentDate().year() + 1) {
            out.taken = fromName.toMSecsSinceEpoch();
            out.dateSource = Photo::FromName;
        } else {
            out.taken = fi.lastModified().toMSecsSinceEpoch();
            out.dateSource = Photo::FromModified;
        }
    }
    return true;
}

Result scan(const Options &options, const QHash<QString, Photo> &cache, const std::atomic_bool *cancel, const std::function<void(int)> &progress)
{
    Result result;
    QSet<QString> seenFolders, seenFiles;
    QSet<QString> excluded;
    for (const QString &e : options.excluded)
        excluded.insert(QDir::cleanPath(e));
    struct Item { QString path; bool topLevel; };
    QList<Item> stack;
    QSet<QString> roots;
    for (const QString &r : options.roots) {
        const QString root = QDir::cleanPath(r);
        roots.insert(root);
        if (QFileInfo(root).isDir())
            stack.append({root, false});
    }
    int found = 0;
    while (!stack.isEmpty()) {
        if (cancel && cancel->load())
            return {};
        const Item item = stack.takeLast();
        const QString canonical = QFileInfo(item.path).canonicalFilePath();
        if (canonical.isEmpty() || seenFolders.contains(canonical) || excluded.contains(item.path))
            continue;
        seenFolders.insert(canonical);
        // A root is always walked: the user chose it, even when it also holds a program or an archive.
        if (options.skipSoftware && !roots.contains(item.path) && (item.topLevel ? looksLikeSoftware(item.path) : hasSoftwareMarker(item.path))) {
            result.skipped.append(item.path);
            continue;
        }
        result.folders.append(item.path);
        const QDir dir(item.path);
        const QFileInfoList entries = dir.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
        // The direct children of a root get the deeper software check.
        const bool isRoot = roots.contains(item.path);
        for (const QFileInfo &e : entries) {
            if (e.isDir()) {
                if (!e.isSymLink())
                    stack.append({e.absoluteFilePath(), isRoot});
                continue;
            }
            const QString name = e.fileName();
            const bool video = isVideoName(name);
            if (!(isImageName(name) || (options.videos && video)))
                continue;
            const QString path = e.absoluteFilePath();
            if (seenFiles.contains(path))
                continue;
            seenFiles.insert(path);
            const auto hit = cache.constFind(path);
            if (hit != cache.constEnd() && hit->mtime == e.lastModified().toSecsSinceEpoch() && hit->size == e.size()) {
                result.photos.append(*hit);
            } else {
                Photo p;
                if (hit != cache.constEnd()) {
                    // Keep what the user wrote; OCR runs again because the pixels changed.
                    p.caption = hit->caption;
                }
                if (!probe(path, p))
                    continue;
                result.photos.append(p);
            }
            if (progress && ++found % 200 == 0)
                progress(found);
        }
    }
    return result;
}

}
