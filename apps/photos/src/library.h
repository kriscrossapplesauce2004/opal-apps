#pragma once
#include "photo.h"
#include "scanner.h"

#include <QFileSystemWatcher>
#include <QFutureWatcher>
#include <QHash>
#include <QObject>
#include <QProcess>
#include <QStringList>
#include <QTimer>
#include <QUrl>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>
#include <atomic>
#include <memory>

class QQmlEngine;
class QJSEngine;

// The photo index: every picture and video under the photo folders, newest first.
// QML: singleton "PhotoLibrary".
// Properties: count, revision (changes when the list changes), loaded, scanning, scanFound, roots, excluded, skipped (software folders
//   left out), skipSoftware, includeVideos, picturesPath, sources ([{id, name, icon, count}]), screenshotCount, videoCount,
//   favouriteCount, ocrAvailable, ocrEnabled, ocrPending.
// Methods: rescan(), addRoot(urlOrPath), removeRoot(path), exclude(path), include(path), photo(path) -> map, isFavourite(path),
//   toggleFavourite(path), setFavourite(paths, on), setCaption(path, text), thumb(path) -> image://thumb URL.
// Settings live in $XDG_CONFIG_HOME/opal/photos.ini, the index in $XDG_CACHE_HOME/opal/photos/index.bin.
// $OPAL_PHOTOS_ROOTS (colon separated) replaces the roots, and $OPAL_PHOTOS_NO_OCR=1 turns OCR off; both are for tests and renders.
class PhotoLibrary : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(int count READ count NOTIFY changed)
    Q_PROPERTY(int revision READ revision NOTIFY changed)
    Q_PROPERTY(bool loaded READ loaded NOTIFY changed)
    Q_PROPERTY(bool scanning READ scanning NOTIFY scanningChanged)
    Q_PROPERTY(int scanFound READ scanFound NOTIFY scanningChanged)
    Q_PROPERTY(QStringList roots READ roots NOTIFY settingsChanged)
    Q_PROPERTY(QStringList excluded READ excluded NOTIFY settingsChanged)
    Q_PROPERTY(QStringList skipped READ skipped NOTIFY changed)
    Q_PROPERTY(bool skipSoftware READ skipSoftware WRITE setSkipSoftware NOTIFY settingsChanged)
    Q_PROPERTY(bool includeVideos READ includeVideos WRITE setIncludeVideos NOTIFY settingsChanged)
    Q_PROPERTY(QString picturesPath READ picturesPath CONSTANT)
    Q_PROPERTY(QVariantList sources READ sources NOTIFY changed)
    Q_PROPERTY(int screenshotCount READ screenshotCount NOTIFY changed)
    Q_PROPERTY(int videoCount READ videoCount NOTIFY changed)
    Q_PROPERTY(int favouriteCount READ favouriteCount NOTIFY favouritesChanged)
    Q_PROPERTY(bool ocrAvailable READ ocrAvailable CONSTANT)
    Q_PROPERTY(bool ocrEnabled READ ocrEnabled WRITE setOcrEnabled NOTIFY settingsChanged)
    Q_PROPERTY(int ocrPending READ ocrPending NOTIFY ocrChanged)

public:
    explicit PhotoLibrary(QObject *parent = nullptr);
    ~PhotoLibrary() override;
    static PhotoLibrary *instance();
    static PhotoLibrary *create(QQmlEngine *, QJSEngine *);

    int count() const { return int(m_photos.size()); }
    int revision() const { return m_revision; }
    bool loaded() const { return m_loaded; }
    bool scanning() const { return m_scanning; }
    int scanFound() const { return m_scanFound; }
    QStringList roots() const;
    QStringList excluded() const { return m_excluded; }
    QStringList skipped() const { return m_skipped; }
    bool skipSoftware() const { return m_skipSoftware; }
    void setSkipSoftware(bool on);
    bool includeVideos() const { return m_videos; }
    void setIncludeVideos(bool on);
    QString picturesPath() const;
    QVariantList sources() const;
    int screenshotCount() const { return m_screenshots; }
    int videoCount() const { return m_videoCount; }
    int favouriteCount() const;
    bool ocrAvailable() const { return !m_tesseract.isEmpty(); }
    bool ocrEnabled() const { return m_ocrEnabled && ocrAvailable(); }
    void setOcrEnabled(bool on);
    int ocrPending() const { return int(m_ocrQueue.size()); }

    const QList<Photo> &photos() const { return m_photos; }
    const Photo *find(const QString &path) const;
    int indexOf(const QString &path) const { return m_index.value(path, -1); }
    // Lower-case text that search matches against: name, folder, date words, caption, camera and OCR text.
    const QString &haystack(int i) const;
    // The device root a photo comes from, or an empty string for local folders.
    QString sourceOf(const QString &path) const;

    Q_INVOKABLE void rescan();
    Q_INVOKABLE void addRoot(const QString &urlOrPath);
    Q_INVOKABLE void removeRoot(const QString &path);
    Q_INVOKABLE void exclude(const QString &path);
    Q_INVOKABLE void include(const QString &path);
    Q_INVOKABLE QVariantMap photo(const QString &path) const;
    Q_INVOKABLE bool isFavourite(const QString &path) const;
    Q_INVOKABLE void toggleFavourite(const QString &path);
    Q_INVOKABLE void setFavourite(const QStringList &paths, bool on);
    Q_INVOKABLE void setCaption(const QString &path, const QString &caption);
    Q_INVOKABLE static QUrl thumb(const QString &path, qint64 mtime = 0);
    Q_INVOKABLE static QString uri(const QString &path);

    // Waits for the first scan; for tests only.
    bool waitForScan(int msecs = 10000);
    static QVariantMap toMap(const Photo &p);

signals:
    void changed();
    void scanningChanged();
    void settingsChanged();
    void favouritesChanged();
    void ocrChanged();
    // A caption or OCR text changed; search results may change, the layout does not.
    void textChanged(const QString &path);
    void captionFailed(const QString &path);

private:
    void loadSettings();
    void saveSettings();
    void loadCache();
    void scheduleSave();
    void saveCache();
    void startScan();
    void applyScan(const Scanner::Result &result);
    void rebuildIndex();
    void watch(const QStringList &folders);
    QStringList deviceRoots() const;
    void queueOcr();
    void runOcr();

    QList<Photo> m_photos;
    QHash<QString, int> m_index;
    mutable QHash<int, QString> m_haystack;
    QStringList m_roots;
    QStringList m_excluded;
    QStringList m_skipped;
    QStringList m_devices;
    bool m_envRoots = false;
    bool m_skipSoftware = true;
    bool m_videos = true;
    bool m_ocrEnabled = true;
    bool m_loaded = false;
    bool m_scanning = false;
    bool m_rescanPending = false;
    int m_scanFound = 0;
    int m_revision = 0;
    int m_screenshots = 0;
    int m_videoCount = 0;
    std::shared_ptr<std::atomic_bool> m_cancel;
    QFileSystemWatcher m_watcher;
    QTimer m_rescanTimer;
    QTimer m_saveTimer;
    QString m_tesseract;
    QString m_nice;
    QStringList m_ocrQueue;
    QProcess *m_ocr = nullptr;
    QString m_ocrPath;
};
