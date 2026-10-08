#include "format.h"

#include "gobj.h"

#include <QCoreApplication>
#include <QHash>
#include <QMutex>
#include <QMutexLocker>
#include <cmath>

namespace Fmt {

QString humanSize(qint64 bytes, const QLocale &loc)
{
    if (bytes < 0)
        return {};
    if (bytes < 1000)
        return QStringLiteral("%1 B").arg(bytes);
    static const char *units[] = {"KB", "MB", "GB", "TB", "PB"};
    double v = double(bytes);
    int u = -1;
    while (v >= 1000 && u < 4) {
        v /= 1000;
        ++u;
    }
    // One decimal below 10, none above; "2.0" shows as "2".
    const double rounded = v < 10 ? std::round(v * 10) / 10 : std::round(v);
    const QString num = rounded == std::floor(rounded) ? loc.toString(qint64(rounded)) : loc.toString(rounded, 'f', 1);
    return num + QLatin1Char(' ') + QLatin1String(units[u]);
}

QString humanDate(const QDateTime &when, const QDateTime &now, const QLocale &loc)
{
    if (!when.isValid())
        return {};
    const QDate d = when.date();
    const QDate today = now.date();
    const qint64 days = d.daysTo(today);
    if (days == 0)
        return QCoreApplication::translate("Fmt", "Today %1").arg(loc.toString(when.time(), QStringLiteral("HH:mm")));
    if (days == 1)
        return QCoreApplication::translate("Fmt", "Yesterday");
    if (days > 1 && days < 6)
        return loc.toString(when, QStringLiteral("ddd HH:mm"));
    if (d.year() == today.year())
        return loc.toString(d, QStringLiteral("d MMM"));
    return loc.toString(d, QStringLiteral("d MMM yyyy"));
}

QString itemCount(int n, const QLocale &loc)
{
    return n == 1 ? QCoreApplication::translate("Fmt", "1 item") : QCoreApplication::translate("Fmt", "%1 items").arg(loc.toString(n));
}

QString humanEta(qint64 s)
{
    if (s < 1)
        s = 1;
    if (s < 60)
        return QCoreApplication::translate("Fmt", "%1 s left").arg(s);
    if (s < 3600)
        return QCoreApplication::translate("Fmt", "%1 min left").arg((s + 30) / 60);
    const qint64 h = s / 3600;
    const qint64 m = (s % 3600 + 30) / 60;
    return m ? QCoreApplication::translate("Fmt", "%1 h %2 min left").arg(h).arg(m) : QCoreApplication::translate("Fmt", "%1 h left").arg(h);
}

static bool inList(const QString &ct, std::initializer_list<const char *> list)
{
    for (const char *s : list)
        if (ct == QLatin1String(s))
            return true;
    return false;
}

Kind kindFor(const QString &ct, bool isDir)
{
    if (isDir || ct == QLatin1String("inode/directory"))
        return Kind::Folder;
    if (ct.startsWith(QLatin1String("image/")))
        return Kind::Image;
    if (ct.startsWith(QLatin1String("video/")))
        return Kind::Video;
    if (ct.startsWith(QLatin1String("audio/")))
        return Kind::Audio;
    if (ct == QLatin1String("application/pdf"))
        return Kind::Pdf;
    if (inList(ct, {"application/zip", "application/gzip", "application/x-gzip", "application/x-tar", "application/x-7z-compressed",
                    "application/x-xz", "application/x-bzip2", "application/x-bzip", "application/zstd", "application/x-zstd-compressed-tar",
                    "application/vnd.rar", "application/x-rar", "application/x-compressed-tar", "application/x-xz-compressed-tar",
                    "application/x-bzip2-compressed-tar", "application/x-cpio", "application/x-archive", "application/x-lzma",
                    "application/x-lz4", "application/x-zip-compressed", "application/vnd.debian.binary-package", "application/x-rpm",
                    "application/x-cd-image", "application/x-iso9660-image", "application/java-archive", "application/x-lzip",
                    "application/x-tarz", "application/x-lrzip", "application/x-zstd", "application/x-cbz", "application/x-cbr"}))
        return Kind::Archive;
    if (inList(ct, {"application/x-executable", "application/x-pie-executable", "application/vnd.appimage", "application/x-iso9660-appimage",
                    "application/x-desktop", "application/vnd.flatpak", "application/x-msdownload", "application/x-sharedlib",
                    "application/x-ms-dos-executable", "application/vnd.microsoft.portable-executable", "application/x-msi",
                    "application/vnd.android.package-archive"}))
        return Kind::App;
    if (ct.startsWith(QLatin1String("text/"))) {
        if (inList(ct, {"text/plain", "text/markdown", "text/csv", "text/rtf", "text/x-log", "text/x-readme", "text/x-copying", "text/x-authors"}))
            return Kind::Text;
        return Kind::Code;
    }
    if (inList(ct, {"application/json", "application/xml", "application/javascript", "application/x-shellscript", "application/x-php",
                    "application/x-perl", "application/x-ruby", "application/x-python-code", "application/x-yaml", "application/toml",
                    "application/x-awk", "application/sql", "application/x-sh", "application/x-csh", "application/x-java",
                    "application/x-wine-extension-ini", "application/ld+json", "application/x-subrip"}))
        return Kind::Code;
    if (ct.startsWith(QLatin1String("application/vnd.oasis.opendocument")) || ct.startsWith(QLatin1String("application/msword"))
        || ct.startsWith(QLatin1String("application/vnd.openxmlformats")) || ct == QLatin1String("application/epub+zip"))
        return Kind::Text;
    return Kind::Other;
}

QString kindString(Kind k)
{
    switch (k) {
    case Kind::Folder: return QStringLiteral("folder");
    case Kind::Image: return QStringLiteral("image");
    case Kind::Video: return QStringLiteral("video");
    case Kind::Audio: return QStringLiteral("audio");
    case Kind::Pdf: return QStringLiteral("pdf");
    case Kind::Archive: return QStringLiteral("archive");
    case Kind::Text: return QStringLiteral("text");
    case Kind::Code: return QStringLiteral("code");
    case Kind::App: return QStringLiteral("app");
    case Kind::Other: break;
    }
    return QStringLiteral("other");
}

Kind kindFromString(const QString &s, bool *ok)
{
    static const Kind all[] = {Kind::Folder, Kind::Image, Kind::Video, Kind::Audio, Kind::Pdf, Kind::Archive, Kind::Text, Kind::Code, Kind::App, Kind::Other};
    for (Kind k : all) {
        if (kindString(k) == s) {
            if (ok) *ok = true;
            return k;
        }
    }
    if (ok) *ok = false;
    return Kind::Other;
}

namespace {
QMutex s_cacheLock;
QHash<QString, QString> s_icons;
QHash<QString, QString> s_descriptions;
}

QString iconNameFor(const QString &ct, bool isDir)
{
    const QString key = isDir ? QStringLiteral("inode/directory") : ct;
    QMutexLocker lock(&s_cacheLock);
    auto it = s_icons.constFind(key);
    if (it != s_icons.constEnd())
        return *it;
    QString name = QStringLiteral("application-x-generic");
    if (!key.isEmpty()) {
        auto icon = Gio::Obj<GIcon>::adopt(g_content_type_get_icon(key.toUtf8().constData()));
        if (icon && G_IS_THEMED_ICON(icon.get())) {
            const gchar *const *names = g_themed_icon_get_names(G_THEMED_ICON(icon.get()));
            if (names && names[0])
                name = Gio::str(names[0]);
        }
    }
    s_icons.insert(key, name);
    return name;
}

QString kindDescription(const QString &ct, bool isDir)
{
    const QString key = isDir ? QStringLiteral("inode/directory") : ct;
    QMutexLocker lock(&s_cacheLock);
    auto it = s_descriptions.constFind(key);
    if (it != s_descriptions.constEnd())
        return *it;
    QString d = key.isEmpty() ? QCoreApplication::translate("Fmt", "Unknown") : Gio::take(g_content_type_get_description(key.toUtf8().constData()));
    s_descriptions.insert(key, d);
    return d;
}

}
