#include "fileinfo.h"

#include "format.h"

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusObjectPath>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QDBusUnixFileDescriptor>
#include <QFile>
#include <QImageReader>
#include <QPointer>
#include <QUrl>
#include <QtConcurrent/QtConcurrentRun>
#include <fcntl.h>
#include <gio/gdesktopappinfo.h>
#include <unistd.h>

namespace {
const char *const kAttrs = "standard::*,owner::*,unix::mode,time::*,access::*";

QDateTime fromSecs(GFileInfo *info, const char *attr)
{
    if (!g_file_info_has_attribute(info, attr))
        return {};
    return QDateTime::fromSecsSinceEpoch(qint64(g_file_info_get_attribute_uint64(info, attr)));
}
}

FileInfo::FileInfo(QObject *parent)
    : QObject(parent)
{
    m_cancel = Gio::Cancellable::adopt(g_cancellable_new());
}

FileInfo::~FileInfo()
{
    g_cancellable_cancel(m_cancel.get());
}

bool FileInfo::inSandbox()
{
    static const bool in = QFile::exists(portalPathForTests());
    return in;
}

void FileInfo::clear()
{
    m_ready = false;
    m_error.clear();
    m_name = m_displayName = m_path = m_contentType = m_symlinkTarget = m_owner = m_group.clear();
    m_modified = m_accessed = m_created = QDateTime();
    m_size = 0;
    m_mode = 0;
    m_imageSize = QSize();
    m_openWith.clear();
    m_isDir = m_isSymlink = m_readable = m_writable = m_executable = m_canSetMode = false;
}

void FileInfo::setUri(const QString &uri)
{
    if (uri == m_uri)
        return;
    m_uri = uri;
    emit uriChanged();
    reload();
}

void FileInfo::reload()
{
    g_cancellable_cancel(m_cancel.get());
    m_cancel = Gio::Cancellable::adopt(g_cancellable_new());
    const int gen = ++m_gen;
    clear();
    emit loadedChanged();
    emit imageSizeChanged();
    auto file = Gio::fileForString(m_uri);
    if (!file)
        return;
    QPointer<FileInfo> self(this);
    auto [cb, ud] = Gio::thunk([self, gen](GObject *src, GAsyncResult *res) {
        Gio::Error err;
        auto info = Gio::Info::adopt(g_file_query_info_finish(G_FILE(src), res, err.out()));
        if (!self || self->m_gen != gen || err.cancelled())
            return;
        if (!info) {
            self->m_error = err.message();
            emit self->loadedChanged();
            return;
        }
        GFileInfo *i = info.get();
        self->m_name = Gio::str(g_file_info_get_name(i));
        self->m_displayName = Gio::str(g_file_info_get_display_name(i));
        self->m_path = Gio::pathOf(G_FILE(src));
        const char *ct = g_file_info_get_content_type(i);
        self->m_isDir = g_file_info_get_file_type(i) == G_FILE_TYPE_DIRECTORY;
        self->m_contentType = self->m_isDir ? QStringLiteral("inode/directory") : Gio::str(ct);
        self->m_isSymlink = g_file_info_get_is_symlink(i);
        self->m_symlinkTarget = Gio::str(g_file_info_get_symlink_target(i));
        self->m_size = qint64(g_file_info_get_size(i));
        self->m_modified = fromSecs(i, "time::modified");
        self->m_accessed = fromSecs(i, "time::access");
        self->m_created = fromSecs(i, "time::created");
        self->m_mode = g_file_info_has_attribute(i, "unix::mode") ? g_file_info_get_attribute_uint32(i, "unix::mode") : 0;
        self->m_owner = Gio::str(g_file_info_get_attribute_string(i, "owner::user"));
        self->m_group = Gio::str(g_file_info_get_attribute_string(i, "owner::group"));
        auto flag = [i](const char *a) { return !g_file_info_has_attribute(i, a) || g_file_info_get_attribute_boolean(i, a); };
        self->m_readable = flag("access::can-read");
        self->m_writable = flag("access::can-write");
        self->m_executable = g_file_info_has_attribute(i, "access::can-execute") && g_file_info_get_attribute_boolean(i, "access::can-execute");
        self->m_canSetMode = g_file_info_has_attribute(i, "unix::mode") && self->m_writable;
        self->m_ready = true;
        self->buildOpenWith();
        emit self->loadedChanged();
        self->readImageSize();
    });
    g_file_query_info_async(file.get(), kAttrs, G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS, G_PRIORITY_DEFAULT, m_cancel.get(), cb, ud);
}

void FileInfo::buildOpenWith()
{
    m_openWith.clear();
    if (m_isDir || m_contentType.isEmpty())
        return;
    const QByteArray ct = m_contentType.toUtf8();
    GAppInfo *def = g_app_info_get_default_for_type(ct.constData(), FALSE);
    const QString defId = def ? Gio::str(g_app_info_get_id(def)) : QString();
    GList *apps = g_app_info_get_all_for_type(ct.constData());
    for (GList *l = apps; l; l = l->next) {
        auto *app = G_APP_INFO(l->data);
        if (!g_app_info_should_show(app))
            continue;
        const QString id = Gio::str(g_app_info_get_id(app));
        GIcon *icon = g_app_info_get_icon(app);
        QString iconName;
        if (icon && G_IS_THEMED_ICON(icon)) {
            const gchar *const *n = g_themed_icon_get_names(G_THEMED_ICON(icon));
            if (n && n[0])
                iconName = QString::fromUtf8(n[0]);
        }
        m_openWith.append(QVariantMap{{QStringLiteral("id"), id}, {QStringLiteral("name"), Gio::str(g_app_info_get_display_name(app))},
                                      {QStringLiteral("iconName"), iconName}, {QStringLiteral("isDefault"), id == defId}});
    }
    g_list_free_full(apps, g_object_unref);
    if (def)
        g_object_unref(def);
}

void FileInfo::readImageSize()
{
    if (!m_contentType.startsWith(QLatin1String("image/")) || m_path.isEmpty())
        return;
    const int gen = m_gen;
    const QString path = m_path;
    QPointer<FileInfo> self(this);
    // QImageReader::size() reads the header only, but still touches the disk, so it runs on a worker.
    QtConcurrent::run([path] { return QImageReader(path).size(); }).then(this, [self, gen](const QSize &s) {
        if (!self || self->m_gen != gen || !s.isValid())
            return;
        self->m_imageSize = s;
        emit self->imageSizeChanged();
    });
}

QString FileInfo::kindName() const { return Fmt::kindDescription(m_contentType, m_isDir); }
QString FileInfo::iconName() const { return Fmt::iconNameFor(m_contentType, m_isDir); }
QString FileInfo::sizeText() const { return m_isDir ? QString() : Fmt::humanSize(m_size); }
QString FileInfo::modifiedText() const { return Fmt::humanDate(m_modified, QDateTime::currentDateTime()); }
QString FileInfo::permissionsOctal() const { return QString::number(m_mode & 07777, 8); }

QString FileInfo::permissionsText() const
{
    QString s;
    static const char sym[] = "rwx";
    for (int i = 0; i < 9; ++i)
        s += (m_mode & (0400 >> i)) ? QLatin1Char(sym[i % 3]) : QLatin1Char('-');
    if (m_mode & 04000) s[2] = (m_mode & 0100) ? QLatin1Char('s') : QLatin1Char('S');
    if (m_mode & 02000) s[5] = (m_mode & 010) ? QLatin1Char('s') : QLatin1Char('S');
    if (m_mode & 01000) s[8] = (m_mode & 01) ? QLatin1Char('t') : QLatin1Char('T');
    return s;
}

QString FileInfo::dimensionsText() const
{
    return m_imageSize.isValid() ? QStringLiteral("%1 × %2").arg(m_imageSize.width()).arg(m_imageSize.height()) : QString();
}

void FileInfo::setPermissions(int mode)
{
    auto file = Gio::fileForString(m_uri);
    if (!file)
        return;
    auto info = Gio::Info::adopt(g_file_info_new());
    g_file_info_set_attribute_uint32(info.get(), "unix::mode", guint32((m_mode & ~07777u) | (quint32(mode) & 07777u)));
    QPointer<FileInfo> self(this);
    auto [cb, ud] = Gio::thunk([self](GObject *src, GAsyncResult *res) {
        Gio::Error err;
        GFileInfo *out = nullptr;
        const bool ok = g_file_set_attributes_finish(G_FILE(src), res, &out, err.out());
        if (out)
            g_object_unref(out);
        if (!self)
            return;
        if (ok) {
            emit self->permissionsSet();
            self->reload();
        } else {
            emit self->failed(err.message());
        }
    });
    g_file_set_attributes_async(file.get(), info.get(), G_FILE_QUERY_INFO_NONE, G_PRIORITY_DEFAULT, m_cancel.get(), cb, ud);
}

void FileInfo::launchApp(const QString &appId, const QStringList &uris)
{
    QString id = appId;
    if (!id.endsWith(QLatin1String(".desktop")))
        id += QStringLiteral(".desktop");
    GDesktopAppInfo *app = g_desktop_app_info_new(id.toUtf8().constData());
    if (!app) {
        emit launchFinished(false, tr("Application %1 not found").arg(appId));
        return;
    }
    GList *list = nullptr;
    for (const QString &u : uris) {
        auto f = Gio::fileForString(u);
        if (f)
            list = g_list_append(list, g_strdup(Gio::uriOf(f.get()).toUtf8().constData()));
    }
    GAppLaunchContext *ctx = g_app_launch_context_new();
    QPointer<FileInfo> self(this);
    auto holder = Gio::Obj<GAppInfo>::adopt(G_APP_INFO(app));
    auto [cb, ud] = Gio::thunk([self, holder, ctx](GObject *src, GAsyncResult *res) {
        Gio::Error err;
        const bool ok = g_app_info_launch_uris_finish(G_APP_INFO(src), res, err.out());
        g_object_unref(ctx);
        if (self)
            emit self->launchFinished(ok, err.message());
    });
    g_app_info_launch_uris_async(holder.get(), list, ctx, nullptr, cb, ud);
    g_list_free_full(list, g_free);
}

void FileInfo::launch(const QString &uri, const QString &appId)
{
    if (appId.isEmpty()) {
        launchDefault({uri});
        return;
    }
    if (inSandbox()) {
        launchViaPortal({uri});
        return;
    }
    launchApp(appId, {uri});
}

void FileInfo::launchDefault(const QStringList &uris)
{
    if (inSandbox()) {
        launchViaPortal(uris);
        return;
    }
    // Files group by default application so one program gets all of its files in one call.
    QHash<QString, QStringList> byApp;
    for (const QString &u : uris) {
        auto f = Gio::fileForString(u);
        if (!f)
            continue;
        const QString canonical = Gio::uriOf(f.get());
        const QByteArray base = Gio::take(g_file_get_basename(f.get())).toUtf8();
        char *ct = g_content_type_guess(base.constData(), nullptr, 0, nullptr);
        GAppInfo *app = ct ? g_app_info_get_default_for_type(ct, FALSE) : nullptr;
        g_free(ct);
        byApp[app ? Gio::str(g_app_info_get_id(app)) : QString()].append(canonical);
        if (app)
            g_object_unref(app);
    }
    for (auto it = byApp.begin(); it != byApp.end(); ++it) {
        if (!it.key().isEmpty()) {
            launchApp(it.key(), it.value());
            continue;
        }
        for (const QString &u : std::as_const(it.value())) {
            GAppLaunchContext *ctx = g_app_launch_context_new();
            QPointer<FileInfo> self(this);
            auto [cb, ud] = Gio::thunk([self, ctx](GObject *, GAsyncResult *res) {
                Gio::Error err;
                const bool ok = g_app_info_launch_default_for_uri_finish(res, err.out());
                g_object_unref(ctx);
                if (self)
                    emit self->launchFinished(ok, err.message());
            });
            g_app_info_launch_default_for_uri_async(u.toUtf8().constData(), ctx, nullptr, cb, ud);
        }
    }
}

void FileInfo::launchViaPortal(const QStringList &uris)
{
    auto bus = QDBusConnection::sessionBus();
    for (const QString &u : uris) {
        auto f = Gio::fileForString(u);
        if (!f)
            continue;
        const QString path = Gio::pathOf(f.get());
        QDBusMessage msg;
        if (!path.isEmpty()) {
            // A file descriptor lets the portal open files that the sandbox cannot name.
            const int fd = ::open(path.toUtf8().constData(), O_RDONLY | O_CLOEXEC);
            if (fd < 0) {
                emit launchFinished(false, tr("Cannot open %1").arg(path));
                continue;
            }
            msg = QDBusMessage::createMethodCall(QStringLiteral("org.freedesktop.portal.Desktop"), QStringLiteral("/org/freedesktop/portal/desktop"),
                                                 QStringLiteral("org.freedesktop.portal.OpenURI"), QStringLiteral("OpenFile"));
            msg << QString() << QVariant::fromValue(QDBusUnixFileDescriptor(fd)) << QVariantMap();
            ::close(fd);
        } else {
            msg = QDBusMessage::createMethodCall(QStringLiteral("org.freedesktop.portal.Desktop"), QStringLiteral("/org/freedesktop/portal/desktop"),
                                                 QStringLiteral("org.freedesktop.portal.OpenURI"), QStringLiteral("OpenURI"));
            msg << QString() << Gio::uriOf(f.get()) << QVariantMap();
        }
        auto *w = new QDBusPendingCallWatcher(bus.asyncCall(msg), this);
        connect(w, &QDBusPendingCallWatcher::finished, this, [this, w] {
            QDBusPendingReply<QDBusObjectPath> reply = *w;
            emit launchFinished(!reply.isError(), reply.isError() ? reply.error().message() : QString());
            w->deleteLater();
        });
    }
}
