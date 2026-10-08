#include "foldermodel.h"

#include "starred.h"

#include <QDateTime>
#include <QStandardPaths>
#include <QtConcurrent/QtConcurrentRun>
#include <algorithm>

namespace {

constexpr int kBatch = 200;
constexpr qint64 kFlushMs = 100;

struct MonitorCtx
{
    QPointer<FolderModel> self;
    quint64 gen;
};

QUrl urlOfFile(GFile *f)
{
    const QString path = Gio::pathOf(f);
    return path.isEmpty() ? QUrl(Gio::uriOf(f)) : QUrl::fromLocalFile(path);
}

}

struct FolderModel::CountJob
{
    QPointer<FolderModel> self;
    quint64 gen = 0;
    QString key;
    int count = 0;
    bool showHidden = false;
    Gio::Obj<GFileEnumerator> en;
    Gio::Cancellable cancel;
};

FolderModel::FolderModel(QObject *parent)
    : EntryModel(parent)
{
    m_collator.setNumericMode(true);
    m_collator.setCaseSensitivity(Qt::CaseInsensitive);
    m_flushTimer.setSingleShot(true);
    m_touchTimer.setSingleShot(true);
    m_statsTimer.setSingleShot(true);
    connect(&m_flushTimer, &QTimer::timeout, this, &FolderModel::flushPending);
    connect(&m_touchTimer, &QTimer::timeout, this, &FolderModel::flushTouched);
    connect(&m_statsTimer, &QTimer::timeout, this, [this] { recomputeStats(); });
    connect(Starred::instance(), &Starred::changed, this, [this] {
        if (m_virtualStarred)
            syncStarred();
    });
}

FolderModel::~FolderModel()
{
    stopAll();
}

void FolderModel::loaded()
{
    if (!m_location.isEmpty())
        startLoad();
}

void FolderModel::setLocation(const QUrl &url)
{
    if (url == m_location)
        return;
    m_location = url;
    emit locationChanged();
    if (m_complete)
        startLoad();
}

void FolderModel::refresh()
{
    if (m_complete)
        startLoad();
}

bool FolderModel::canGoUp() const
{
    if (m_virtualStarred || m_location.isEmpty())
        return false;
    auto f = Gio::fileForUrl(m_location);
    auto p = Gio::File::adopt(f ? g_file_get_parent(f.get()) : nullptr);
    return bool(p);
}

QUrl FolderModel::parentLocation() const
{
    auto f = Gio::fileForUrl(m_location);
    if (!f)
        return {};
    auto p = Gio::File::adopt(g_file_get_parent(f.get()));
    return p ? urlOfFile(p.get()) : QUrl();
}

QVariantList FolderModel::breadcrumbs() const
{
    QVariantList out;
    if (m_virtualStarred) {
        out.append(QVariantMap{{QStringLiteral("name"), tr("Starred")}, {QStringLiteral("uri"), QStringLiteral("starred:///")}});
        return out;
    }
    auto cur = Gio::fileForUrl(m_location);
    auto home = Gio::fileForPath(QString::fromUtf8(g_get_home_dir()));
    while (cur) {
        const QString uri = Gio::uriOf(cur.get());
        QString name;
        auto parent = Gio::File::adopt(g_file_get_parent(cur.get()));
        if (g_file_equal(cur.get(), home.get()))
            name = tr("Home");
        else if (!parent) {
            const QUrl u(uri);
            name = u.isLocalFile() || u.host().isEmpty() ? (u.scheme() == QLatin1String("file") ? QStringLiteral("/") : u.scheme()) : u.host();
        } else
            name = Gio::take(g_file_get_basename(cur.get()));
        out.prepend(QVariantMap{{QStringLiteral("name"), name}, {QStringLiteral("uri"), uri}});
        if (g_file_equal(cur.get(), home.get()))
            break;
        cur = parent;
    }
    return out;
}

int FolderModel::rowOfName(const QString &name) const
{
    auto it = m_all.constFind(name);
    if (it == m_all.constEnd() || !(*it)->inView)
        return -1;
    return int(m_view.indexOf(it->get()));
}

void FolderModel::setLoading(bool on)
{
    if (m_loading == on)
        return;
    m_loading = on;
    emit loadingChanged();
}

void FolderModel::setError(const QString &msg)
{
    if (m_error == msg)
        return;
    m_error = msg;
    emit errorChanged();
}

void FolderModel::stopAll()
{
    ++m_gen;
    if (m_cancel)
        g_cancellable_cancel(m_cancel.get());
    m_cancel.reset();
    if (m_monitor) {
        g_file_monitor_cancel(m_monitor.get());
        m_monitor.reset();
        emit liveChanged();
    }
    m_enum.reset();
    m_countQueue.clear();
    m_countActive = 0;
    m_touched.clear();
    m_inheritSelection.clear();
    m_flushTimer.stop();
    m_touchTimer.stop();
    m_starredInflight = 0;
}

void FolderModel::startLoad()
{
    stopAll();
    beginResetModel();
    m_view.clear();
    forgetAllEntries();
    m_all.clear();
    m_pending.clear();
    m_anchor = -1;
    endResetModel();
    recomputeStats();
    setError({});

    m_virtualStarred = m_location.scheme() == QLatin1String("starred");
    if (m_location.isEmpty()) {
        m_locationName.clear();
        setLoading(false);
        return;
    }
    m_cancel = Gio::Cancellable::adopt(g_cancellable_new());
    m_lastFlush = 0;
    setLoading(true);
    if (m_virtualStarred) {
        m_dir = {};
        m_locationName = tr("Starred");
        emit locationChanged();
        if (Starred::instance()->loaded())
            syncStarred();
        return;
    }
    m_dir = Gio::fileForUrl(m_location);
    if (!m_dir) {
        setError(tr("Invalid location"));
        setLoading(false);
        return;
    }
    const QString home = QString::fromUtf8(g_get_home_dir());
    m_locationName = Gio::pathOf(m_dir.get()) == home ? tr("Home") : Gio::take(g_file_get_basename(m_dir.get()));
    emit locationChanged();

    const quint64 gen = m_gen;
    QPointer<FolderModel> self(this);
    {
        auto [cb, ud] = Gio::thunk([self, gen](GObject *src, GAsyncResult *res) {
            Gio::Error err;
            auto info = Gio::Info::adopt(g_file_query_info_finish(G_FILE(src), res, err.out()));
            if (!self || self->m_gen != gen || !info)
                return;
            const QString name = Gio::str(g_file_info_get_display_name(info.get()));
            if (!name.isEmpty() && self->m_locationName != name && Gio::pathOf(self->m_dir.get()) != QString::fromUtf8(g_get_home_dir())) {
                self->m_locationName = name;
                emit self->locationChanged();
            }
        });
        g_file_query_info_async(m_dir.get(), "standard::display-name", G_FILE_QUERY_INFO_NONE, G_PRIORITY_DEFAULT, m_cancel.get(), cb, ud);
    }
    auto [cb, ud] = Gio::thunk([self, gen](GObject *src, GAsyncResult *res) {
        Gio::Error err;
        auto en = Gio::Obj<GFileEnumerator>::adopt(g_file_enumerate_children_finish(G_FILE(src), res, err.out()));
        if (!self || self->m_gen != gen)
            return;
        if (!en) {
            if (err.is(G_IO_ERROR_NOT_MOUNTED))
                emit self->needsMount(Gio::uriOf(self->m_dir.get()));
            self->setError(err.message());
            self->setLoading(false);
            return;
        }
        self->m_enum = en;
        self->beginMonitor();
        self->nextBatch();
    });
    g_file_enumerate_children_async(m_dir.get(), Gio::kEntryAttributes, G_FILE_QUERY_INFO_NONE, G_PRIORITY_DEFAULT, m_cancel.get(), cb, ud);
}

void FolderModel::nextBatch()
{
    const quint64 gen = m_gen;
    QPointer<FolderModel> self(this);
    auto [cb, ud] = Gio::thunk([self, gen](GObject *src, GAsyncResult *res) {
        Gio::Error err;
        GList *infos = g_file_enumerator_next_files_finish(G_FILE_ENUMERATOR(src), res, err.out());
        if (!self || self->m_gen != gen) {
            g_list_free_full(infos, g_object_unref);
            return;
        }
        if (err) {
            self->setError(err.message());
            self->finishLoad();
            return;
        }
        if (!infos) {
            self->finishLoad();
            return;
        }
        self->onBatch(infos);
        g_list_free_full(infos, g_object_unref);
        self->nextBatch();
    });
    g_file_enumerator_next_files_async(m_enum.get(), kBatch, G_PRIORITY_DEFAULT, m_cancel.get(), cb, ud);
}

void FolderModel::onBatch(GList *infos)
{
    for (GList *l = infos; l; l = l->next) {
        auto e = Gio::makeEntry(G_FILE_INFO(l->data), m_dir.get());
        addOrUpdate(e, e->name);
    }
    scheduleFlush();
}

void FolderModel::finishLoad()
{
    // Closing is async so a slow backend cannot stall the GUI.
    if (m_enum) {
        auto en = m_enum;
        auto [cb, ud] = Gio::thunk([en](GObject *, GAsyncResult *res) {
            Gio::Error err;
            g_file_enumerator_close_finish(en.get(), res, err.out());
        });
        g_file_enumerator_close_async(en.get(), G_PRIORITY_DEFAULT, nullptr, cb, ud);
        m_enum.reset();
    }
    flushPending();
    setLoading(false);
}

void FolderModel::beginMonitor()
{
    // A GFileMonitor sends events on the thread-default context of the thread that made it. Qt worker threads have their own glib context, so the monitor is made here on the GUI thread.
    Gio::Error err;
    auto mon = Gio::Obj<GFileMonitor>::adopt(g_file_monitor_directory(m_dir.get(), G_FILE_MONITOR_WATCH_MOVES, nullptr, err.out()));
    if (!mon)
        return;
    g_signal_connect_data(mon.get(), "changed", G_CALLBACK(&FolderModel::monitorChanged), new MonitorCtx{QPointer<FolderModel>(this), m_gen},
                          [](gpointer d, GClosure *) { delete static_cast<MonitorCtx *>(d); }, GConnectFlags(0));
    m_monitor = mon;
    emit liveChanged();
}

void FolderModel::monitorChanged(GFileMonitor *, GFile *file, GFile *other, GFileMonitorEvent ev, gpointer data)
{
    auto *c = static_cast<MonitorCtx *>(data);
    if (c->self && c->self->m_gen == c->gen)
        c->self->onMonitor(file, other, ev);
}

void FolderModel::onMonitor(GFile *file, GFile *other, GFileMonitorEvent ev)
{
    const QString name = Gio::take(g_file_get_basename(file));
    switch (ev) {
    case G_FILE_MONITOR_EVENT_DELETED:
    case G_FILE_MONITOR_EVENT_MOVED_OUT:
        m_touched.remove(name);
        removeKey(name);
        break;
    case G_FILE_MONITOR_EVENT_RENAMED: {
        if (!other)
            break;
        const QString newName = Gio::take(g_file_get_basename(other));
        auto it = m_all.constFind(name);
        if (it != m_all.constEnd() && (*it)->selected)
            m_inheritSelection.insert(newName);
        m_touched.remove(name);
        removeKey(name);
        touch(newName);
        break;
    }
    case G_FILE_MONITOR_EVENT_CREATED:
    case G_FILE_MONITOR_EVENT_MOVED_IN:
    case G_FILE_MONITOR_EVENT_CHANGED:
    case G_FILE_MONITOR_EVENT_CHANGES_DONE_HINT:
    case G_FILE_MONITOR_EVENT_ATTRIBUTE_CHANGED:
        touch(name);
        break;
    default:
        break;
    }
}

void FolderModel::touch(const QString &name)
{
    m_touched.insert(name);
    if (!m_touchTimer.isActive())
        m_touchTimer.start(80);
}

void FolderModel::flushTouched()
{
    if (!m_dir || !m_cancel)
        return;
    const QSet<QString> names = std::exchange(m_touched, {});
    const quint64 gen = m_gen;
    QPointer<FolderModel> self(this);
    for (const QString &name : names) {
        auto child = Gio::File::adopt(g_file_get_child(m_dir.get(), name.toUtf8().constData()));
        auto [cb, ud] = Gio::thunk([self, gen, name](GObject *src, GAsyncResult *res) {
            Gio::Error err;
            auto info = Gio::Info::adopt(g_file_query_info_finish(G_FILE(src), res, err.out()));
            if (!self || self->m_gen != gen)
                return;
            if (!info) {
                if (err.is(G_IO_ERROR_NOT_FOUND))
                    self->removeKey(name);
                return;
            }
            auto e = Gio::makeEntry(info.get(), self->m_dir.get());
            self->addOrUpdate(e, e->name);
        });
        g_file_query_info_async(child.get(), Gio::kEntryAttributes, G_FILE_QUERY_INFO_NONE, G_PRIORITY_DEFAULT, m_cancel.get(), cb, ud);
    }
}

void FolderModel::syncStarred()
{
    if (!m_virtualStarred || !m_cancel)
        return;
    const QStringList uris = Starred::instance()->uris();
    const QSet<QString> want(uris.begin(), uris.end());
    const QList<QString> have = m_all.keys();
    for (const QString &k : have)
        if (!want.contains(k))
            removeKey(k);
    const quint64 gen = m_gen;
    QPointer<FolderModel> self(this);
    int launched = 0;
    for (const QString &uri : uris) {
        if (m_all.contains(uri))
            continue;
        ++launched;
        ++m_starredInflight;
        auto file = Gio::fileForUri(uri);
        auto [cb, ud] = Gio::thunk([self, gen, file](GObject *src, GAsyncResult *res) {
            Gio::Error err;
            auto info = Gio::Info::adopt(g_file_query_info_finish(G_FILE(src), res, err.out()));
            if (!self || self->m_gen != gen)
                return;
            --self->m_starredInflight;
            if (info) {
                auto parent = Gio::File::adopt(g_file_get_parent(file.get()));
                if (parent) {
                    auto e = Gio::makeEntry(info.get(), parent.get());
                    self->addOrUpdate(e, e->uri);
                }
            }
            if (self->m_starredInflight <= 0) {
                self->flushPending();
                self->setLoading(false);
            }
        });
        g_file_query_info_async(file.get(), Gio::kEntryAttributes, G_FILE_QUERY_INFO_NONE, G_PRIORITY_DEFAULT, m_cancel.get(), cb, ud);
    }
    if (!launched) {
        flushPending();
        setLoading(false);
    }
}

QString FolderModel::keyFor(const Entry &e) const
{
    return m_virtualStarred ? e.uri : e.name;
}

void FolderModel::prepareKey(Entry *e) const
{
    e->key = m_collator.sortKey(e->displayName.isEmpty() ? e->name : e->displayName);
}

int FolderModel::cmpName(const Entry *a, const Entry *b) const
{
    if (a->key && b->key)
        return a->key->compare(*b->key);
    return a->name.compare(b->name);
}

bool FolderModel::less(const Entry *a, const Entry *b) const
{
    if (m_foldersFirst && a->isDir != b->isDir)
        return a->isDir;
    int c = 0;
    switch (m_sortRole) {
    case Name: c = cmpName(a, b); break;
    case Size:
        if (!a->isDir && !b->isDir)
            c = a->size < b->size ? -1 : a->size > b->size ? 1 : 0;
        break;
    case Modified: c = a->modifiedMs < b->modifiedMs ? -1 : a->modifiedMs > b->modifiedMs ? 1 : 0; break;
    case Kind:
        c = int(a->kind) - int(b->kind);
        if (c == 0)
            c = a->contentType.compare(b->contentType);
        break;
    }
    if (c == 0 && m_sortRole != Name)
        c = cmpName(a, b);
    if (c == 0)
        c = a->name.compare(b->name);
    return m_sortOrder == Qt::AscendingOrder ? c < 0 : c > 0;
}

bool FolderModel::visible(const Entry *e) const
{
    if (!m_showHidden && e->isHidden)
        return false;
    if (m_kindMask && !(m_kindMask & (1u << unsigned(e->kind))))
        return false;
    if (!m_nameFilter.isEmpty()) {
        const QString &n = e->displayName.isEmpty() ? e->name : e->displayName;
        if (m_nameRegex.isValid() && !m_nameRegex.pattern().isEmpty()) {
            if (!m_nameRegex.match(n).hasMatch())
                return false;
        } else if (!n.contains(m_nameFilter, Qt::CaseInsensitive)) {
            return false;
        }
    }
    return true;
}

void FolderModel::scheduleStats()
{
    if (!m_statsTimer.isActive())
        m_statsTimer.start(50);
}

void FolderModel::scheduleFlush()
{
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    const qint64 since = now - m_lastFlush;
    if (m_view.isEmpty() || since >= kFlushMs)
        flushPending();
    else if (!m_flushTimer.isActive())
        m_flushTimer.start(int(kFlushMs - since));
}

void FolderModel::flushPending()
{
    m_flushTimer.stop();
    m_lastFlush = QDateTime::currentMSecsSinceEpoch();
    if (m_pending.empty())
        return;
    QVector<Entry *> add;
    add.reserve(qsizetype(m_pending.size()));
    for (const auto &sp : m_pending) {
        Entry *e = sp.get();
        if (e->gone || e->inView)
            continue;
        if (!e->key)
            prepareKey(e);
        if (visible(e))
            add.push_back(e);
    }
    m_pending.clear();
    if (add.isEmpty())
        return;
    std::sort(add.begin(), add.end(), [this](const Entry *a, const Entry *b) { return less(a, b); });
    if (m_view.isEmpty()) {
        beginInsertRows({}, 0, int(add.size()) - 1);
        m_view = add;
        for (Entry *e : std::as_const(m_view))
            e->inView = true;
        endInsertRows();
    } else if (add.size() <= 32) {
        for (Entry *e : std::as_const(add))
            insertSorted(e);
    } else {
        // A big merge is cheaper as one reset than as hundreds of scattered inserts.
        beginResetModel();
        QVector<Entry *> merged(m_view.size() + add.size());
        std::merge(m_view.begin(), m_view.end(), add.begin(), add.end(), merged.begin(), [this](const Entry *a, const Entry *b) { return less(a, b); });
        m_view = merged;
        for (Entry *e : std::as_const(m_view))
            e->inView = true;
        m_anchor = -1;
        endResetModel();
    }
    scheduleStats();
}

void FolderModel::insertSorted(Entry *e)
{
    auto pos = std::lower_bound(m_view.begin(), m_view.end(), e, [this](const Entry *a, const Entry *b) { return less(a, b); });
    const int row = int(pos - m_view.begin());
    beginInsertRows({}, row, row);
    m_view.insert(pos, e);
    e->inView = true;
    endInsertRows();
    scheduleStats();
}

void FolderModel::removeFromView(Entry *e)
{
    const int row = int(m_view.indexOf(e));
    if (row < 0)
        return;
    setEntrySelected(e, false);
    beginRemoveRows({}, row, row);
    m_view.remove(row);
    e->inView = false;
    endRemoveRows();
    scheduleStats();
}

void FolderModel::addOrUpdate(const std::shared_ptr<Entry> &fresh, const QString &key)
{
    auto it = m_all.find(key);
    if (it == m_all.end()) {
        m_all.insert(key, fresh);
        if (m_inheritSelection.remove(key))
        {
            fresh->selected = false;
            setEntrySelected(fresh.get(), true);
        }
        prepareKey(fresh.get());
        if (m_loading) {
            m_pending.push_back(fresh);
        } else if (visible(fresh.get())) {
            insertSorted(fresh.get());
        } else if (fresh->selected) {
            setEntrySelected(fresh.get(), false);
        }
        return;
    }
    Entry *old = it->get();
    // Update in place so the pointer in the view and the thumbnail wait list stay valid.
    const bool sameFile = old->modifiedMs == fresh->modifiedMs && old->size == fresh->size;
    const bool selected = old->selected;
    if (old->selected)
        setEntrySelected(old, false);
    old->displayName = fresh->displayName;
    old->uri = fresh->uri;
    old->contentType = fresh->contentType;
    old->kind = fresh->kind;
    old->size = fresh->size;
    old->modifiedMs = fresh->modifiedMs;
    old->isDir = fresh->isDir;
    old->isHidden = fresh->isHidden;
    old->isSymlink = fresh->isSymlink;
    prepareKey(old);
    if (!sameFile) {
        old->thumbState = 0;
        old->thumbnail = QUrl();
    }
    if (selected)
        setEntrySelected(old, true);
    if (!old->inView) {
        if (!m_loading && visible(old) && std::find_if(m_pending.begin(), m_pending.end(), [old](const auto &p) { return p.get() == old; }) == m_pending.end())
            insertSorted(old);
        return;
    }
    if (!visible(old)) {
        removeFromView(old);
        return;
    }
    const int row = int(m_view.indexOf(old));
    const bool orderOk = (row == 0 || !less(old, m_view.at(row - 1))) && (row == m_view.size() - 1 || !less(m_view.at(row + 1), old));
    if (orderOk) {
        emit dataChanged(index(row), index(row));
    } else {
        const bool wasSelected = old->selected;
        beginRemoveRows({}, row, row);
        m_view.remove(row);
        endRemoveRows();
        old->inView = false;
        auto pos = std::lower_bound(m_view.begin(), m_view.end(), old, [this](const Entry *a, const Entry *b) { return less(a, b); });
        const int nrow = int(pos - m_view.begin());
        beginInsertRows({}, nrow, nrow);
        m_view.insert(pos, old);
        old->inView = true;
        endInsertRows();
        Q_UNUSED(wasSelected)
    }
    scheduleStats();
}

void FolderModel::removeKey(const QString &key)
{
    auto it = m_all.find(key);
    if (it == m_all.end())
        return;
    std::shared_ptr<Entry> keep = *it;
    m_all.erase(it);
    keep->gone = true;
    if (keep->inView)
        removeFromView(keep.get());
    forgetEntry(keep.get());
    scheduleStats();
}

void FolderModel::rebuildView()
{
    beginResetModel();
    m_view.clear();
    for (auto it = m_all.begin(); it != m_all.end(); ++it) {
        Entry *e = it->get();
        e->inView = false;
        if (!e->key)
            prepareKey(e);
        if (visible(e)) {
            e->inView = true;
            m_view.push_back(e);
        } else if (e->selected) {
            setEntrySelected(e, false);
        }
    }
    m_pending.clear();
    std::sort(m_view.begin(), m_view.end(), [this](const Entry *a, const Entry *b) { return less(a, b); });
    m_anchor = -1;
    endResetModel();
    recomputeStats();
}

void FolderModel::setSortRole(SortRole r)
{
    if (r == m_sortRole)
        return;
    m_sortRole = r;
    emit sortChanged();
    rebuildView();
}

void FolderModel::setSortOrder(Qt::SortOrder o)
{
    if (o == m_sortOrder)
        return;
    m_sortOrder = o;
    emit sortChanged();
    rebuildView();
}

void FolderModel::setFoldersFirst(bool on)
{
    if (on == m_foldersFirst)
        return;
    m_foldersFirst = on;
    emit sortChanged();
    rebuildView();
}

void FolderModel::setShowHidden(bool on)
{
    if (on == m_showHidden)
        return;
    m_showHidden = on;
    emit filterChanged();
    rebuildView();
}

void FolderModel::setFilterKind(const QStringList &kinds)
{
    if (kinds == m_filterKinds)
        return;
    m_filterKinds = kinds;
    m_kindMask = 0;
    for (const QString &k : kinds) {
        bool ok = false;
        const Fmt::Kind kind = Fmt::kindFromString(k, &ok);
        if (ok)
            m_kindMask |= 1u << unsigned(kind);
    }
    emit filterChanged();
    rebuildView();
}

void FolderModel::setNameFilter(const QString &text)
{
    if (text == m_nameFilter)
        return;
    m_nameFilter = text;
    if (text.contains(QLatin1Char('*')) || text.contains(QLatin1Char('?')))
        m_nameRegex = QRegularExpression(QRegularExpression::wildcardToRegularExpression(text, QRegularExpression::UnanchoredWildcardConversion),
                                         QRegularExpression::CaseInsensitiveOption);
    else
        m_nameRegex = QRegularExpression();
    emit filterChanged();
    rebuildView();
}

void FolderModel::requestChildCount(Entry *e)
{
    e->countPending = true;
    m_countQueue.push_back(keyFor(*e));
    // Counting starts after the current data() call returns, so no model state changes under a view.
    QMetaObject::invokeMethod(this, &FolderModel::pumpCounts, Qt::QueuedConnection);
}

void FolderModel::pumpCounts()
{
    while (m_countActive < 3 && !m_countQueue.empty()) {
        const QString key = m_countQueue.front();
        m_countQueue.pop_front();
        auto it = m_all.find(key);
        if (it == m_all.end() || !(*it)->isDir)
            continue;
        auto job = std::make_shared<CountJob>();
        job->self = this;
        job->gen = m_gen;
        job->key = key;
        job->showHidden = m_showHidden;
        job->cancel = m_cancel;
        auto dir = Gio::fileForUri((*it)->uri);
        ++m_countActive;
        auto [cb, ud] = Gio::thunk([job](GObject *src, GAsyncResult *res) {
            Gio::Error err;
            auto en = Gio::Obj<GFileEnumerator>::adopt(g_file_enumerate_children_finish(G_FILE(src), res, err.out()));
            if (!job->self || job->self->m_gen != job->gen)
                return;
            if (!en) {
                --job->self->m_countActive;
                job->self->pumpCounts();
                return;
            }
            job->en = en;
            job->self->countStep(job);
        });
        g_file_enumerate_children_async(dir.get(), "standard::name,standard::is-hidden", G_FILE_QUERY_INFO_NONE, G_PRIORITY_LOW, m_cancel.get(), cb, ud);
    }
}

void FolderModel::countStep(const std::shared_ptr<CountJob> &job)
{
    auto [cb, ud] = Gio::thunk([job](GObject *src, GAsyncResult *res) {
        Gio::Error err;
        GList *infos = g_file_enumerator_next_files_finish(G_FILE_ENUMERATOR(src), res, err.out());
        if (!job->self || job->self->m_gen != job->gen) {
            g_list_free_full(infos, g_object_unref);
            return;
        }
        FolderModel *self = job->self;
        if (infos) {
            for (GList *l = infos; l; l = l->next)
                if (job->showHidden || !g_file_info_get_is_hidden(G_FILE_INFO(l->data)))
                    ++job->count;
            g_list_free_full(infos, g_object_unref);
            self->countStep(job);
            return;
        }
        --self->m_countActive;
        if (!err) {
            auto it = self->m_all.find(job->key);
            if (it != self->m_all.end()) {
                (*it)->childCount = job->count;
                self->notifyRow(it->get(), {SizeTextRole, ChildCountRole});
            }
        }
        self->pumpCounts();
    });
    g_file_enumerator_next_files_async(job->en.get(), 1000, G_PRIORITY_LOW, job->cancel.get(), cb, ud);
}
