#include "photodetails.h"
#include "exif.h"
#include "format.h"
#include "library.h"
#include "thumbnailer.h"

#include <QDir>
#include <QHash>
#include <QImage>
#include <QLocale>
#include <QtConcurrent>
#include <algorithm>

PhotoDetails::PhotoDetails(QObject *parent)
    : QObject(parent)
{
    connect(PhotoLibrary::instance(), &PhotoLibrary::textChanged, this, [this](const QString &p) {
        if (p != m_path)
            return;
        const Photo *photo = PhotoLibrary::instance()->find(p);
        if (photo) {
            m_caption = photo->caption;
            m_text = photo->text;
            emit loaded();
        }
    });
}

void PhotoDetails::setPath(const QString &p)
{
    if (m_path == p)
        return;
    m_path = p;
    emit pathChanged();
    load();
}

QString PhotoDetails::folderDisplay() const
{
    const QString f = folder(), home = QDir::homePath();
    return f.startsWith(home) ? QStringLiteral("~") + f.mid(home.size()) : f;
}

QString PhotoDetails::dateText() const
{
    return m_taken.isValid() ? QLocale().toString(m_taken.date(), QStringLiteral("d MMM yyyy")) : QString();
}

QString PhotoDetails::timeText() const
{
    return m_taken.isValid() ? QLocale().toString(m_taken, QStringLiteral("dddd, HH:mm")) : QString();
}

QString PhotoDetails::locationText() const
{
    if (!m_hasLocation)
        return {};
    return QStringLiteral("%1° %2, %3° %4")
        .arg(std::abs(m_lat), 0, 'f', 4)
        .arg(m_lat >= 0 ? QLatin1Char('N') : QLatin1Char('S'))
        .arg(std::abs(m_lon), 0, 'f', 4)
        .arg(m_lon >= 0 ? QLatin1Char('E') : QLatin1Char('W'));
}

QList<QColor> PhotoDetails::dominantColours(const QImage &source, int count)
{
    if (source.isNull())
        return {};
    const QImage img = source.convertToFormat(QImage::Format_RGB32).scaled(64, 64, Qt::KeepAspectRatio, Qt::FastTransformation);
    // A 4-bit-per-channel histogram, then the average colour of each of the biggest buckets.
    struct Bucket { int n = 0; qint64 r = 0, g = 0, b = 0; };
    QHash<int, Bucket> buckets;
    for (int y = 0; y < img.height(); ++y) {
        const QRgb *line = reinterpret_cast<const QRgb *>(img.constScanLine(y));
        for (int x = 0; x < img.width(); ++x) {
            const QRgb c = line[x];
            Bucket &b = buckets[(qRed(c) >> 4) << 8 | (qGreen(c) >> 4) << 4 | qBlue(c) >> 4];
            ++b.n;
            b.r += qRed(c);
            b.g += qGreen(c);
            b.b += qBlue(c);
        }
    }
    QList<Bucket> list = buckets.values();
    std::sort(list.begin(), list.end(), [](const Bucket &a, const Bucket &b) { return a.n > b.n; });
    QList<QColor> out;
    for (const Bucket &b : list) {
        const QColor c(int(b.r / b.n), int(b.g / b.n), int(b.b / b.n));
        bool near = false;
        for (const QColor &o : out) {
            const int d = std::abs(c.red() - o.red()) + std::abs(c.green() - o.green()) + std::abs(c.blue() - o.blue());
            if (d < 90) {
                near = true;
                break;
            }
        }
        if (!near)
            out << c;
        if (out.size() >= count)
            break;
    }
    return out;
}

void PhotoDetails::load()
{
    const int generation = ++m_generation;
    m_ready = false;
    m_colours.clear();
    const Photo *p = PhotoLibrary::instance()->find(m_path);
    m_taken = p ? QDateTime::fromMSecsSinceEpoch(p->taken) : QDateTime();
    m_dateNote = !p || p->dateSource == Photo::FromExif ? QString() : p->dateSource == Photo::FromName ? tr("Date from the file name") : tr("Date from the file");
    m_dimensions = p ? QStringLiteral("%1 × %2").arg(p->width).arg(p->height) : QString();
    m_megapixels = p && !p->video && p->width * qint64(p->height) >= 1'000'000 ? QStringLiteral("%1 MP").arg(p->width * double(p->height) / 1e6, 0, 'f', 1) : QString();
    m_sizeText = p ? Fmt::humanSize(p->size) : QString();
    m_caption = p ? p->caption : QString();
    m_text = p ? p->text : QString();
    m_video = p && p->video;
    m_camera = p ? p->camera : QString();
    m_lens.clear();
    m_exposure.clear();
    m_hasLocation = false;
    emit loaded();
    emit coloursChanged();
    if (m_path.isEmpty())
        return;
    const QString path = m_path;
    struct Result { ExifData exif; QList<QColor> colours; };
    QtConcurrent::run([path] {
        Result r;
        r.exif = Exif::read(path);
        r.colours = dominantColours(Thumbnailer::instance()->loadOrGenerate(PhotoLibrary::uri(path), 128));
        return r;
    }).then(this, [this, generation](const Result &r) {
        if (generation != m_generation)
            return;
        if (!r.exif.camera().isEmpty())
            m_camera = r.exif.camera();
        m_lens = r.exif.lens;
        m_exposure = r.exif.summary();
        m_hasLocation = r.exif.hasGps;
        m_lat = r.exif.latitude;
        m_lon = r.exif.longitude;
        for (const QColor &c : r.colours)
            m_colours << c;
        m_ready = true;
        emit loaded();
        emit coloursChanged();
    });
}
