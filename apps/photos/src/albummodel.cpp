#include "albummodel.h"
#include "library.h"

#include <QDir>
#include <algorithm>

AlbumModel::AlbumModel(QObject *parent)
    : QAbstractListModel(parent)
    , m_lib(PhotoLibrary::instance())
{
    connect(m_lib, &PhotoLibrary::changed, this, &AlbumModel::rebuild);
    connect(m_lib, &PhotoLibrary::favouritesChanged, this, &AlbumModel::rebuild);
    rebuild();
}

int AlbumModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : int(m_albums.size());
}

QHash<int, QByteArray> AlbumModel::roleNames() const
{
    return {{KindRole, "kind"}, {PathRole, "path"}, {NameRole, "name"}, {ParentRole, "parentName"}, {CountRole, "count"}, {CoversRole, "covers"}, {LatestRole, "latest"}};
}

QVariant AlbumModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_albums.size())
        return {};
    const Album &a = m_albums[index.row()];
    switch (role) {
    case KindRole: return a.kind;
    case PathRole: return a.path;
    case NameRole: return a.name;
    case ParentRole: return a.parentName;
    case CountRole: return a.count;
    case CoversRole: {
        QVariantList l;
        for (const QUrl &u : a.covers)
            l << u;
        return l;
    }
    case LatestRole: return a.latest;
    }
    return {};
}

QVariantMap AlbumModel::get(int row) const
{
    QVariantMap m;
    if (row < 0 || row >= m_albums.size())
        return m;
    const QHash<int, QByteArray> names = roleNames();
    for (auto it = names.cbegin(); it != names.cend(); ++it)
        m.insert(QString::fromLatin1(it.value()), data(index(row), it.key()));
    return m;
}

void AlbumModel::setSource(const QString &s)
{
    if (m_source == s)
        return;
    m_source = s;
    emit sourceChanged();
    rebuild();
}

QVariantList AlbumModel::folders() const
{
    QVariantList out;
    for (const Album &a : m_albums)
        if (a.kind == QLatin1String("folder") && m_lib->sourceOf(a.path).isEmpty())
            out << QVariantMap{{QStringLiteral("path"), a.path}, {QStringLiteral("name"), a.name}};
    return out;
}

void AlbumModel::rebuild()
{
    QHash<QString, int> byPath;
    QList<Album> albums;
    Album fav;
    fav.kind = QStringLiteral("favourites");
    fav.name = tr("Favourites");
    const QString home = QDir::homePath();
    // The library is newest first, so the first photos met are the covers and the latest date.
    for (const Photo &p : m_lib->photos()) {
        if (m_lib->sourceOf(p.path) != m_source)
            continue;
        if (m_lib->isFavourite(p.path)) {
            ++fav.count;
            if (fav.covers.size() < 4)
                fav.covers << PhotoLibrary::thumb(p.path, p.mtime);
            if (!fav.latest.isValid())
                fav.latest = QDateTime::fromMSecsSinceEpoch(p.taken);
        }
        auto it = byPath.find(p.folder);
        if (it == byPath.end()) {
            Album a;
            a.kind = QStringLiteral("folder");
            a.path = p.folder;
            a.name = p.folder.section(QLatin1Char('/'), -1);
            const QString parent = p.folder.section(QLatin1Char('/'), 0, -2);
            a.parentName = parent == home ? QStringLiteral("~") : parent.startsWith(home + QLatin1Char('/')) ? QStringLiteral("~") + parent.mid(home.size()) : parent;
            a.latest = QDateTime::fromMSecsSinceEpoch(p.taken);
            it = byPath.insert(p.folder, int(albums.size()));
            albums.append(a);
        }
        Album &a = albums[*it];
        ++a.count;
        if (a.covers.size() < 4)
            a.covers << PhotoLibrary::thumb(p.path, p.mtime);
    }
    if (fav.count > 0 && m_source.isEmpty())
        albums.prepend(fav);
    const int before = m_albums.size();
    beginResetModel();
    m_albums = albums;
    endResetModel();
    if (before != m_albums.size())
        emit countChanged();
}
