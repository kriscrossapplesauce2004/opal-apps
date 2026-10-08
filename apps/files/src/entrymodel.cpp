#include "entrymodel.h"

#include "starred.h"
#include "thumbnailer.h"

#include <QQmlEngine>

namespace Gio {

const char *const kEntryAttributes = "standard::name,standard::display-name,standard::type,standard::is-hidden,standard::is-symlink,"
                                     "standard::size,standard::fast-content-type,standard::target-uri,time::modified";

std::shared_ptr<Entry> makeEntry(GFileInfo *info, GFile *parent)
{
    auto e = std::make_shared<Entry>();
    const char *rawName = g_file_info_get_name(info);
    e->name = str(rawName);
    const QString display = str(g_file_info_get_display_name(info));
    if (display != e->name)
        e->displayName = display;
    const GFileType type = g_file_info_get_file_type(info);
    e->isDir = type == G_FILE_TYPE_DIRECTORY || type == G_FILE_TYPE_MOUNTABLE;
    e->isHidden = g_file_info_get_is_hidden(info);
    e->isSymlink = g_file_info_get_is_symlink(info);
    e->size = e->isDir ? 0 : qint64(g_file_info_get_size(info));
    e->modifiedMs = qint64(g_file_info_get_attribute_uint64(info, "time::modified")) * 1000;
    const char *ct = g_file_info_get_attribute_string(info, "standard::fast-content-type");
    if (!ct && g_file_info_has_attribute(info, "standard::content-type"))
        ct = g_file_info_get_content_type(info);
    e->contentType = e->isDir ? QStringLiteral("inode/directory") : str(ct);
    // GIO reports empty files as zero-size; the name tells more.
    if (!e->isDir && e->contentType == QLatin1String("application/x-zerosize"))
        e->contentType = take(g_content_type_guess(rawName, nullptr, 0, nullptr));
    e->kind = Fmt::kindFor(e->contentType, e->isDir);
    if (const char *target = g_file_info_get_attribute_string(info, "standard::target-uri")) {
        e->uri = str(target);
    } else {
        auto child = File::adopt(g_file_get_child(parent, rawName));
        e->uri = uriOf(child.get());
    }
    return e;
}

}

EntryModel::EntryModel(QObject *parent)
    : QAbstractListModel(parent)
{
    connect(Starred::instance(), &Starred::changed, this, [this] {
        if (!m_view.isEmpty())
            emit dataChanged(index(0), index(int(m_view.size()) - 1), {StarredRole});
    });
    connect(Thumbnailer::instance(), &Thumbnailer::ready, this, &EntryModel::onThumbReady);
    connect(Thumbnailer::instance(), &Thumbnailer::failed, this, &EntryModel::onThumbFailed);
}

EntryModel::~EntryModel() = default;

int EntryModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : int(m_view.size());
}

QHash<int, QByteArray> EntryModel::roleNames() const
{
    return {
        {NameRole, "name"}, {DisplayNameRole, "displayName"}, {UriRole, "uri"}, {PathRole, "path"},
        {IsDirRole, "isDir"}, {IsHiddenRole, "isHidden"}, {IsSymlinkRole, "isSymlink"}, {SizeRole, "size"},
        {SizeTextRole, "sizeText"}, {ModifiedRole, "modified"}, {ModifiedTextRole, "modifiedText"}, {KindRole, "kind"},
        {KindNameRole, "kindName"}, {ContentTypeRole, "contentType"}, {IconNameRole, "iconName"},
        {ChildCountRole, "childCount"}, {ThumbnailRole, "thumbnail"}, {StarredRole, "starred"}, {SelectedRole, "selected"},
        {ParentPathRole, "parentPath"}, {SnippetRole, "snippet"}, {LineRole, "line"},
    };
}

static QString localPathOf(const QString &uri)
{
    return uri.startsWith(QLatin1String("file://")) ? QUrl(uri).toLocalFile() : QString();
}

QVariant EntryModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_view.size())
        return {};
    Entry *e = m_view.at(index.row());
    switch (role) {
    case NameRole:
    case Qt::DisplayRole: return e->name;
    case DisplayNameRole: return e->displayName.isEmpty() ? e->name : e->displayName;
    case UriRole: return e->uri;
    case PathRole: return localPathOf(e->uri);
    case IsDirRole: return e->isDir;
    case IsHiddenRole: return e->isHidden;
    case IsSymlinkRole: return e->isSymlink;
    case SizeRole: return e->size;
    case SizeTextRole:
        if (e->isDir) {
            if (e->childCount < 0 && !e->countPending)
                const_cast<EntryModel *>(this)->requestChildCount(e);
            return e->childCount >= 0 ? Fmt::itemCount(e->childCount) : QString();
        }
        return Fmt::humanSize(e->size);
    case ModifiedRole: return e->modifiedMs ? QVariant(QDateTime::fromMSecsSinceEpoch(e->modifiedMs)) : QVariant(QDateTime());
    case ModifiedTextRole:
        return e->modifiedMs ? Fmt::humanDate(QDateTime::fromMSecsSinceEpoch(e->modifiedMs), QDateTime::currentDateTime()) : QString();
    case KindRole: return Fmt::kindString(e->kind);
    case KindNameRole: return Fmt::kindDescription(e->contentType, e->isDir);
    case ContentTypeRole: return e->contentType;
    case IconNameRole: return Fmt::iconNameFor(e->contentType, e->isDir);
    case ChildCountRole:
        if (e->isDir && e->childCount < 0 && !e->countPending)
            const_cast<EntryModel *>(this)->requestChildCount(e);
        return e->childCount;
    case ThumbnailRole: return e->thumbnail;
    case StarredRole: return Starred::instance()->isStarred(e->uri);
    case SelectedRole: return e->selected;
    case ParentPathRole: {
        const QString p = localPathOf(e->uri);
        if (!p.isEmpty())
            return p.left(std::max<qsizetype>(1, p.lastIndexOf(QLatin1Char('/'))));
        return QUrl(e->uri).adjusted(QUrl::RemoveFilename).toString();
    }
    case SnippetRole: return e->snippet;
    case LineRole: return e->line;
    }
    return {};
}

bool EntryModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (role != SelectedRole || !index.isValid() || index.row() >= m_view.size())
        return false;
    setSelected(index.row(), value.toBool());
    return true;
}

void EntryModel::notifyRow(Entry *e, const QList<int> &roles)
{
    if (!e->inView)
        return;
    const int row = int(m_view.indexOf(e));
    if (row >= 0)
        emit dataChanged(index(row), index(row), roles);
}

void EntryModel::setEntrySelected(Entry *e, bool on)
{
    if (e->selected == on)
        return;
    e->selected = on;
    const int d = on ? 1 : -1;
    m_selCount += d;
    if (!e->isDir)
        m_selSize += d * e->size;
}

void EntryModel::recomputeStats()
{
    qint64 total = 0, selSize = 0;
    int selCount = 0;
    for (const Entry *e : std::as_const(m_view)) {
        if (!e->isDir)
            total += e->size;
        if (e->selected) {
            ++selCount;
            if (!e->isDir)
                selSize += e->size;
        }
    }
    const bool countChangedNow = total != m_totalSize;
    const bool selChanged = selCount != m_selCount || selSize != m_selSize;
    m_totalSize = total;
    m_selCount = selCount;
    m_selSize = selSize;
    emit countChanged();
    Q_UNUSED(countChangedNow)
    if (selChanged)
        emit selectionChanged();
}

void EntryModel::select(int row, SelectMode mode)
{
    if (row < 0 || row >= m_view.size())
        return;
    const int before = m_selCount;
    const qint64 beforeSize = m_selSize;
    const int rows = int(m_view.size());
    switch (mode) {
    case Replace:
        for (Entry *e : std::as_const(m_view))
            setEntrySelected(e, false);
        setEntrySelected(m_view.at(row), true);
        m_anchor = row;
        emit dataChanged(index(0), index(rows - 1), {SelectedRole});
        break;
    case Toggle:
        setEntrySelected(m_view.at(row), !m_view.at(row)->selected);
        m_anchor = row;
        emit dataChanged(index(row), index(row), {SelectedRole});
        break;
    case Range: {
        const int a = m_anchor >= 0 && m_anchor < rows ? m_anchor : row;
        const int lo = std::min(a, row), hi = std::max(a, row);
        for (int i = 0; i < rows; ++i)
            setEntrySelected(m_view.at(i), i >= lo && i <= hi);
        emit dataChanged(index(0), index(rows - 1), {SelectedRole});
        break;
    }
    }
    if (before != m_selCount || beforeSize != m_selSize || mode != Toggle)
        emit selectionChanged();
}

void EntryModel::setSelected(int row, bool on)
{
    if (row < 0 || row >= m_view.size() || m_view.at(row)->selected == on)
        return;
    setEntrySelected(m_view.at(row), on);
    emit dataChanged(index(row), index(row), {SelectedRole});
    emit selectionChanged();
}

void EntryModel::selectAll()
{
    for (Entry *e : std::as_const(m_view))
        setEntrySelected(e, true);
    if (!m_view.isEmpty())
        emit dataChanged(index(0), index(int(m_view.size()) - 1), {SelectedRole});
    emit selectionChanged();
}

void EntryModel::clearSelection()
{
    if (m_selCount == 0)
        return;
    for (Entry *e : std::as_const(m_view))
        setEntrySelected(e, false);
    m_anchor = -1;
    if (!m_view.isEmpty())
        emit dataChanged(index(0), index(int(m_view.size()) - 1), {SelectedRole});
    emit selectionChanged();
}

void EntryModel::invertSelection()
{
    for (Entry *e : std::as_const(m_view))
        setEntrySelected(e, !e->selected);
    if (!m_view.isEmpty())
        emit dataChanged(index(0), index(int(m_view.size()) - 1), {SelectedRole});
    emit selectionChanged();
}

QStringList EntryModel::selectedUris() const
{
    QStringList out;
    for (const Entry *e : m_view)
        if (e->selected)
            out << e->uri;
    return out;
}

QStringList EntryModel::selectedPaths() const
{
    QStringList out;
    for (const Entry *e : m_view)
        if (e->selected) {
            const QString p = localPathOf(e->uri);
            if (!p.isEmpty())
                out << p;
        }
    return out;
}

QString EntryModel::uriAt(int row) const
{
    return row >= 0 && row < m_view.size() ? m_view.at(row)->uri : QString();
}

int EntryModel::indexOfUri(const QString &uri) const
{
    for (qsizetype i = 0; i < m_view.size(); ++i)
        if (m_view.at(i)->uri == uri)
            return int(i);
    return -1;
}

QVariantMap EntryModel::get(int row) const
{
    QVariantMap m;
    if (row < 0 || row >= m_view.size())
        return m;
    const auto names = roleNames();
    for (auto it = names.constBegin(); it != names.constEnd(); ++it)
        m.insert(QString::fromLatin1(it.value()), data(index(row), it.key()));
    return m;
}

void EntryModel::toggleStar(int row)
{
    if (row >= 0 && row < m_view.size())
        Starred::instance()->toggle(m_view.at(row)->uri);
}

void EntryModel::setThumbnailSize(int s)
{
    s = Thumbnailer::bucket(s);
    if (s == m_thumbSize)
        return;
    m_thumbSize = s;
    emit thumbnailSizeChanged();
}

void EntryModel::requestThumbnails(int first, int last)
{
    if (m_view.isEmpty())
        return;
    first = std::max(0, first);
    last = std::min(last, int(m_view.size()) - 1);
    for (int i = first; i <= last; ++i) {
        Entry *e = m_view.at(i);
        const bool eligible = e->kind == Fmt::Kind::Image || e->kind == Fmt::Kind::Video || e->kind == Fmt::Kind::Pdf;
        if (!eligible || e->thumbState != 0 || !e->uri.startsWith(QLatin1String("file://")))
            continue;
        e->thumbState = 1;
        m_thumbWait.insert(e->uri, e);
        Thumbnailer::instance()->request(e->uri, m_thumbSize);
    }
}

void EntryModel::onThumbReady(const QString &uri, int)
{
    auto it = m_thumbWait.find(uri);
    if (it == m_thumbWait.end())
        return;
    Entry *e = it.value();
    m_thumbWait.erase(it);
    e->thumbState = 2;
    e->thumbnail = Thumbnailer::url(e->uri, e->modifiedMs / 1000);
    notifyRow(e, {ThumbnailRole});
}

void EntryModel::onThumbFailed(const QString &uri)
{
    auto it = m_thumbWait.find(uri);
    if (it == m_thumbWait.end())
        return;
    it.value()->thumbState = 3;
    m_thumbWait.erase(it);
}

void EntryModel::forgetEntry(Entry *e)
{
    m_thumbWait.remove(e->uri);
}

void EntryModel::forgetAllEntries()
{
    m_thumbWait.clear();
}

void EntryModel::componentComplete()
{
    m_complete = true;
    Thumbnailer::attach(qmlEngine(this));
    loaded();
}
