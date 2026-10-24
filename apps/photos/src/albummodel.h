#pragma once
#include <QAbstractListModel>
#include <QDateTime>
#include <QUrl>
#include <QtQml/qqmlregistration.h>

class PhotoLibrary;

// Albums: every folder that holds photos, newest first, plus the Favourites album when it has photos.
// QML: AlbumModel { source }. source is a device root from PhotoLibrary.sources, or "" for this PC.
// Roles: kind ("folder" or "favourites"), path, name, parentName, count, covers (up to four thumbnail URLs), latest (date).
// Properties: count. Methods: get(row) -> map, folders() -> [{path, name}] for album pickers (local folders only).
class AlbumModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QString source READ source WRITE setSource NOTIFY sourceChanged)
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    enum Roles { KindRole = Qt::UserRole + 1, PathRole, NameRole, ParentRole, CountRole, CoversRole, LatestRole };

    explicit AlbumModel(QObject *parent = nullptr);
    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    QString source() const { return m_source; }
    void setSource(const QString &s);
    int count() const { return int(m_albums.size()); }

    Q_INVOKABLE QVariantMap get(int row) const;
    Q_INVOKABLE QVariantList folders() const;

signals:
    void sourceChanged();
    void countChanged();

private:
    struct Album
    {
        QString kind;
        QString path;
        QString name;
        QString parentName;
        int count = 0;
        QList<QUrl> covers;
        QDateTime latest;
    };
    void rebuild();

    PhotoLibrary *m_lib;
    QString m_source;
    QList<Album> m_albums;
};
