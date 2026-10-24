#include "imageio.h"

#include <QBuffer>
#include <QFile>
#include <QFileInfo>
#include <QImageReader>
#include <QImageWriter>
#include <QSaveFile>
#include <QTransform>

namespace {

// Finds the JPEG APP1 Exif segment. Returns the offset of the segment marker, or -1.
qsizetype findExif(const QByteArray &d, int *length)
{
    if (!d.startsWith("\xFF\xD8"))
        return -1;
    qsizetype o = 2;
    while (o + 4 <= d.size() && uchar(d[o]) == 0xFF) {
        const uchar marker = uchar(d[o + 1]);
        const int len = uchar(d[o + 2]) << 8 | uchar(d[o + 3]);
        if (marker == 0xDA || marker == 0xD9 || len < 2)
            break;
        if (marker == 0xE1 && d.mid(o + 4, 6) == QByteArray("Exif\0\0", 6)) {
            *length = len;
            return o;
        }
        o += 2 + len;
    }
    return -1;
}

// Returns the file offset of the orientation value in IFD0 of an Exif segment that starts at seg, or -1.
qsizetype orientationOffset(const QByteArray &d, qsizetype seg, int length)
{
    const qsizetype tiff = seg + 10;
    if (tiff + 8 > d.size())
        return -1;
    const bool le = d.mid(tiff, 2) == "II";
    auto u16 = [&](qsizetype o) -> quint16 { return o + 2 > d.size() ? 0 : le ? quint16(uchar(d[o]) | uchar(d[o + 1]) << 8) : quint16(uchar(d[o]) << 8 | uchar(d[o + 1])); };
    auto u32 = [&](qsizetype o) -> quint32 {
        if (o + 4 > d.size())
            return 0;
        const quint32 a = uchar(d[o]), b = uchar(d[o + 1]), c = uchar(d[o + 2]), e = uchar(d[o + 3]);
        return le ? a | b << 8 | c << 16 | e << 24 : a << 24 | b << 16 | c << 8 | e;
    };
    const qsizetype ifd = tiff + u32(tiff + 4);
    const int n = u16(ifd);
    if (n <= 0 || ifd + 2 + n * 12 > seg + 2 + length)
        return -1;
    for (int i = 0; i < n; ++i) {
        const qsizetype e = ifd + 2 + i * 12;
        if (u16(e) == 0x0112)
            return e + 8;
    }
    return -1;
}

void writeU16(QByteArray &d, qsizetype o, quint16 v, bool le)
{
    d[o] = char(le ? v & 0xff : v >> 8);
    d[o + 1] = char(le ? v >> 8 : v & 0xff);
}

bool isJpeg(const QString &path)
{
    const QString s = QFileInfo(path).suffix().toLower();
    return s == QLatin1String("jpg") || s == QLatin1String("jpeg");
}

// EXIF orientation as (mirror, clockwise degrees applied after the mirror).
constexpr int Mirror[9] = {0, 0, 1, 0, 1, 1, 0, 1, 0};
constexpr int Turn[9] = {0, 0, 0, 180, 180, 270, 90, 90, 270};

}

namespace ImageIO {

QImage loadOriented(const QString &path, QString *error)
{
    QImageReader r(path);
    r.setAutoTransform(true);
    // Camera photos pass the default 256 MB decode limit.
    QImageReader::setAllocationLimit(2048);
    QImage img = r.read();
    if (img.isNull() && error)
        *error = r.errorString();
    return img;
}

int composeOrientation(int orientation, int quarterTurns)
{
    if (orientation < 1 || orientation > 8)
        orientation = 1;
    const int mirror = Mirror[orientation];
    const int turn = ((Turn[orientation] + quarterTurns * 90) % 360 + 360) % 360;
    for (int o = 1; o <= 8; ++o)
        if (Mirror[o] == mirror && Turn[o] == turn)
            return o;
    return 1;
}

int jpegOrientation(const QString &path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly))
        return 0;
    const QByteArray d = f.read(256 * 1024);
    int len = 0;
    const qsizetype seg = findExif(d, &len);
    if (seg < 0)
        return 0;
    const qsizetype o = orientationOffset(d, seg, len);
    if (o < 0)
        return 0;
    const bool le = d.mid(seg + 10, 2) == "II";
    return le ? uchar(d[o]) | uchar(d[o + 1]) << 8 : uchar(d[o]) << 8 | uchar(d[o + 1]);
}

bool setJpegOrientation(const QString &path, int orientation)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadWrite))
        return false;
    QByteArray d = f.read(256 * 1024);
    int len = 0;
    const qsizetype seg = findExif(d, &len);
    if (seg < 0)
        return false;
    const qsizetype o = orientationOffset(d, seg, len);
    if (o < 0)
        return false;
    writeU16(d, o, quint16(orientation), d.mid(seg + 10, 2) == "II");
    if (!f.seek(o))
        return false;
    return f.write(d.mid(o, 2)) == 2;
}

bool save(const QImage &image, const QString &dest, const QString &original, QString *error)
{
    QByteArray format = QFileInfo(dest).suffix().toLower().toLatin1();
    if (format == "jpg")
        format = "jpeg";
    QByteArray bytes;
    {
        QBuffer buffer(&bytes);
        buffer.open(QIODevice::WriteOnly);
        QImageWriter w(&buffer, format);
        if (format == "jpeg" || format == "webp")
            w.setQuality(93);
        const QImage out = format == "jpeg" ? image.convertToFormat(QImage::Format_RGB888) : image;
        if (!w.write(out)) {
            if (error)
                *error = w.errorString();
            return false;
        }
    }
    if (format == "jpeg" && isJpeg(original)) {
        QFile o(original);
        if (o.open(QIODevice::ReadOnly)) {
            QByteArray src = o.read(256 * 1024);
            int len = 0;
            const qsizetype seg = findExif(src, &len);
            if (seg >= 0) {
                // The pixels are already upright, so the copied EXIF must say "normal".
                const qsizetype oo = orientationOffset(src, seg, len);
                if (oo >= 0)
                    writeU16(src, oo, 1, src.mid(seg + 10, 2) == "II");
                bytes.insert(2, src.mid(seg, 2 + len));
            }
        }
    }
    QSaveFile f(dest);
    if (!f.open(QIODevice::WriteOnly) || f.write(bytes) != bytes.size() || !f.commit()) {
        if (error)
            *error = f.errorString();
        return false;
    }
    return true;
}

bool rotate(const QString &path, int quarterTurns, QString *error)
{
    quarterTurns = ((quarterTurns % 4) + 4) % 4;
    if (quarterTurns == 0)
        return true;
    if (isJpeg(path)) {
        const int current = jpegOrientation(path);
        if (current > 0)
            return setJpegOrientation(path, composeOrientation(current, quarterTurns));
    }
    const QImage img = loadOriented(path, error);
    if (img.isNull())
        return false;
    return save(img.transformed(QTransform().rotate(90 * quarterTurns)), path, path, error);
}

QString copyName(const QString &path)
{
    const QFileInfo fi(path);
    const QString base = fi.completeBaseName(), suffix = fi.suffix().isEmpty() ? QString() : QLatin1Char('.') + fi.suffix();
    for (int n = 1;; ++n) {
        const QString name = n == 1 ? base + QStringLiteral("-edited") + suffix : base + QStringLiteral("-edited-%1").arg(n) + suffix;
        const QString candidate = fi.absolutePath() + QLatin1Char('/') + name;
        if (!QFileInfo::exists(candidate))
            return candidate;
    }
}

}
