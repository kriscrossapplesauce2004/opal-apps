#pragma once
#include <QByteArray>
#include <QDateTime>
#include <QString>

// The few EXIF fields Photos shows or sorts by.
struct ExifData
{
    QDateTime taken;
    int orientation = 1;
    QString make;
    QString model;
    QString lens;
    double exposure = 0;
    double fNumber = 0;
    double focalLength = 0;
    int iso = 0;
    bool hasGps = false;
    double latitude = 0;
    double longitude = 0;

    bool isEmpty() const { return !taken.isValid() && make.isEmpty() && model.isEmpty(); }
    // Returns "Pixel 8 · f/1.7 · 1/120 s · ISO 50 · 6.9 mm" with the parts that are known.
    QString summary() const;
    QString camera() const;
};

namespace Exif {
// Reads EXIF from JPEG (APP1), PNG (eXIf) and WebP (EXIF chunk). Reads at most the first 512 KiB. Blocking.
ExifData read(const QString &path);
// Parses a raw TIFF block, starting at the "II" or "MM" byte order mark.
ExifData parseTiff(const QByteArray &tiff);
// Finds a date and time in names like shot-20261008-182941, IMG_20261008_182941 or Screenshot_2026-10-08_18-29-41.
QDateTime dateFromName(const QString &fileName);
}
