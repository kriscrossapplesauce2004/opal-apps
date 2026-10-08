#pragma once
#include <QDateTime>
#include <QLocale>
#include <QString>

// Text formatting and kind classification shared by every Files model.
namespace Fmt {

// Kinds of entries, from the content type.
enum class Kind : quint8 { Folder, Image, Video, Audio, Pdf, Archive, Text, Code, App, Other };

// Returns "4.8 MB", "880 KB", "2 KB". Units are decimal (1000), as in the design fixtures.
QString humanSize(qint64 bytes, const QLocale &loc = QLocale());

// Returns "Today 14:02", "Yesterday", "Mon 22:41", "2 Oct" or "27 Sep 2024".
QString humanDate(const QDateTime &when, const QDateTime &now, const QLocale &loc = QLocale());

// Returns "1 item", "38 items".
QString itemCount(int n, const QLocale &loc = QLocale());

// Returns "40 s left", "2 min left", "1 h 5 min left".
QString humanEta(qint64 seconds);

Kind kindFor(const QString &contentType, bool isDir);
QString kindString(Kind k);
Kind kindFromString(const QString &s, bool *ok = nullptr);

// Returns a freedesktop icon name for the content type, cached.
QString iconNameFor(const QString &contentType, bool isDir);
// Returns the localized description of a content type, cached.
QString kindDescription(const QString &contentType, bool isDir);

}
