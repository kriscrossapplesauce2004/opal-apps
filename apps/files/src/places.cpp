#include "places.h"

#include "format.h"

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QRegularExpression>
#include <QSaveFile>
#include <QStandardPaths>
#include <QUrl>
#include <QtConcurrent/QtConcurrentRun>
#include <algorithm>

namespace {

// Pointer pair that signal handlers get as user data.
struct Ctx
{
    QPointer<Places> self;
};

void freeCtx(gpointer d, GClosure *) { delete static_cast<Ctx *>(d); }

QString iconOf(GIcon *icon, const QString &fallback)
{
    if (icon && G_IS_THEMED_ICON(icon)) {
        const gchar *const *names = g_themed_icon_get_names(G_THEMED_ICON(icon));
        if (names && names[0])
            return QString::fromUtf8(names[0]);
    }
    return fallback;
}

bool isRemoteScheme(const QString &s)
{
    static const QStringList remote = {QStringLiteral("smb"), QStringLiteral("sftp"), QStringLiteral("ssh"), QStringLiteral("ftp"),
                                       QStringLiteral("ftps"), QStringLiteral("dav"), QStringLiteral("davs"), QStringLiteral("nfs"),
                                       QStringLiteral("afp"), QStringLiteral("http"), QStringLiteral("https"), QStringLiteral("network"),
                                       QStringLiteral("google-drive"), QStringLiteral("onedrive"), QStringLiteral("cifs")};
    return remote.contains(s);
}

QString schemeOf(const QString &uri)
{
    const int c = uri.indexOf(QLatin1Char(':'));
    return c > 0 ? uri.left(c) : QString();
}

struct TrashStats
{
    int count = 0;
    qint64 size = 0;
};

// Walks the home trash. Runs on a worker thread.
TrashStats scanTrash(const QString &filesDir)
{
    TrashStats st;
    const QDir dir(filesDir);
    st.count = int(dir.entryList(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System).size());
    QDirIterator it(filesDir, QDir::Files | QDir::Hidden | QDir::System | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        it.next();
        st.size += it.fileInfo().size();
    }
    return st;
}

QString trashFilesDir()
{
    return QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QStringLiteral("/Trash/files");
}

}

struct Places::Loaded
{
    QByteArray bookmarkText;
    QList<XdgDir> dirs;
};

Places::Places(QObject *parent)
    : QAbstractListModel(parent)
    , m_configDir(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation))
    , m_home(QDir::homePath())
{
    m_cancel = Gio::Cancellable::adopt(g_cancellable_new());
    m_rebuildTimer.setSingleShot(true);
    m_rebuildTimer.setInterval(60);
    connect(&m_rebuildTimer, &QTimer::timeout, this, &Places::rebuild);
    m_trashTimer.setSingleShot(true);
    m_trashTimer.setInterval(400);
    connect(&m_trashTimer, &QTimer::timeout, this, &Places::refreshTrash);
    connect(&m_watcher, &QFileSystemWatcher::fileChanged, this, [this] { loadFiles(); });
    connect(&m_watcher, &QFileSystemWatcher::directoryChanged, this, [this](const QString &dir) {
        if (dir.endsWith(QLatin1String("/Trash/files")) || dir.endsWith(QLatin1String("/Trash/info")))
            m_trashTimer.start();
        else
            loadFiles();
    });
    setupMonitors();
    rebuild();
    loadFiles();
    refreshTrash();
}

Places::~Places()
{
    if (m_cancel)
        g_cancellable_cancel(m_cancel.get());
}

QString Places::bookmarksFile() const { return m_configDir + QStringLiteral("/gtk-3.0/bookmarks"); }

void Places::setConfigDir(const QString &dir)
{
    if (dir == m_configDir)
        return;
    m_configDir = dir;
    emit pathsChanged();
    loadFiles();
}

void Places::setHomePath(const QString &dir)
{
    if (dir == m_home)
        return;
    m_home = dir;
    emit pathsChanged();
    loadFiles();
    scheduleRebuild();
}

QString Places::trashSizeText() const { return Fmt::humanSize(m_trashSize); }

int Places::rowCount(const QModelIndex &parent) const { return parent.isValid() ? 0 : int(m_items.size()); }

QHash<int, QByteArray> Places::roleNames() const
{
    return {
        {NameRole, "name"}, {UriRole, "uri"}, {PathRole, "path"}, {IconNameRole, "iconName"}, {GlyphRole, "glyph"},
        {SectionRole, "section"}, {KindRole, "kind"}, {MountedRole, "mounted"}, {CanMountRole, "canMount"},
        {CanUnmountRole, "canUnmount"}, {CanEjectRole, "canEject"}, {EditableRole, "editable"}, {FreeTextRole, "freeText"},
        {UsedFractionRole, "usedFraction"}, {DetailRole, "detail"}, {CountRole, "count"},
    };
}

QVariant Places::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= int(m_items.size()))
        return {};
    const Item &i = m_items[size_t(index.row())];
    switch (role) {
    case NameRole:
    case Qt::DisplayRole: return i.name;
    case UriRole: return i.uri;
    case PathRole: return i.path;
    case IconNameRole: return i.iconName;
    case GlyphRole: return i.glyph;
    case SectionRole: return i.section;
    case KindRole: return i.kind;
    case MountedRole: return i.mounted;
    case CanMountRole: return i.canMount;
    case CanUnmountRole: return i.canUnmount;
    case CanEjectRole: return i.canEject;
    case EditableRole: return i.editable;
    case FreeTextRole: return i.freeText;
    case UsedFractionRole: return i.used;
    case DetailRole: return i.detail;
    case CountRole: return i.count;
    }
    return {};
}

QVariantMap Places::get(int row) const
{
    QVariantMap m;
    if (row < 0 || row >= int(m_items.size()))
        return m;
    const auto names = roleNames();
    for (auto it = names.constBegin(); it != names.constEnd(); ++it)
        m.insert(QString::fromLatin1(it.value()), data(index(row), it.key()));
    return m;
}

int Places::indexOfUri(const QString &uri) const
{
    for (size_t i = 0; i < m_items.size(); ++i)
        if (m_items[i].uri == uri)
            return int(i);
    return -1;
}

void Places::refresh()
{
    loadFiles();
    refreshTrash();
    m_free.clear();
    scheduleRebuild();
}

void Places::scheduleRebuild()
{
    if (!m_rebuildTimer.isActive())
        m_rebuildTimer.start();
}

// The volume monitor and the trash monitor are made here, on the GUI thread, because GIO emits on the creating thread's context.
void Places::setupMonitors()
{
    m_monitor = Gio::Obj<GVolumeMonitor>::adopt(g_volume_monitor_get());
    static const char *const signalNames[] = {"mount-added", "mount-removed", "mount-changed", "volume-added", "volume-removed",
                                              "volume-changed", "drive-connected", "drive-disconnected", "drive-changed"};
    for (const char *name : signalNames) {
        g_signal_connect_data(m_monitor.get(), name, G_CALLBACK(+[](GVolumeMonitor *, GObject *, gpointer d) {
                                  auto *c = static_cast<Ctx *>(d);
                                  if (c->self)
                                      c->self->scheduleRebuild();
                              }),
                              new Ctx{QPointer<Places>(this)}, freeCtx, GConnectFlags(0));
    }
    auto trash = Gio::fileForUri(QStringLiteral("trash:///"));
    Gio::Error err;
    m_trashMonitor = Gio::Obj<GFileMonitor>::adopt(g_file_monitor_directory(trash.get(), G_FILE_MONITOR_NONE, nullptr, err.out()));
    if (m_trashMonitor)
        g_signal_connect_data(m_trashMonitor.get(), "changed", G_CALLBACK(+[](GFileMonitor *, GFile *, GFile *, GFileMonitorEvent, gpointer d) {
                                  auto *c = static_cast<Ctx *>(d);
                                  if (c->self)
                                      c->self->m_trashTimer.start();
                              }),
                              new Ctx{QPointer<Places>(this)}, freeCtx, GConnectFlags(0));
    // The local trash folder also gets a plain watcher, which works without gvfs.
    const QString files = trashFilesDir();
    m_watcher.addPath(files);
    m_watcher.addPath(QFileInfo(files).absolutePath() + QStringLiteral("/info"));
}

void Places::loadFiles()
{
    const QString bm = bookmarksFile();
    const QString cfg = m_configDir;
    const QString home = m_home;
    const int gen = ++m_loadGen;
    QPointer<Places> self(this);
    QtConcurrent::run([bm, cfg, home]() {
        Loaded l;
        QFile f(bm);
        if (f.open(QIODevice::ReadOnly))
            l.bookmarkText = f.readAll();
        // user-dirs.dirs lines look like XDG_MUSIC_DIR="$HOME/Music".
        struct Def { const char *key, *fallback, *glyph; };
        static const Def defs[] = {{"DESKTOP", "Desktop", "desktop_windows"}, {"DOCUMENTS", "Documents", "description"},
                                   {"DOWNLOAD", "Downloads", "download"}, {"MUSIC", "Music", "music_note"},
                                   {"PICTURES", "Pictures", "image"}, {"VIDEOS", "Videos", "movie"}};
        QHash<QString, QString> found;
        QFile u(cfg + QStringLiteral("/user-dirs.dirs"));
        if (u.open(QIODevice::ReadOnly)) {
            static const QRegularExpression re(QStringLiteral("^XDG_(\\w+)_DIR=\"(.*)\"\\s*$"));
            for (const QByteArray &line : u.readAll().split('\n')) {
                const auto m = re.match(QString::fromUtf8(line));
                if (m.hasMatch())
                    found.insert(m.captured(1), QString(m.captured(2)).replace(QStringLiteral("$HOME"), home));
            }
        }
        for (const Def &d : defs) {
            const QString path = found.value(QLatin1String(d.key), home + QLatin1Char('/') + QLatin1String(d.fallback));
            if (path == home || !QFileInfo(path).isDir())
                continue;
            l.dirs.append({QFileInfo(path).fileName(), path, QLatin1String(d.glyph)});
        }
        return l;
    }).then(this, [self, gen](const Loaded &l) {
        if (self)
            self->applyLoaded(l, gen);
    });
    m_watcher.addPath(bm);
    m_watcher.addPath(QFileInfo(bm).absolutePath());
}

void Places::applyLoaded(const Loaded &l, int gen)
{
    if (gen != m_loadGen)
        return;
    m_xdg = l.dirs;
    // Our own write must not replace the in-memory list.
    if (!m_bookmarksLoaded || QString::fromUtf8(l.bookmarkText) != m_lastWritten) {
        QList<Bookmark> list;
        for (const QByteArray &raw : l.bookmarkText.split('\n')) {
            const QString line = QString::fromUtf8(raw).trimmed();
            if (line.isEmpty())
                continue;
            const int sp = line.indexOf(QLatin1Char(' '));
            list.append({sp < 0 ? line : line.left(sp), sp < 0 ? QString() : line.mid(sp + 1).trimmed()});
        }
        m_bookmarks = list;
        m_lastWritten = QString::fromUtf8(l.bookmarkText);
    }
    m_bookmarksLoaded = true;
    rebuild();
}

void Places::saveBookmarks()
{
    QString text;
    for (const Bookmark &b : std::as_const(m_bookmarks))
        text += b.uri + (b.name.isEmpty() ? QString() : QLatin1Char(' ') + b.name) + QLatin1Char('\n');
    m_lastWritten = text;
    const QString path = bookmarksFile();
    // Writes go through a worker and a rename, so a crash never leaves half a file.
    (void)QtConcurrent::run([path, text] {
        QDir().mkpath(QFileInfo(path).absolutePath());
        QSaveFile f(path);
        if (f.open(QIODevice::WriteOnly)) {
            f.write(text.toUtf8());
            f.commit();
        }
    });
}

void Places::addBookmark(const QString &uriIn, const QString &name)
{
    auto f = Gio::fileForString(uriIn);
    if (!f)
        return;
    const QString uri = Gio::uriOf(f.get());
    for (const Bookmark &b : std::as_const(m_bookmarks))
        if (b.uri == uri)
            return;
    m_bookmarks.append({uri, QString(name).replace(QLatin1Char('\n'), QLatin1Char(' ')).trimmed()});
    saveBookmarks();
    scheduleRebuild();
}

void Places::removeBookmark(const QString &uriIn)
{
    auto f = Gio::fileForString(uriIn);
    const QString uri = f ? Gio::uriOf(f.get()) : uriIn;
    const auto before = m_bookmarks.size();
    m_bookmarks.removeIf([&](const Bookmark &b) { return b.uri == uri; });
    if (m_bookmarks.size() != before) {
        saveBookmarks();
        scheduleRebuild();
    }
}

void Places::renameBookmark(const QString &uriIn, const QString &name)
{
    auto f = Gio::fileForString(uriIn);
    const QString uri = f ? Gio::uriOf(f.get()) : uriIn;
    for (Bookmark &b : m_bookmarks)
        if (b.uri == uri) {
            b.name = QString(name).replace(QLatin1Char('\n'), QLatin1Char(' ')).trimmed();
            saveBookmarks();
            scheduleRebuild();
            return;
        }
}

void Places::moveBookmark(int fromRow, int toRow)
{
    auto bookmarkIndex = [this](int row) -> int {
        if (row < 0 || row >= int(m_items.size()) || m_items[size_t(row)].kind != QLatin1String("bookmark"))
            return -1;
        int n = 0;
        for (int i = 0; i < row; ++i)
            if (m_items[size_t(i)].kind == QLatin1String("bookmark"))
                ++n;
        return n;
    };
    const int from = bookmarkIndex(fromRow), to = bookmarkIndex(toRow);
    if (from < 0 || to < 0 || from == to)
        return;
    m_bookmarks.move(from, to);
    saveBookmarks();
    rebuild();
}

void Places::collectFolders(std::vector<Item> &out)
{
    Item home;
    home.name = tr("Home");
    home.uri = Gio::uriOf(Gio::fileForPath(m_home).get());
    home.path = m_home;
    home.iconName = QStringLiteral("user-home");
    home.glyph = QStringLiteral("home");
    home.section = QStringLiteral("places");
    home.kind = QStringLiteral("home");
    out.push_back(home);
    Item recent;
    recent.name = tr("Recent");
    recent.uri = QStringLiteral("recent:///");
    recent.iconName = QStringLiteral("document-open-recent");
    recent.glyph = QStringLiteral("schedule");
    recent.section = QStringLiteral("places");
    recent.kind = QStringLiteral("recent");
    out.push_back(recent);
    Item star;
    star.name = tr("Starred");
    star.uri = QStringLiteral("starred:///");
    star.iconName = QStringLiteral("starred");
    star.glyph = QStringLiteral("star");
    star.section = QStringLiteral("places");
    star.kind = QStringLiteral("starred");
    out.push_back(star);
    for (const XdgDir &d : std::as_const(m_xdg)) {
        Item i;
        i.name = d.name;
        i.path = d.path;
        i.uri = Gio::uriOf(Gio::fileForPath(d.path).get());
        i.iconName = QStringLiteral("folder");
        i.glyph = d.glyph;
        i.section = QStringLiteral("folders");
        i.kind = QStringLiteral("xdg");
        out.push_back(i);
    }
    for (const Bookmark &b : std::as_const(m_bookmarks)) {
        Item i;
        i.uri = b.uri;
        i.path = b.uri.startsWith(QLatin1String("file://")) ? QUrl(b.uri).toLocalFile() : QString();
        i.name = !b.name.isEmpty() ? b.name : (i.path.isEmpty() ? QUrl::fromPercentEncoding(b.uri.mid(b.uri.lastIndexOf(QLatin1Char('/')) + 1).toUtf8()) : QFileInfo(i.path).fileName());
        if (i.name.isEmpty())
            i.name = b.uri;
        i.iconName = QStringLiteral("folder");
        i.glyph = QStringLiteral("folder");
        i.section = QStringLiteral("bookmarks");
        i.kind = QStringLiteral("bookmark");
        i.editable = true;
        out.push_back(i);
    }
}

void Places::collectDevices(std::vector<Item> &out)
{
    auto fill = [this](Item &it, GMount *mount, GVolume *volume, GDrive *drive) {
        it.mount = Gio::Obj<GMount>::retain(mount);
        it.volume = Gio::Obj<GVolume>::retain(volume);
        it.drive = Gio::Obj<GDrive>::retain(drive);
        it.mounted = mount != nullptr;
        if (mount) {
            auto root = Gio::File::adopt(g_mount_get_default_location(mount));
            it.uri = Gio::uriOf(root.get());
            it.path = Gio::pathOf(root.get());
            it.canUnmount = g_mount_can_unmount(mount);
        } else if (volume) {
            auto root = Gio::File::adopt(g_volume_get_activation_root(volume));
            it.uri = root ? Gio::uriOf(root.get()) : QString();
            it.canMount = g_volume_can_mount(volume);
        }
        it.canEject = (mount && g_mount_can_eject(mount)) || (volume && g_volume_can_eject(volume)) || (drive && g_drive_can_eject(drive));
        const QString scheme = schemeOf(it.uri);
        const bool remote = !scheme.isEmpty() && isRemoteScheme(scheme);
        const bool phone = scheme == QLatin1String("mtp") || scheme == QLatin1String("gphoto2") || scheme == QLatin1String("afc");
        const bool removable = drive && (g_drive_is_removable(drive) || g_drive_is_media_removable(drive));
        it.section = remote ? QStringLiteral("network") : QStringLiteral("devices");
        it.kind = remote ? QStringLiteral("network-mount") : volume ? QStringLiteral("volume") : QStringLiteral("mount");
        it.glyph = remote ? QStringLiteral("dns") : phone ? QStringLiteral("smartphone") : removable ? QStringLiteral("usb") : QStringLiteral("hard_drive");
        GIcon *icon = mount ? g_mount_get_icon(mount) : volume ? g_volume_get_icon(volume) : nullptr;
        it.iconName = iconOf(icon, remote ? QStringLiteral("folder-remote") : QStringLiteral("drive-harddisk"));
        if (icon)
            g_object_unref(icon);
        auto cached = m_free.find(it.uri);
        if (cached != m_free.end() && cached->second.first > 0) {
            it.freeText = Fmt::humanSize(cached->second.second) + tr(" free");
            it.used = 1.0 - double(cached->second.second) / double(cached->second.first);
            it.detail = it.freeText;
        }
    };

    GList *volumes = g_volume_monitor_get_volumes(m_monitor.get());
    for (GList *l = volumes; l; l = l->next) {
        auto *v = G_VOLUME(l->data);
        GMount *m = g_volume_get_mount(v);
        GDrive *d = g_volume_get_drive(v);
        if (m && g_mount_is_shadowed(m)) {
            g_object_unref(m);
            if (d)
                g_object_unref(d);
            continue;
        }
        Item it;
        it.name = Gio::take(g_volume_get_name(v));
        fill(it, m, v, d);
        out.push_back(std::move(it));
        if (m)
            g_object_unref(m);
        if (d)
            g_object_unref(d);
    }
    g_list_free_full(volumes, g_object_unref);

    GList *mounts = g_volume_monitor_get_mounts(m_monitor.get());
    for (GList *l = mounts; l; l = l->next) {
        auto *m = G_MOUNT(l->data);
        GVolume *v = g_mount_get_volume(m);
        if (v) {
            g_object_unref(v);
            continue;
        }
        if (g_mount_is_shadowed(m))
            continue;
        Item it;
        it.name = Gio::take(g_mount_get_name(m));
        fill(it, m, nullptr, nullptr);
        out.push_back(std::move(it));
    }
    g_list_free_full(mounts, g_object_unref);

    // Drives that hold no volume yet (empty card reader) still show, so the user can eject them.
    GList *drives = g_volume_monitor_get_connected_drives(m_monitor.get());
    for (GList *l = drives; l; l = l->next) {
        auto *d = G_DRIVE(l->data);
        GList *dv = g_drive_get_volumes(d);
        const bool hasVolumes = dv != nullptr;
        g_list_free_full(dv, g_object_unref);
        if (hasVolumes || !g_drive_is_removable(d))
            continue;
        Item it;
        it.name = Gio::take(g_drive_get_name(d));
        fill(it, nullptr, nullptr, d);
        it.kind = QStringLiteral("drive");
        it.mounted = false;
        out.push_back(std::move(it));
    }
    g_list_free_full(drives, g_object_unref);
}

void Places::rebuild()
{
    m_rebuildTimer.stop();
    std::vector<Item> items;
    collectFolders(items);
    std::vector<Item> devices;
    collectDevices(devices);
    // Sections keep their design order: places, folders, bookmarks, devices, network, trash.
    for (const char *section : {"devices", "network"}) {
        if (QLatin1String(section) == QLatin1String("network")) {
            Item net;
            net.name = tr("Network");
            net.uri = QStringLiteral("network:///");
            net.iconName = QStringLiteral("network-workgroup");
            net.glyph = QStringLiteral("lan");
            net.section = QStringLiteral("network");
            net.kind = QStringLiteral("network-root");
            items.push_back(net);
        }
        for (Item &d : devices)
            if (d.section == QLatin1String(section))
                items.push_back(d);
    }
    Item trash;
    trash.name = tr("Trash");
    trash.uri = QStringLiteral("trash:///");
    trash.iconName = QStringLiteral("user-trash");
    trash.glyph = QStringLiteral("delete");
    trash.section = QStringLiteral("trash");
    trash.kind = QStringLiteral("trash");
    trash.count = m_trashCount;
    trash.detail = m_trashCount > 0 ? trashSizeText() : QString();
    items.push_back(trash);

    beginResetModel();
    m_items = std::move(items);
    endResetModel();
    emit countChanged();
    for (const Item &i : m_items)
        if (i.mounted && !i.uri.isEmpty() && i.section != QLatin1String("network") && i.kind != QLatin1String("trash") && i.kind != QLatin1String("bookmark")
            && i.kind != QLatin1String("recent") && i.kind != QLatin1String("starred") && i.kind != QLatin1String("network-root") && !m_free.count(i.uri)) {
            auto root = Gio::fileForUri(i.uri);
            queryFree(i.uri, root.get());
        }
}

void Places::queryFree(const QString &key, GFile *root)
{
    m_free[key] = {0, 0};
    QPointer<Places> self(this);
    auto [cb, ud] = Gio::thunk([self, key](GObject *src, GAsyncResult *res) {
        Gio::Error err;
        auto info = Gio::Info::adopt(g_file_query_filesystem_info_finish(G_FILE(src), res, err.out()));
        if (!self || !info)
            return;
        const qint64 total = qint64(g_file_info_get_attribute_uint64(info.get(), "filesystem::size"));
        const qint64 free = qint64(g_file_info_get_attribute_uint64(info.get(), "filesystem::free"));
        if (total <= 0)
            return;
        self->m_free[key] = {total, free};
        for (size_t r = 0; r < self->m_items.size(); ++r) {
            Item &it = self->m_items[r];
            if (it.uri != key)
                continue;
            it.freeText = Fmt::humanSize(free) + self->tr(" free");
            it.detail = it.freeText;
            it.used = 1.0 - double(free) / double(total);
            emit self->dataChanged(self->index(int(r)), self->index(int(r)), {FreeTextRole, UsedFractionRole, DetailRole});
        }
    });
    g_file_query_filesystem_info_async(root, "filesystem::size,filesystem::free", G_PRIORITY_LOW, m_cancel.get(), cb, ud);
}

void Places::storageInfo(const QString &location)
{
    auto f = Gio::fileForString(location);
    if (!f)
        return;
    QPointer<Places> self(this);
    auto [cb, ud] = Gio::thunk([self, location](GObject *src, GAsyncResult *res) {
        Gio::Error err;
        auto info = Gio::Info::adopt(g_file_query_filesystem_info_finish(G_FILE(src), res, err.out()));
        if (!self)
            return;
        if (!info) {
            emit self->storageInfoReady(location, -1, -1, -1, QString());
            return;
        }
        const qint64 total = qint64(g_file_info_get_attribute_uint64(info.get(), "filesystem::size"));
        const qint64 free = qint64(g_file_info_get_attribute_uint64(info.get(), "filesystem::free"));
        const qint64 used = g_file_info_has_attribute(info.get(), "filesystem::used") ? qint64(g_file_info_get_attribute_uint64(info.get(), "filesystem::used")) : total - free;
        emit self->storageInfoReady(location, total, free, used, self->tr("%1 free of %2").arg(Fmt::humanSize(free), Fmt::humanSize(total)));
    });
    g_file_query_filesystem_info_async(f.get(), "filesystem::size,filesystem::free,filesystem::used", G_PRIORITY_DEFAULT, m_cancel.get(), cb, ud);
}

void Places::refreshTrash()
{
    const int gen = ++m_trashGen;
    const QString dir = trashFilesDir();
    QPointer<Places> self(this);
    QtConcurrent::run([dir] { return scanTrash(dir); }).then(this, [self, gen](const TrashStats &st) {
        if (!self || gen != self->m_trashGen)
            return;
        if (st.count == self->m_trashCount && st.size == self->m_trashSize)
            return;
        self->m_trashCount = st.count;
        self->m_trashSize = st.size;
        emit self->trashChanged();
        for (size_t r = 0; r < self->m_items.size(); ++r) {
            Item &it = self->m_items[r];
            if (it.kind != QLatin1String("trash"))
                continue;
            it.count = st.count;
            it.detail = st.count > 0 ? self->trashSizeText() : QString();
            emit self->dataChanged(self->index(int(r)), self->index(int(r)), {CountRole, DetailRole});
        }
    });
}

void Places::report(const QString &title, GError *err)
{
    if (!err)
        return;
    // A cancelled password dialog is not an error to show.
    if (g_error_matches(err, G_IO_ERROR, G_IO_ERROR_CANCELLED) || g_error_matches(err, G_IO_ERROR, G_IO_ERROR_FAILED_HANDLED))
        return;
    emit error(title, QString::fromUtf8(err->message));
}

// The mount operation is made on the GUI thread so its prompts come back here.
Gio::Obj<GMountOperation> Places::makeOperation()
{
    auto op = Gio::Obj<GMountOperation>::adopt(g_mount_operation_new());
    g_signal_connect_data(op.get(), "ask-password",
                          G_CALLBACK(+[](GMountOperation *o, const char *message, const char *user, const char *domain, GAskPasswordFlags flags, gpointer d) {
                              auto *c = static_cast<Ctx *>(d);
                              if (!c->self) {
                                  g_mount_operation_reply(o, G_MOUNT_OPERATION_ABORTED);
                                  return;
                              }
                              const int id = c->self->m_nextPrompt++;
                              c->self->m_prompts[id] = {Gio::Obj<GMountOperation>::retain(o)};
                              emit c->self->credentialsRequested(id, Gio::str(message), Gio::str(user), Gio::str(domain), flags & G_ASK_PASSWORD_NEED_USERNAME,
                                                                 flags & G_ASK_PASSWORD_NEED_DOMAIN, flags & G_ASK_PASSWORD_NEED_PASSWORD,
                                                                 flags & G_ASK_PASSWORD_ANONYMOUS_SUPPORTED, flags & G_ASK_PASSWORD_SAVING_SUPPORTED);
                          }),
                          new Ctx{QPointer<Places>(this)}, freeCtx, GConnectFlags(0));
    g_signal_connect_data(op.get(), "ask-question",
                          G_CALLBACK(+[](GMountOperation *o, const char *message, const char **choices, gpointer d) {
                              auto *c = static_cast<Ctx *>(d);
                              if (!c->self) {
                                  g_mount_operation_reply(o, G_MOUNT_OPERATION_ABORTED);
                                  return;
                              }
                              QStringList list;
                              for (int i = 0; choices && choices[i]; ++i)
                                  list << QString::fromUtf8(choices[i]);
                              const int id = c->self->m_nextPrompt++;
                              c->self->m_prompts[id] = {Gio::Obj<GMountOperation>::retain(o)};
                              emit c->self->questionRequested(id, Gio::str(message), list);
                          }),
                          new Ctx{QPointer<Places>(this)}, freeCtx, GConnectFlags(0));
    g_signal_connect_data(op.get(), "show-processes",
                          G_CALLBACK(+[](GMountOperation *o, const char *, GArray *, const char **, gpointer d) {
                              auto *c = static_cast<Ctx *>(d);
                              // Closing other programs is the user's job; the unmount stops with a clear message.
                              g_mount_operation_reply(o, G_MOUNT_OPERATION_ABORTED);
                              if (c->self)
                                  emit c->self->error(c->self->tr("Device is busy"), c->self->tr("Close the programs that use this device and try again."));
                          }),
                          new Ctx{QPointer<Places>(this)}, freeCtx, GConnectFlags(0));
    return op;
}

void Places::replyCredentials(int id, const QString &user, const QString &password, const QString &domain, bool remember, bool anonymous)
{
    auto it = m_prompts.find(id);
    if (it == m_prompts.end())
        return;
    GMountOperation *op = it->second.op.get();
    if (anonymous) {
        g_mount_operation_set_anonymous(op, TRUE);
    } else {
        g_mount_operation_set_username(op, user.toUtf8().constData());
        g_mount_operation_set_password(op, password.toUtf8().constData());
        if (!domain.isEmpty())
            g_mount_operation_set_domain(op, domain.toUtf8().constData());
    }
    g_mount_operation_set_password_save(op, remember ? G_PASSWORD_SAVE_PERMANENTLY : G_PASSWORD_SAVE_NEVER);
    g_mount_operation_reply(op, G_MOUNT_OPERATION_HANDLED);
    m_prompts.erase(it);
}

void Places::replyQuestion(int id, int choice)
{
    auto it = m_prompts.find(id);
    if (it == m_prompts.end())
        return;
    g_mount_operation_set_choice(it->second.op.get(), choice);
    g_mount_operation_reply(it->second.op.get(), G_MOUNT_OPERATION_HANDLED);
    m_prompts.erase(it);
}

void Places::cancelPrompt(int id)
{
    auto it = m_prompts.find(id);
    if (it == m_prompts.end())
        return;
    g_mount_operation_reply(it->second.op.get(), G_MOUNT_OPERATION_ABORTED);
    m_prompts.erase(it);
}

void Places::connectToServer(const QString &uri)
{
    auto file = Gio::fileForString(uri.trimmed());
    if (!file) {
        emit error(tr("Cannot connect"), tr("The address is empty."));
        return;
    }
    auto op = makeOperation();
    QPointer<Places> self(this);
    const QString target = Gio::uriOf(file.get());
    auto [cb, ud] = Gio::thunk([self, target, op](GObject *src, GAsyncResult *res) {
        Gio::Error err;
        const bool ok = g_file_mount_enclosing_volume_finish(G_FILE(src), res, err.out());
        if (!self)
            return;
        if (ok || err.is(G_IO_ERROR_ALREADY_MOUNTED)) {
            emit self->serverConnected(target);
            self->scheduleRebuild();
        } else if (!err.cancelled() && !err.is(G_IO_ERROR_FAILED_HANDLED)) {
            emit self->error(self->tr("Cannot connect to %1").arg(target), err.message());
        }
    });
    g_file_mount_enclosing_volume(file.get(), G_MOUNT_MOUNT_NONE, op.get(), m_cancel.get(), cb, ud);
}

void Places::mountPlace(int row)
{
    if (row < 0 || row >= int(m_items.size()))
        return;
    const Item &it = m_items[size_t(row)];
    if (!it.volume || it.mounted)
        return;
    auto op = makeOperation();
    QPointer<Places> self(this);
    const QString name = it.name;
    auto [cb, ud] = Gio::thunk([self, name, op](GObject *src, GAsyncResult *res) {
        Gio::Error err;
        const bool ok = g_volume_mount_finish(G_VOLUME(src), res, err.out());
        if (!self)
            return;
        if (ok)
            emit self->mountedPlace(name);
        else if (!err.is(G_IO_ERROR_ALREADY_MOUNTED))
            emit self->error(self->tr("Cannot mount %1").arg(name), err.message());
    });
    g_volume_mount(it.volume.get(), G_MOUNT_MOUNT_NONE, op.get(), m_cancel.get(), cb, ud);
}

void Places::unmountPlace(int row)
{
    if (row < 0 || row >= int(m_items.size()))
        return;
    const Item &it = m_items[size_t(row)];
    if (!it.mount)
        return;
    auto op = makeOperation();
    QPointer<Places> self(this);
    const QString name = it.name;
    auto [cb, ud] = Gio::thunk([self, name, op](GObject *src, GAsyncResult *res) {
        Gio::Error err;
        const bool ok = g_mount_unmount_with_operation_finish(G_MOUNT(src), res, err.out());
        if (self && !ok && !err.cancelled() && !err.is(G_IO_ERROR_FAILED_HANDLED))
            emit self->error(self->tr("Cannot unmount %1").arg(name), err.message());
    });
    g_mount_unmount_with_operation(it.mount.get(), G_MOUNT_UNMOUNT_NONE, op.get(), m_cancel.get(), cb, ud);
}

void Places::ejectPlace(int row)
{
    if (row < 0 || row >= int(m_items.size()))
        return;
    const Item &it = m_items[size_t(row)];
    auto op = makeOperation();
    QPointer<Places> self(this);
    const QString name = it.name;
    auto done = [self, name, op](bool ok, const Gio::Error &err) {
        if (self && !ok && !err.cancelled() && !err.is(G_IO_ERROR_FAILED_HANDLED))
            emit self->error(self->tr("Cannot eject %1").arg(name), err.message());
    };
    if (it.mount && g_mount_can_eject(it.mount.get())) {
        auto [cb, ud] = Gio::thunk([done](GObject *src, GAsyncResult *res) {
            Gio::Error err;
            const bool ok = g_mount_eject_with_operation_finish(G_MOUNT(src), res, err.out());
            done(ok, err);
        });
        g_mount_eject_with_operation(it.mount.get(), G_MOUNT_UNMOUNT_NONE, op.get(), m_cancel.get(), cb, ud);
    } else if (it.volume && g_volume_can_eject(it.volume.get())) {
        auto [cb, ud] = Gio::thunk([done](GObject *src, GAsyncResult *res) {
            Gio::Error err;
            const bool ok = g_volume_eject_with_operation_finish(G_VOLUME(src), res, err.out());
            done(ok, err);
        });
        g_volume_eject_with_operation(it.volume.get(), G_MOUNT_UNMOUNT_NONE, op.get(), m_cancel.get(), cb, ud);
    } else if (it.drive && g_drive_can_eject(it.drive.get())) {
        auto [cb, ud] = Gio::thunk([done](GObject *src, GAsyncResult *res) {
            Gio::Error err;
            const bool ok = g_drive_eject_with_operation_finish(G_DRIVE(src), res, err.out());
            done(ok, err);
        });
        g_drive_eject_with_operation(it.drive.get(), G_MOUNT_UNMOUNT_NONE, op.get(), m_cancel.get(), cb, ud);
    } else if (it.mount && g_mount_can_unmount(it.mount.get())) {
        unmountPlace(row);
    }
}
