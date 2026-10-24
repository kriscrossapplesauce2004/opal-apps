#include "exif.h"

#include <QFile>
#include <QRegularExpression>
#include <QStringList>
#include <QTimeZone>

namespace {

class Reader
{
public:
    explicit Reader(const QByteArray &d) : m_d(d), m_le(d.startsWith("II")) {}
    bool ok(quint32 off, quint32 len) const { return off <= quint32(m_d.size()) && len <= quint32(m_d.size()) - off; }
    quint16 u16(quint32 off) const
    {
        if (!ok(off, 2))
            return 0;
        const auto *p = reinterpret_cast<const uchar *>(m_d.constData() + off);
        return m_le ? quint16(p[0] | p[1] << 8) : quint16(p[0] << 8 | p[1]);
    }
    quint32 u32(quint32 off) const
    {
        if (!ok(off, 4))
            return 0;
        const auto *p = reinterpret_cast<const uchar *>(m_d.constData() + off);
        return m_le ? quint32(p[0] | p[1] << 8 | p[2] << 16 | quint32(p[3]) << 24) : quint32(quint32(p[0]) << 24 | p[1] << 16 | p[2] << 8 | p[3]);
    }
    // Returns the offset of the value bytes of an IFD entry: inline when it fits in four bytes.
    quint32 valueOffset(quint32 entry, quint32 bytes) const { return bytes <= 4 ? entry + 8 : u32(entry + 8); }
    QString ascii(quint32 entry) const
    {
        const quint32 count = u32(entry + 4), off = valueOffset(entry, count);
        if (!ok(off, count))
            return {};
        QByteArray s = m_d.mid(off, count);
        const int nul = s.indexOf('\0');
        if (nul >= 0)
            s.truncate(nul);
        return QString::fromUtf8(s).trimmed();
    }
    double rational(quint32 off) const
    {
        const quint32 den = u32(off + 4);
        return den ? double(u32(off)) / den : 0;
    }
    double rationalEntry(quint32 entry, int index = 0) const { return rational(u32(entry + 8) + 8 * index); }
    int number(quint32 entry) const { return u16(entry + 2) == 3 ? u16(entry + 8) : int(u32(entry + 8)); }

private:
    const QByteArray &m_d;
    bool m_le;
};

QDateTime exifDate(const QString &s)
{
    // EXIF dates are "YYYY:MM:DD HH:MM:SS" in local time without a zone.
    QDateTime d = QDateTime::fromString(s.left(19), QStringLiteral("yyyy:MM:dd HH:mm:ss"));
    return d.isValid() && d.date().year() > 1900 ? d : QDateTime();
}

double degrees(const Reader &r, quint32 entry)
{
    return r.rationalEntry(entry, 0) + r.rationalEntry(entry, 1) / 60.0 + r.rationalEntry(entry, 2) / 3600.0;
}

}

ExifData Exif::parseTiff(const QByteArray &tiff)
{
    ExifData out;
    if (tiff.size() < 8 || !(tiff.startsWith("II") || tiff.startsWith("MM")))
        return out;
    Reader r(tiff);
    if (r.u16(2) != 42)
        return out;
    quint32 exifIfd = 0, gpsIfd = 0;
    QString digitized, plain;
    auto walk = [&](quint32 ifd, auto &&visit) {
        const int n = r.u16(ifd);
        if (n <= 0 || n > 512 || !r.ok(ifd + 2, quint32(n) * 12))
            return;
        for (int i = 0; i < n; ++i) {
            const quint32 e = ifd + 2 + quint32(i) * 12;
            visit(r.u16(e), e);
        }
    };
    walk(r.u32(4), [&](quint16 tag, quint32 e) {
        switch (tag) {
        case 0x010F: out.make = r.ascii(e); break;
        case 0x0110: out.model = r.ascii(e); break;
        case 0x0112: out.orientation = r.number(e); break;
        case 0x0132: plain = r.ascii(e); break;
        case 0x8769: exifIfd = r.u32(e + 8); break;
        case 0x8825: gpsIfd = r.u32(e + 8); break;
        }
    });
    if (exifIfd)
        walk(exifIfd, [&](quint16 tag, quint32 e) {
            switch (tag) {
            case 0x9003: out.taken = exifDate(r.ascii(e)); break;
            case 0x9004: digitized = r.ascii(e); break;
            case 0x829A: out.exposure = r.rationalEntry(e); break;
            case 0x829D: out.fNumber = r.rationalEntry(e); break;
            case 0x8827: out.iso = r.number(e); break;
            case 0x920A: out.focalLength = r.rationalEntry(e); break;
            case 0xA434: out.lens = r.ascii(e); break;
            }
        });
    if (gpsIfd) {
        QString latRef, lonRef;
        double lat = -1, lon = -1;
        walk(gpsIfd, [&](quint16 tag, quint32 e) {
            switch (tag) {
            case 1: latRef = r.ascii(e); break;
            case 2: lat = degrees(r, e); break;
            case 3: lonRef = r.ascii(e); break;
            case 4: lon = degrees(r, e); break;
            }
        });
        if (lat >= 0 && lon >= 0 && (lat > 0 || lon > 0)) {
            out.hasGps = true;
            out.latitude = latRef == QLatin1String("S") ? -lat : lat;
            out.longitude = lonRef == QLatin1String("W") ? -lon : lon;
        }
    }
    if (!out.taken.isValid())
        out.taken = exifDate(digitized);
    if (!out.taken.isValid())
        out.taken = exifDate(plain);
    if (out.orientation < 1 || out.orientation > 8)
        out.orientation = 1;
    return out;
}

ExifData Exif::read(const QString &path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly))
        return {};
    const QByteArray d = f.read(512 * 1024);
    auto u32be = [&](qsizetype o) { return o + 4 <= d.size() ? quint32(uchar(d[o])) << 24 | uchar(d[o + 1]) << 16 | uchar(d[o + 2]) << 8 | uchar(d[o + 3]) : 0u; };
    if (d.startsWith("\xFF\xD8")) {
        qsizetype o = 2;
        while (o + 4 <= d.size() && uchar(d[o]) == 0xFF) {
            const uchar marker = uchar(d[o + 1]);
            const int len = uchar(d[o + 2]) << 8 | uchar(d[o + 3]);
            if (marker == 0xDA || marker == 0xD9 || len < 2)
                break;
            if (marker == 0xE1 && d.mid(o + 4, 6) == QByteArray("Exif\0\0", 6))
                return parseTiff(d.mid(o + 10, len - 8));
            o += 2 + len;
        }
    } else if (d.startsWith("\x89PNG\r\n\x1a\n")) {
        qsizetype o = 8;
        while (o + 8 <= d.size()) {
            const quint32 len = u32be(o);
            const QByteArray type = d.mid(o + 4, 4);
            if (type == "eXIf")
                return parseTiff(d.mid(o + 8, len));
            if (type == "IDAT" || type == "IEND")
                break;
            o += 12 + qsizetype(len);
        }
    } else if (d.startsWith("RIFF") && d.mid(8, 4) == "WEBP") {
        qsizetype o = 12;
        while (o + 8 <= d.size()) {
            const quint32 len = uchar(d[o + 4]) | uchar(d[o + 5]) << 8 | uchar(d[o + 6]) << 16 | quint32(uchar(d[o + 7])) << 24;
            if (d.mid(o, 4) == "EXIF") {
                QByteArray block = d.mid(o + 8, len);
                if (block.startsWith(QByteArray("Exif\0\0", 6)))
                    block.remove(0, 6);
                return parseTiff(block);
            }
            o += 8 + qsizetype(len) + (len & 1);
        }
    }
    return {};
}

QDateTime Exif::dateFromName(const QString &fileName)
{
    // Year, month, day, then an optional time with any of the separators that cameras and screenshot tools use.
    static const QRegularExpression re(QStringLiteral("(?<![0-9])((?:19|20)[0-9]{2})[-_.]?([01][0-9])[-_.]?([0-3][0-9])(?:[-_ T.]{0,3}([0-2][0-9])[-_:.h]?([0-5][0-9])[-_:.m]?([0-5][0-9]))?"));
    auto it = re.globalMatch(fileName);
    while (it.hasNext()) {
        const auto m = it.next();
        const QDate date(m.captured(1).toInt(), m.captured(2).toInt(), m.captured(3).toInt());
        if (!date.isValid())
            continue;
        QTime time(0, 0);
        if (!m.captured(4).isEmpty()) {
            const QTime t(m.captured(4).toInt(), m.captured(5).toInt(), m.captured(6).toInt());
            if (t.isValid())
                time = t;
        }
        return QDateTime(date, time);
    }
    return {};
}

QString ExifData::camera() const
{
    // Many cameras repeat the make inside the model ("Canon" + "Canon EOS R6").
    if (model.isEmpty())
        return make;
    if (make.isEmpty() || model.startsWith(make.section(QLatin1Char(' '), 0, 0), Qt::CaseInsensitive))
        return model;
    return make + QLatin1Char(' ') + model;
}

QString ExifData::summary() const
{
    QStringList parts;
    if (fNumber > 0)
        parts << QStringLiteral("f/%1").arg(fNumber, 0, 'g', 2);
    if (exposure > 0)
        parts << (exposure < 1 ? QStringLiteral("1/%1 s").arg(qRound(1 / exposure)) : QStringLiteral("%1 s").arg(exposure, 0, 'g', 3));
    if (iso > 0)
        parts << QStringLiteral("ISO %1").arg(iso);
    if (focalLength > 0)
        parts << QStringLiteral("%1 mm").arg(focalLength, 0, 'g', 3);
    return parts.join(QStringLiteral(" · "));
}
