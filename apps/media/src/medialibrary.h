#pragma once
#include "track.h"

#include <QFileSystemWatcher>
#include <QHash>
#include <QSet>
#include <QObject>
#include <QStringList>
#include <QTimer>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>
#include <atomic>
#include <functional>
#include <memory>

class QQmlEngine;
class QJSEngine;

// The media index: songs under the music folders, audiobooks under the book folders, and videos.
// QML: singleton "MediaLibrary".
// Properties: count, songCount, albumCount, videoCount, bookCount, revision, loaded, scanning, scanFound, musicRoots, bookRoots,
//   videoRoots, genres, skipped, recentAlbums (album keys played lately).
// Music: albums(query, genre, sort) -> [{key, title, artist, year, cover, count, duration, genre, added}], album(key) -> {…, tracks},
//   artists(query) -> [{name, albums, songs, cover}], artist(name) -> {name, albums, tracks}, songs(query, sort) -> [track maps],
//   playlists() -> [{name, path, count}], playlist(path) -> tracks, createPlaylist(name, paths), addToPlaylist(path, paths),
//   removeFromPlaylist(path, index), deletePlaylist(path), liked(path), toggleLiked(path), likedSongs().
// Books: books() -> [{key, title, author, cover, chapters, duration, position, chapter, finished}], book(key) -> {…, tracks},
//   setBookProgress(key, chapter, positionMs).
// Videos: videos(query) -> [{path, title, folder, duration, width, height, thumb, position, added}], setVideoPosition(path, ms).
// Other: track(path) -> map, search(query) -> {albums, artists, songs, videos, books}, noteAlbumPlayed(key), rescan(), addRoot(kind, url), removeRoot(kind, path).
// $OPAL_MEDIA_MUSIC, $OPAL_MEDIA_BOOKS and $OPAL_MEDIA_VIDEOS (colon separated) replace the roots, for tests and renders.
class MediaLibrary : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(int count READ count NOTIFY changed)
    Q_PROPERTY(int songCount READ songCount NOTIFY changed)
    Q_PROPERTY(int albumCount READ albumCount NOTIFY changed)
    Q_PROPERTY(int videoCount READ videoCount NOTIFY changed)
    Q_PROPERTY(int bookCount READ bookCount NOTIFY changed)
    Q_PROPERTY(int revision READ revision NOTIFY changed)
    Q_PROPERTY(bool loaded READ loaded NOTIFY changed)
    Q_PROPERTY(bool scanning READ scanning NOTIFY scanningChanged)
    Q_PROPERTY(int scanFound READ scanFound NOTIFY scanningChanged)
    Q_PROPERTY(QStringList musicRoots READ musicRoots NOTIFY settingsChanged)
    Q_PROPERTY(QStringList bookRoots READ bookRoots NOTIFY settingsChanged)
    Q_PROPERTY(QStringList videoRoots READ videoRoots NOTIFY settingsChanged)
    Q_PROPERTY(QStringList genres READ genres NOTIFY changed)
    Q_PROPERTY(QStringList skipped READ skipped NOTIFY changed)
    Q_PROPERTY(QStringList recentAlbums READ recentAlbums NOTIFY historyChanged)
    Q_PROPERTY(int likedRevision READ likedRevision NOTIFY likedChanged)
    Q_PROPERTY(int progressRevision READ progressRevision NOTIFY progressChanged)

public:
    struct Options
    {
        QStringList music, books, videos;
    };
    struct Result
    {
        QList<Track> tracks;
        QStringList folders;
        QStringList skipped;
    };

    explicit MediaLibrary(QObject *parent = nullptr);
    ~MediaLibrary() override;
    static MediaLibrary *instance();
    static MediaLibrary *create(QQmlEngine *, QJSEngine *);

    int count() const { return int(m_tracks.size()); }
    int songCount() const { return m_songs; }
    int albumCount() const { return int(m_albumOrder.size()); }
    int videoCount() const { return m_videos; }
    int bookCount() const { return int(m_bookOrder.size()); }
    int revision() const { return m_revision; }
    bool loaded() const { return m_loaded; }
    bool scanning() const { return m_scanning; }
    int scanFound() const { return m_scanFound; }
    QStringList musicRoots() const { return m_music; }
    QStringList bookRoots() const { return m_books; }
    QStringList videoRoots() const { return m_videoRoots; }
    QStringList genres() const { return m_genres; }
    QStringList skipped() const { return m_skipped; }
    QStringList recentAlbums() const { return m_recent; }
    int likedRevision() const { return m_likedRevision; }
    int progressRevision() const { return m_progressRevision; }
    const QList<Track> &tracks() const { return m_tracks; }
    const Track *find(const QString &path) const;

    Q_INVOKABLE QVariantList albums(const QString &query = {}, const QString &genre = {}, const QString &sort = QStringLiteral("added")) const;
    Q_INVOKABLE QVariantMap album(const QString &key) const;
    Q_INVOKABLE QVariantList artists(const QString &query = {}) const;
    Q_INVOKABLE QVariantMap artist(const QString &name) const;
    Q_INVOKABLE QVariantList songs(const QString &query = {}, const QString &sort = QStringLiteral("title")) const;
    Q_INVOKABLE QVariantList playlists() const;
    Q_INVOKABLE QVariantList playlist(const QString &path) const;
    Q_INVOKABLE QString createPlaylist(const QString &name, const QStringList &paths = {});
    Q_INVOKABLE bool addToPlaylist(const QString &path, const QStringList &paths);
    Q_INVOKABLE bool removeFromPlaylist(const QString &path, int index);
    Q_INVOKABLE bool deletePlaylist(const QString &path);
    Q_INVOKABLE bool liked(const QString &path) const { return m_liked.contains(path); }
    Q_INVOKABLE void toggleLiked(const QString &path);
    Q_INVOKABLE QVariantList likedSongs() const;
    Q_INVOKABLE QVariantList books() const;
    Q_INVOKABLE QVariantMap book(const QString &key) const;
    Q_INVOKABLE void setBookProgress(const QString &key, int chapter, qint64 position);
    Q_INVOKABLE QVariantList videos(const QString &query = {}) const;
    Q_INVOKABLE void setVideoPosition(const QString &path, qint64 position);
    Q_INVOKABLE QVariantMap track(const QString &path) const;
    Q_INVOKABLE QVariantMap search(const QString &query) const;
    Q_INVOKABLE void noteAlbumPlayed(const QString &key);
    Q_INVOKABLE void rescan();
    Q_INVOKABLE void addRoot(const QString &kind, const QString &urlOrPath);
    Q_INVOKABLE void removeRoot(const QString &kind, const QString &path);
    Q_INVOKABLE static QString formatDuration(qint64 ms);

    QVariantMap toMap(const Track &t) const;
    bool waitForScan(int msecs = 15000);

    // Scanning, public for tests. Pure: no Qt objects, safe on a worker thread.
    static Result scan(const Options &options, const QHash<QString, Track> &cache, const QString &coverDir, const std::atomic_bool *cancel = nullptr,
                       const std::function<void(int)> &progress = {});
    // single: a loose file at the top of a music folder, which has no album art of its own folder.
    static bool probeAudio(const QString &path, Track &out, const QString &coverDir, bool single = false);
    static bool probeVideo(const QString &path, Track &out, const QString &thumbDir);
    static bool isAudioName(const QString &name);
    static bool isVideoName(const QString &name);
    static bool isSoftwareFolder(const QString &folder);
    // A cover image next to the file: cover.jpg, folder.png, front.jpg, or the only image in the folder.
    static QString folderCover(const QString &folder);
    static QString coverDir();
    static QString thumbDir();

signals:
    void changed();
    void scanningChanged();
    void settingsChanged();
    void historyChanged();
    void likedChanged();
    void progressChanged();

private:
    struct AlbumInfo
    {
        QString key, title, artist, cover, genre;
        int year = 0;
        qint64 duration = 0;
        qint64 added = 0;
        QList<int> tracks;
    };
    struct BookInfo
    {
        QString key, title, author, cover;
        qint64 duration = 0;
        QList<int> tracks;
    };
    void loadSettings();
    void saveSettings();
    void loadCache();
    void saveCache();
    void startScan();
    void apply(const Result &r);
    void rebuild();
    QVariantMap albumMap(const AlbumInfo &a, bool withTracks) const;
    QVariantMap bookMap(const BookInfo &b, bool withTracks) const;
    QVariantList trackList(const QList<int> &indices) const;
    bool matches(const Track &t, const QString &q) const;

    QList<Track> m_tracks;
    QHash<QString, int> m_index;
    QHash<QString, AlbumInfo> m_albums;
    QStringList m_albumOrder;
    QHash<QString, BookInfo> m_bookInfo;
    QStringList m_bookOrder;
    QStringList m_genres;
    QStringList m_skipped;
    QStringList m_recent;
    QStringList m_music, m_books, m_videoRoots;
    QSet<QString> m_liked;
    bool m_envRoots = false;
    bool m_loaded = false;
    bool m_scanning = false;
    bool m_rescanPending = false;
    int m_scanFound = 0;
    int m_revision = 0;
    int m_likedRevision = 0;
    int m_progressRevision = 0;
    int m_songs = 0;
    int m_videos = 0;
    std::shared_ptr<std::atomic_bool> m_cancel;
    QFileSystemWatcher m_watcher;
    QTimer m_rescanTimer;
    QTimer m_saveTimer;
};
