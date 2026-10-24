#pragma once
#include <QDataStream>
#include <QString>

// One indexed file: a song, an audiobook chapter or a video.
struct Track
{
    enum Kind : quint8 { Music, Book, Video };

    QString path;
    quint8 kind = Music;
    qint64 mtime = 0;
    qint64 size = 0;
    QString title;
    QString artist;
    QString album;
    QString albumArtist;
    QString genre;
    int year = 0;
    int number = 0;
    int disc = 0;
    qint64 duration = 0;
    // A cover image file: cached embedded art, or a cover image next to the file.
    QString cover;
    // Video frame size.
    int width = 0;
    int height = 0;
    // When the file was first indexed, for "Recently added".
    qint64 added = 0;

    QString folder() const { return path.section(QLatin1Char('/'), 0, -2); }
    QString fileName() const { return path.section(QLatin1Char('/'), -1); }
    // With an album artist, albums group by it; without one, by album and folder, so guest artists and compilations stay one album.
    QString albumKey() const
    {
        return albumArtist.isEmpty() ? album.toLower() + QLatin1Char('\x1f') + folder() : albumArtist.toLower() + QLatin1Char('\x1f') + album.toLower();
    }
};

inline QDataStream &operator<<(QDataStream &s, const Track &t)
{
    return s << t.path << t.kind << t.mtime << t.size << t.title << t.artist << t.album << t.albumArtist << t.genre << qint32(t.year)
             << qint32(t.number) << qint32(t.disc) << t.duration << t.cover << qint32(t.width) << qint32(t.height) << t.added;
}

inline QDataStream &operator>>(QDataStream &s, Track &t)
{
    qint32 year = 0, number = 0, disc = 0, w = 0, h = 0;
    s >> t.path >> t.kind >> t.mtime >> t.size >> t.title >> t.artist >> t.album >> t.albumArtist >> t.genre >> year >> number >> disc >> t.duration >> t.cover
        >> w >> h >> t.added;
    t.year = year;
    t.number = number;
    t.disc = disc;
    t.width = w;
    t.height = h;
    return s;
}
