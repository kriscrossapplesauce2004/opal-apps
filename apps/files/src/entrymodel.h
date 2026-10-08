#pragma once
#include "format.h"
#include "gobj.h"

#include <QAbstractListModel>
#include <QCollator>
#include <QDateTime>
#include <QHash>
#include <QQmlParserStatus>
#include <QUrl>
#include <QVariantMap>
#include <QtQml/qqmlregistration.h>
#include <optional>

// One file system entry as the models store it. Built from a GFileInfo.
struct Entry
{
    QString name;
    QString displayName; // empty when equal to name
    QString uri;
    QString contentType;
    QString snippet; // content search: the matching line
    std::optional<QCollatorSortKey> key;
    qint64 size = 0;
    qint64 modifiedMs = 0;
    int childCount = -1;
    int line = 0;
    Fmt::Kind kind = Fmt::Kind::Other;
    bool isDir = false;
    bool isHidden = false;
    bool isSymlink = false;
    bool selected = false;
    bool inView = false;
    bool countPending = false;
    bool gone = false; // removed from the folder while still queued
    quint8 thumbState = 0; // 0 none, 1 requested, 2 ready, 3 failed
    QUrl thumbnail;
};

namespace Gio {
// GFileInfo attributes the models ask for. Uses the fast content type so no file is opened.
extern const char *const kEntryAttributes;
// Builds an Entry from info for a child of parent. Thread safe.
std::shared_ptr<Entry> makeEntry(GFileInfo *info, GFile *parent);
}

// Shared base of FolderModel and Search: roles, selection, thumbnails and statistics.
//
// Roles: name, displayName, uri, path, isDir, isHidden, isSymlink, size, sizeText, modified (QDateTime),
// modifiedText, kind (folder/image/video/audio/pdf/archive/text/code/app/other), kindName, contentType,
// iconName, childCount (-1 until counted), thumbnail (QUrl, empty until ready), starred, selected (writable),
// parentPath, snippet and line (content search only).
//
// Properties: count, totalSize (bytes of files shown), selectedCount, selectedSize, thumbnailSize (128/256/512).
// Methods: select(row, mode), setSelected(row, bool), selectAll(), clearSelection(), invertSelection(),
// selectedUris(), selectedPaths(), uriAt(row), indexOfUri(uri), get(row), toggleStar(row),
// requestThumbnails(firstRow, lastRow).
class EntryModel : public QAbstractListModel, public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)
    QML_ANONYMOUS
    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(qint64 totalSize READ totalSize NOTIFY countChanged)
    Q_PROPERTY(int selectedCount READ selectedCount NOTIFY selectionChanged)
    Q_PROPERTY(qint64 selectedSize READ selectedSize NOTIFY selectionChanged)
    Q_PROPERTY(int thumbnailSize READ thumbnailSize WRITE setThumbnailSize NOTIFY thumbnailSizeChanged)

public:
    enum Role {
        NameRole = Qt::UserRole + 1,
        DisplayNameRole,
        UriRole,
        PathRole,
        IsDirRole,
        IsHiddenRole,
        IsSymlinkRole,
        SizeRole,
        SizeTextRole,
        ModifiedRole,
        ModifiedTextRole,
        KindRole,
        KindNameRole,
        ContentTypeRole,
        IconNameRole,
        ChildCountRole,
        ThumbnailRole,
        StarredRole,
        SelectedRole,
        ParentPathRole,
        SnippetRole,
        LineRole,
    };
    Q_ENUM(Role)
    enum SelectMode { Replace, Toggle, Range };
    Q_ENUM(SelectMode)

    explicit EntryModel(QObject *parent = nullptr);
    ~EntryModel() override;

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    QHash<int, QByteArray> roleNames() const override;

    int count() const { return int(m_view.size()); }
    qint64 totalSize() const { return m_totalSize; }
    int selectedCount() const { return m_selCount; }
    qint64 selectedSize() const { return m_selSize; }
    int thumbnailSize() const { return m_thumbSize; }
    void setThumbnailSize(int s);

    Q_INVOKABLE void select(int row, SelectMode mode = Replace);
    Q_INVOKABLE void setSelected(int row, bool on);
    Q_INVOKABLE void selectAll();
    Q_INVOKABLE void clearSelection();
    Q_INVOKABLE void invertSelection();
    Q_INVOKABLE QStringList selectedUris() const;
    Q_INVOKABLE QStringList selectedPaths() const;
    Q_INVOKABLE QString uriAt(int row) const;
    Q_INVOKABLE int indexOfUri(const QString &uri) const;
    Q_INVOKABLE QVariantMap get(int row) const;
    Q_INVOKABLE void toggleStar(int row);
    Q_INVOKABLE void requestThumbnails(int firstRow, int lastRow);

    void classBegin() override { m_complete = false; }
    void componentComplete() override;

signals:
    void countChanged();
    void selectionChanged();
    void thumbnailSizeChanged();

protected:
    // Hook for folder models: start counting the children of a folder entry.
    virtual void requestChildCount(Entry *) {}
    // Called once the QML component is complete.
    virtual void loaded() {}
    // Forgets raw pointers to an entry that is about to be freed.
    void forgetEntry(Entry *e);
    void forgetAllEntries();
    void setEntrySelected(Entry *e, bool on);
    void recomputeStats();
    void notifyRow(Entry *e, const QList<int> &roles);

    QVector<Entry *> m_view;
    bool m_complete = true;
    int m_anchor = -1;

private:
    void onThumbReady(const QString &uri, int size);
    void onThumbFailed(const QString &uri);

    QHash<QString, Entry *> m_thumbWait;
    qint64 m_totalSize = 0;
    qint64 m_selSize = 0;
    int m_selCount = 0;
    int m_thumbSize = 256;
};
