#pragma once
#include "gobj.h"

#include <QAbstractListModel>
#include <QFileSystemWatcher>
#include <QQmlParserStatus>
#include <QTimer>
#include <QtQml/qqmlregistration.h>
#include <map>
#include <vector>

// Sidebar model: Home, Recent, Starred, user folders, bookmarks, drives, network and Trash.
//
// QML element "Places" (QAbstractListModel).
// Roles: name, uri, path, iconName (freedesktop), glyph (Material Symbols name), section, kind, mounted, canMount,
//   canUnmount, canEject, editable (bookmarks), freeText ("412 GB free"), usedFraction (0..1, -1 unknown), detail
//   (Trash: "1.4 GB"; devices: freeText), count (Trash item count, else -1).
// section: "places" | "folders" | "bookmarks" | "devices" | "network" | "trash".
// kind: home recent starred xdg bookmark drive volume mount network-root network-mount trash.
// Properties: count, trashCount, trashSize (bytes), trashSizeText, configDir and homePath (set before use in tests),
//   bookmarksFile (read only).
// Methods: get(row), indexOfUri(uri), mountPlace(row), unmountPlace(row), ejectPlace(row), addBookmark(uri, name),
//   removeBookmark(uri), renameBookmark(uri, name), moveBookmark(fromRow, toRow), connectToServer(uri),
//   replyCredentials(id, user, password, domain, remember, anonymous), replyQuestion(id, choice), cancelPrompt(id),
//   storageInfo(location), refresh().
// Signals: error(title, message), serverConnected(uri), credentialsRequested(id, message, defaultUser, defaultDomain,
//   needUsername, needDomain, needPassword, canAnonymous, canRemember), questionRequested(id, message, choices),
//   storageInfoReady(location, total, free, used, text), mountedPlace(uri).
// Everything GIO emits (volume monitor, mount operations, file monitors) is created on the GUI thread.
class Places : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(int trashCount READ trashCount NOTIFY trashChanged)
    Q_PROPERTY(qint64 trashSize READ trashSize NOTIFY trashChanged)
    Q_PROPERTY(QString trashSizeText READ trashSizeText NOTIFY trashChanged)
    Q_PROPERTY(QString configDir READ configDir WRITE setConfigDir NOTIFY pathsChanged)
    Q_PROPERTY(QString homePath READ homePath WRITE setHomePath NOTIFY pathsChanged)
    Q_PROPERTY(QString bookmarksFile READ bookmarksFile NOTIFY pathsChanged)

public:
    enum Role {
        NameRole = Qt::UserRole + 1,
        UriRole,
        PathRole,
        IconNameRole,
        GlyphRole,
        SectionRole,
        KindRole,
        MountedRole,
        CanMountRole,
        CanUnmountRole,
        CanEjectRole,
        EditableRole,
        FreeTextRole,
        UsedFractionRole,
        DetailRole,
        CountRole,
    };
    Q_ENUM(Role)

    explicit Places(QObject *parent = nullptr);
    ~Places() override;

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    int count() const { return int(m_items.size()); }
    int trashCount() const { return m_trashCount; }
    qint64 trashSize() const { return m_trashSize; }
    QString trashSizeText() const;
    QString configDir() const { return m_configDir; }
    void setConfigDir(const QString &dir);
    QString homePath() const { return m_home; }
    void setHomePath(const QString &dir);
    QString bookmarksFile() const;

    Q_INVOKABLE QVariantMap get(int row) const;
    Q_INVOKABLE int indexOfUri(const QString &uri) const;
    Q_INVOKABLE void mountPlace(int row);
    Q_INVOKABLE void unmountPlace(int row);
    Q_INVOKABLE void ejectPlace(int row);
    Q_INVOKABLE void addBookmark(const QString &uri, const QString &name = QString());
    Q_INVOKABLE void removeBookmark(const QString &uri);
    Q_INVOKABLE void renameBookmark(const QString &uri, const QString &name);
    Q_INVOKABLE void moveBookmark(int fromRow, int toRow);
    Q_INVOKABLE void connectToServer(const QString &uri);
    Q_INVOKABLE void replyCredentials(int id, const QString &user, const QString &password, const QString &domain, bool remember, bool anonymous);
    Q_INVOKABLE void replyQuestion(int id, int choice);
    Q_INVOKABLE void cancelPrompt(int id);
    Q_INVOKABLE void storageInfo(const QString &location);
    Q_INVOKABLE void refresh();

signals:
    void countChanged();
    void trashChanged();
    void pathsChanged();
    void error(const QString &title, const QString &message);
    void serverConnected(const QString &uri);
    void mountedPlace(const QString &uri);
    void credentialsRequested(int id, const QString &message, const QString &defaultUser, const QString &defaultDomain, bool needUsername,
                              bool needDomain, bool needPassword, bool canAnonymous, bool canRemember);
    void questionRequested(int id, const QString &message, const QStringList &choices);
    void storageInfoReady(const QString &location, qint64 total, qint64 free, qint64 used, const QString &text);

private:
    struct Item
    {
        QString name, uri, path, iconName, glyph, section, kind, freeText, detail;
        bool mounted = true, canMount = false, canUnmount = false, canEject = false, editable = false;
        double used = -1;
        int count = -1;
        Gio::Obj<GMount> mount;
        Gio::Obj<GVolume> volume;
        Gio::Obj<GDrive> drive;
    };
    struct Bookmark
    {
        QString uri, name;
    };
    struct Prompt
    {
        Gio::Obj<GMountOperation> op;
    };
    struct XdgDir
    {
        QString name, path, glyph;
    };
    struct Loaded;

    void rebuild();
    void scheduleRebuild();
    void collectDevices(std::vector<Item> &out);
    void collectFolders(std::vector<Item> &out);
    void queryFree(const QString &key, GFile *root);
    void setupMonitors();
    void loadFiles();
    void saveBookmarks();
    void refreshTrash();
    void scanTrashSize();
    Gio::Obj<GMountOperation> makeOperation();
    void report(const QString &title, GError *err);
    void applyLoaded(const Loaded &l, int gen);

    std::vector<Item> m_items;
    QList<Bookmark> m_bookmarks;
    QList<XdgDir> m_xdg;
    QString m_configDir, m_home, m_lastWritten;
    bool m_bookmarksLoaded = false;
    QTimer m_rebuildTimer, m_trashTimer;
    QFileSystemWatcher m_watcher;
    Gio::Obj<GVolumeMonitor> m_monitor;
    Gio::Obj<GFileMonitor> m_trashMonitor;
    Gio::Cancellable m_cancel;
    std::map<QString, std::pair<qint64, qint64>> m_free; // by mount root URI: total, free
    std::map<int, Prompt> m_prompts;
    int m_nextPrompt = 1;
    int m_trashCount = -1;
    qint64 m_trashSize = 0;
    int m_trashGen = 0;
    int m_loadGen = 0;
};
