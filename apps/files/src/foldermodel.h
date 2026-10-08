#pragma once
#include "entrymodel.h"

#include <QRegularExpression>
#include <QSet>
#include <QStringList>
#include <QTimer>
#include <deque>

// Lists one folder through GIO and keeps it live.
//
// QML element "FolderModel" (roles and selection API come from EntryModel).
// Properties:
//   location     QUrl   file path or any GIO URI (file, trash, recent, smb, sftp, mtp, afc, network).
//                       "starred:///" lists the starred files.
//   sortRole     enum   Name, Size, Modified, Kind. Natural, case-insensitive, locale-aware name order.
//   sortOrder    Qt.AscendingOrder or Qt.DescendingOrder.
//   foldersFirst bool   default true.
//   showHidden   bool   default false.
//   filterKind   list   kinds to show ("image", "video", ...). Empty shows all. Add "folder" to keep folders.
//   nameFilter   string search as you type: substring, or glob when it holds * or ?.
//   loading, error, locationName (display name of the folder), canGoUp.
// Methods: refresh(), parentLocation(), breadcrumbs() -> [{name, uri}], rowOfName(name).
// Signals: needsMount(uri) when the location is on an unmounted share; use Places.connectToServer.
// Rows arrive in batches of about 200 and are merged in sorted order at most every 100 ms, so a folder
// of 100000 entries stays responsive. Later changes (create, delete, rename, change) come from a GFileMonitor
// and change single rows without a reset.
class FolderModel : public EntryModel
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QUrl location READ location WRITE setLocation NOTIFY locationChanged)
    Q_PROPERTY(SortRole sortRole READ sortRole WRITE setSortRole NOTIFY sortChanged)
    Q_PROPERTY(Qt::SortOrder sortOrder READ sortOrder WRITE setSortOrder NOTIFY sortChanged)
    Q_PROPERTY(bool foldersFirst READ foldersFirst WRITE setFoldersFirst NOTIFY sortChanged)
    Q_PROPERTY(bool showHidden READ showHidden WRITE setShowHidden NOTIFY filterChanged)
    Q_PROPERTY(QStringList filterKind READ filterKind WRITE setFilterKind NOTIFY filterChanged)
    Q_PROPERTY(QString nameFilter READ nameFilter WRITE setNameFilter NOTIFY filterChanged)
    Q_PROPERTY(bool loading READ loading NOTIFY loadingChanged)
    Q_PROPERTY(QString error READ error NOTIFY errorChanged)
    Q_PROPERTY(QString locationName READ locationName NOTIFY locationChanged)
    Q_PROPERTY(bool canGoUp READ canGoUp NOTIFY locationChanged)
    Q_PROPERTY(bool live READ live NOTIFY liveChanged)

public:
    enum SortRole { Name, Size, Modified, Kind };
    Q_ENUM(SortRole)

    explicit FolderModel(QObject *parent = nullptr);
    ~FolderModel() override;

    QUrl location() const { return m_location; }
    void setLocation(const QUrl &url);
    SortRole sortRole() const { return m_sortRole; }
    void setSortRole(SortRole r);
    Qt::SortOrder sortOrder() const { return m_sortOrder; }
    void setSortOrder(Qt::SortOrder o);
    bool foldersFirst() const { return m_foldersFirst; }
    void setFoldersFirst(bool on);
    bool showHidden() const { return m_showHidden; }
    void setShowHidden(bool on);
    QStringList filterKind() const { return m_filterKinds; }
    void setFilterKind(const QStringList &kinds);
    QString nameFilter() const { return m_nameFilter; }
    void setNameFilter(const QString &text);
    bool loading() const { return m_loading; }
    QString error() const { return m_error; }
    QString locationName() const { return m_locationName; }
    bool canGoUp() const;
    // True once the GFileMonitor watches the folder.
    bool live() const { return bool(m_monitor); }

    Q_INVOKABLE void refresh();
    Q_INVOKABLE QUrl parentLocation() const;
    Q_INVOKABLE QVariantList breadcrumbs() const;
    Q_INVOKABLE int rowOfName(const QString &name) const;

signals:
    void locationChanged();
    void sortChanged();
    void filterChanged();
    void loadingChanged();
    void errorChanged();
    void liveChanged();
    void needsMount(const QString &uri);

protected:
    void requestChildCount(Entry *e) override;
    void loaded() override;

private:
    struct CountJob;
    void startLoad();
    void stopAll();
    void beginMonitor();
    void onBatch(GList *infos);
    void nextBatch();
    void finishLoad();
    void addOrUpdate(const std::shared_ptr<Entry> &fresh, const QString &key);
    void removeKey(const QString &key);
    void touch(const QString &name);
    void flushTouched();
    void scheduleFlush();
    void flushPending();
    void rebuildView();
    void setLoading(bool on);
    void setError(const QString &msg);
    bool visible(const Entry *e) const;
    bool less(const Entry *a, const Entry *b) const;
    int cmpName(const Entry *a, const Entry *b) const;
    void prepareKey(Entry *e) const;
    void insertSorted(Entry *e);
    void removeFromView(Entry *e);
    void pumpCounts();
    void countStep(const std::shared_ptr<CountJob> &job);
    void syncStarred();
    void scheduleStats();
    QString keyFor(const Entry &e) const;
    void onMonitor(GFile *file, GFile *other, GFileMonitorEvent ev);
    static void monitorChanged(GFileMonitor *, GFile *file, GFile *other, GFileMonitorEvent ev, gpointer data);

    QUrl m_location;
    QString m_locationName;
    Gio::File m_dir;
    Gio::Cancellable m_cancel;
    Gio::Obj<GFileEnumerator> m_enum;
    Gio::Obj<GFileMonitor> m_monitor;
    quint64 m_gen = 0;
    bool m_virtualStarred = false;

    QHash<QString, std::shared_ptr<Entry>> m_all; // by child name; by URI in virtual folders
    std::vector<std::shared_ptr<Entry>> m_pending;
    QSet<QString> m_touched;
    QSet<QString> m_inheritSelection;
    QTimer m_flushTimer;
    QTimer m_touchTimer;
    QTimer m_statsTimer;
    int m_starredInflight = 0;
    qint64 m_lastFlush = 0;

    QCollator m_collator;
    SortRole m_sortRole = Name;
    Qt::SortOrder m_sortOrder = Qt::AscendingOrder;
    bool m_foldersFirst = true;
    bool m_showHidden = false;
    QStringList m_filterKinds;
    quint32 m_kindMask = 0;
    QString m_nameFilter;
    QRegularExpression m_nameRegex;
    bool m_loading = false;
    QString m_error;

    std::deque<QString> m_countQueue;
    int m_countActive = 0;
};
