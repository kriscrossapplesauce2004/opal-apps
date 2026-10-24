#include "photoactions.h"
#include "imageio.h"
#include "library.h"

#include <QClipboard>
#include <QDate>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QMimeData>
#include <QProcess>
#include <QStandardPaths>
#include <QUrl>
#include <QtConcurrent>

namespace {

QList<QUrl> urlsOf(const QStringList &paths)
{
    QList<QUrl> out;
    for (const QString &p : paths)
        out << QUrl::fromLocalFile(p);
    return out;
}

QString plural(int n, const char *one, const char *many)
{
    return n == 1 ? QString::fromLatin1(one) : QString::number(n) + QLatin1Char(' ') + QString::fromLatin1(many);
}

// Opal marks its running session with this file; see the opal command.
bool opalRunning()
{
    const QString runtime = QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation);
    return !runtime.isEmpty() && QFileInfo::exists(runtime + QStringLiteral("/opal-active"));
}

QString opalConfig()
{
    return QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + QStringLiteral("/quickshell/opal");
}

}

PhotoActions::PhotoActions(QObject *parent)
    : QObject(parent)
{
}

void PhotoActions::begin()
{
    if (m_busy++ == 0)
        emit busyChanged();
}

void PhotoActions::end()
{
    if (--m_busy == 0)
        emit busyChanged();
}

QString PhotoActions::opalCommand()
{
    if (!QFileInfo::exists(opalConfig() + QStringLiteral("/shell.qml")))
        return {};
    return QStandardPaths::findExecutable(QStringLiteral("quickshell"));
}

QString PhotoActions::wallpaperTarget() const
{
    if (!opalCommand().isEmpty() && opalRunning())
        return QStringLiteral("Opal");
    return QDBusConnection::sessionBus().isConnected() ? QStringLiteral("your desktop") : QString();
}

void PhotoActions::setWallpaper(const QString &path)
{
    if (!QFileInfo(path).isFile()) {
        emit failed(tr("That photo is gone"));
        return;
    }
    const QString quickshell = opalCommand();
    if (quickshell.isEmpty() || !opalRunning()) {
        portalWallpaper(path);
        return;
    }
    auto *proc = new QProcess(this);
    begin();
    connect(proc, &QProcess::finished, this, [this, proc, path](int code, QProcess::ExitStatus status) {
        proc->deleteLater();
        end();
        if (status == QProcess::NormalExit && code == 0)
            emit done(tr("Wallpaper set"));
        else
            portalWallpaper(path); // Opal did not answer; the desktop portal is the general way.
    });
    proc->start(quickshell, {QStringLiteral("ipc"), QStringLiteral("-p"), opalConfig(), QStringLiteral("call"), QStringLiteral("opal"), QStringLiteral("wallpaper"), path});
}

void PhotoActions::portalWallpaper(const QString &path)
{
    QDBusMessage call = QDBusMessage::createMethodCall(QStringLiteral("org.freedesktop.portal.Desktop"), QStringLiteral("/org/freedesktop/portal/desktop"),
                                                       QStringLiteral("org.freedesktop.portal.Wallpaper"), QStringLiteral("SetWallpaperURI"));
    call << QString() << QUrl::fromLocalFile(path).toString() << QVariantMap{{QStringLiteral("show-preview"), true}, {QStringLiteral("set-on"), QStringLiteral("both")}};
    begin();
    auto *watch = new QDBusPendingCallWatcher(QDBusConnection::sessionBus().asyncCall(call), this);
    connect(watch, &QDBusPendingCallWatcher::finished, this, [this, watch] {
        watch->deleteLater();
        end();
        if (watch->isError())
            emit failed(tr("Your desktop has no way to set a wallpaper from apps"));
        else
            emit done(tr("Wallpaper sent to your desktop"));
    });
}

void PhotoActions::copy(const QStringList &paths)
{
    if (paths.isEmpty())
        return;
    if (paths.size() > 1) {
        auto *mime = new QMimeData;
        mime->setUrls(urlsOf(paths));
        QByteArray gnome = "copy";
        for (const QUrl &u : urlsOf(paths))
            gnome += '\n' + u.toEncoded();
        mime->setData(QStringLiteral("x-special/gnome-copied-files"), gnome);
        QGuiApplication::clipboard()->setMimeData(mime);
        emit done(tr("Copied %1").arg(plural(int(paths.size()), "photo", "photos")));
        return;
    }
    // One photo: the picture itself, so it pastes into chats and editors, plus the file for file managers.
    const QString path = paths.first();
    begin();
    QtConcurrent::run([path] { return ImageIO::loadOriented(path); }).then(this, [this, path](const QImage &image) {
        end();
        auto *mime = new QMimeData;
        if (!image.isNull())
            mime->setImageData(image);
        mime->setUrls({QUrl::fromLocalFile(path)});
        QGuiApplication::clipboard()->setMimeData(mime);
        emit done(image.isNull() ? tr("Copied the file") : tr("Copied"));
    });
}

void PhotoActions::copyPath(const QStringList &paths)
{
    QGuiApplication::clipboard()->setText(paths.join(QLatin1Char('\n')));
    emit done(paths.size() == 1 ? tr("Copied the path") : tr("Copied %1 paths").arg(paths.size()));
}

void PhotoActions::showInFiles(const QString &path)
{
    QDBusMessage call = QDBusMessage::createMethodCall(QStringLiteral("org.freedesktop.FileManager1"), QStringLiteral("/org/freedesktop/FileManager1"),
                                                       QStringLiteral("org.freedesktop.FileManager1"), QStringLiteral("ShowItems"));
    call << QStringList{QUrl::fromLocalFile(path).toString()} << QString();
    auto *watch = new QDBusPendingCallWatcher(QDBusConnection::sessionBus().asyncCall(call), this);
    connect(watch, &QDBusPendingCallWatcher::finished, this, [watch, path] {
        watch->deleteLater();
        if (watch->isError())
            QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(path).absolutePath()));
    });
}

void PhotoActions::openWith(const QString &path)
{
    if (!QDesktopServices::openUrl(QUrl::fromLocalFile(path)))
        emit failed(tr("No app opens this kind of file"));
}

void PhotoActions::trash(const QStringList &paths)
{
    if (paths.isEmpty())
        return;
    begin();
    QtConcurrent::run([paths] {
        QStringList gone;
        for (const QString &p : paths) {
            QFile f(p);
            if (f.moveToTrash())
                gone << p;
        }
        return gone;
    }).then(this, [this, paths](const QStringList &gone) {
        end();
        if (!gone.isEmpty()) {
            emit trashed(gone);
            PhotoLibrary::instance()->rescan();
        }
        if (gone.size() == paths.size())
            emit done(tr("Moved %1 to the trash").arg(plural(int(gone.size()), "photo", "photos")));
        else
            emit failed(tr("%1 of %2 could not be moved to the trash").arg(paths.size() - gone.size()).arg(paths.size()));
    });
}

void PhotoActions::rotate(const QStringList &paths, int quarterTurns)
{
    if (paths.isEmpty() || quarterTurns % 4 == 0)
        return;
    begin();
    QtConcurrent::run([paths, quarterTurns] {
        int failed = 0;
        for (const QString &p : paths) {
            if (!ImageIO::rotate(p, quarterTurns))
                failed++;
        }
        return failed;
    }).then(this, [this](int failedCount) {
        end();
        PhotoLibrary::instance()->rescan();
        if (failedCount)
            emit failed(tr("%1 could not be rotated").arg(plural(failedCount, "One photo", "photos")));
    });
}

QString PhotoActions::importFolder(const QString &pictures)
{
    return pictures + QStringLiteral("/Imported/") + QDate::currentDate().toString(Qt::ISODate);
}

void PhotoActions::importPhotos(const QStringList &paths)
{
    if (paths.isEmpty())
        return;
    const QString folder = importFolder(PhotoLibrary::instance()->picturesPath());
    begin();
    QtConcurrent::run([paths, folder] {
        QDir().mkpath(folder);
        int copied = 0;
        for (const QString &p : paths) {
            const QFileInfo src(p);
            QString dest = folder + QLatin1Char('/') + src.fileName();
            // The same file imported twice is skipped; a different file with the same name gets a number.
            if (QFileInfo(dest).exists() && QFileInfo(dest).size() == src.size())
                continue;
            for (int n = 2; QFileInfo::exists(dest); n++)
                dest = folder + QLatin1Char('/') + src.completeBaseName() + QStringLiteral("-%1.").arg(n) + src.suffix();
            if (QFile::copy(p, dest)) {
                QFile(dest).setFileTime(src.lastModified(), QFileDevice::FileModificationTime);
                copied++;
            }
        }
        return copied;
    }).then(this, [this, folder](int copied) {
        end();
        PhotoLibrary::instance()->rescan();
        emit imported(folder, copied);
        emit done(copied ? tr("Imported %1").arg(plural(copied, "photo", "photos")) : tr("Already imported"));
    });
}
