#pragma once
#include <QDataStream>
#include <QString>

// One indexed photo or video. Width and height are as displayed, after the EXIF orientation.
struct Photo
{
    enum DateSource : quint8 { FromExif, FromName, FromModified };

    QString path;
    QString folder;
    qint64 mtime = 0;
    qint64 size = 0;
    int width = 0;
    int height = 0;
    // Milliseconds since the epoch; local wall-clock time of capture.
    qint64 taken = 0;
    quint8 dateSource = FromModified;
    bool video = false;
    bool screenshot = false;
    QString camera;
    QString caption;
    // Text found in a screenshot by OCR.
    QString text;
    bool textDone = false;

    double aspect() const { return height > 0 ? double(width) / height : 1.0; }
    QString name() const { return path.section(QLatin1Char('/'), -1); }
};

inline QDataStream &operator<<(QDataStream &s, const Photo &p)
{
    return s << p.path << p.folder << p.mtime << p.size << qint32(p.width) << qint32(p.height) << p.taken << p.dateSource << p.video
             << p.screenshot << p.camera << p.caption << p.text << p.textDone;
}

inline QDataStream &operator>>(QDataStream &s, Photo &p)
{
    qint32 w = 0, h = 0;
    s >> p.path >> p.folder >> p.mtime >> p.size >> w >> h >> p.taken >> p.dateSource >> p.video >> p.screenshot >> p.camera >> p.caption >> p.text
        >> p.textDone;
    p.width = w;
    p.height = h;
    return s;
}
