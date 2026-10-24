#include "editor.h"
#include "imageio.h"
#include "library.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QPainter>
#include <QPainterPath>
#include <QTransform>
#include <QtConcurrent>
#include <algorithm>
#include <cmath>
#include <numeric>

namespace {

const char *const Ids[] = {"brightness", "contrast", "saturation", "warmth", "tint", "highlights", "shadows", "vignette", "fade"};

struct Filter
{
    const char *id;
    const char *name;
    QVariantMap preset;
};

const QList<Filter> &filterTable()
{
    using M = QVariantMap;
    static const QList<Filter> table{
        {"none", QT_TRANSLATE_NOOP("PhotoEditor", "Original"), M{}},
        {"vivid", QT_TRANSLATE_NOOP("PhotoEditor", "Vivid"), M{{"saturation", .35}, {"contrast", .12}}},
        {"warm", QT_TRANSLATE_NOOP("PhotoEditor", "Warm"), M{{"warmth", .45}, {"saturation", .08}}},
        {"cool", QT_TRANSLATE_NOOP("PhotoEditor", "Cool"), M{{"warmth", -.45}, {"tint", -.1}}},
        {"film", QT_TRANSLATE_NOOP("PhotoEditor", "Film"), M{{"warmth", .18}, {"contrast", .08}, {"fade", .06}, {"saturation", -.1}, {"vignette", .2}}},
        {"fade", QT_TRANSLATE_NOOP("PhotoEditor", "Faded"), M{{"contrast", -.2}, {"fade", .12}, {"saturation", -.15}}},
        {"dramatic", QT_TRANSLATE_NOOP("PhotoEditor", "Dramatic"), M{{"contrast", .3}, {"highlights", -.3}, {"shadows", .2}, {"saturation", .1}, {"vignette", .3}}},
        {"mono", QT_TRANSLATE_NOOP("PhotoEditor", "Mono"), M{{"saturation", -1}, {"contrast", .1}}},
        {"noir", QT_TRANSLATE_NOOP("PhotoEditor", "Noir"), M{{"saturation", -1}, {"contrast", .4}, {"shadows", -.25}, {"vignette", .35}}},
    };
    return table;
}

float smoothstep(float a, float b, float x)
{
    const float t = std::clamp((x - a) / (b - a), 0.f, 1.f);
    return t * t * (3 - 2 * t);
}

}

PhotoEditor::PhotoEditor(QObject *parent)
    : QObject(parent)
{
    reset();
}

void PhotoEditor::setPath(const QString &p)
{
    if (m_path == p)
        return;
    m_path = p;
    emit pathChanged();
    reset();
}

QVariantList PhotoEditor::filters()
{
    QVariantList out;
    for (const Filter &f : filterTable())
        out << QVariantMap{{QStringLiteral("id"), QString::fromLatin1(f.id)}, {QStringLiteral("name"), tr(f.name)}};
    return out;
}

QVariantList PhotoEditor::adjustments()
{
    auto a = [](const char *id, const QString &name, const char *icon) {
        return QVariantMap{{QStringLiteral("id"), QString::fromLatin1(id)}, {QStringLiteral("name"), name}, {QStringLiteral("icon"), QString::fromLatin1(icon)}};
    };
    return {a("brightness", tr("Brightness"), "brightness_6"), a("contrast", tr("Contrast"), "contrast"), a("highlights", tr("Highlights"), "highlight"),
            a("shadows", tr("Shadows"), "shadow"), a("saturation", tr("Saturation"), "palette"), a("warmth", tr("Warmth"), "thermostat"),
            a("tint", tr("Tint"), "colorize"), a("vignette", tr("Vignette"), "vignette"), a("fade", tr("Fade"), "gradient")};
}

QVariantMap PhotoEditor::effective() const
{
    QVariantMap out;
    QVariantMap preset;
    for (const Filter &f : filterTable())
        if (m_filter == QLatin1String(f.id))
            preset = f.preset;
    for (const char *id : Ids) {
        const QString k = QString::fromLatin1(id);
        const double lo = k == QLatin1String("fade") ? 0 : -1;
        out.insert(k, std::clamp(m_values.value(k).toDouble() + preset.value(k).toDouble(), lo, 1.0));
    }
    return out;
}

void PhotoEditor::setFilter(const QString &f)
{
    if (m_filter == f)
        return;
    m_filter = f;
    emit valuesChanged();
    emit dirtyChanged();
}

void PhotoEditor::setValue(const QString &id, qreal v)
{
    const double lo = id == QLatin1String("fade") ? 0 : -1;
    v = std::clamp(v, lo, 1.0);
    if (qFuzzyCompare(m_values.value(id).toDouble() + 2, v + 2))
        return;
    m_values.insert(id, v);
    emit valuesChanged();
    emit dirtyChanged();
}

void PhotoEditor::setQuarterTurns(int t)
{
    t = ((t % 4) + 4) % 4;
    if (m_turns == t)
        return;
    // A quarter turn swaps the frame axes, so the crop turns with the picture.
    const int delta = ((t - m_turns) % 4 + 4) % 4;
    QRectF c = m_crop;
    for (int i = 0; i < delta; ++i)
        c = QRectF(1 - c.y() - c.height(), c.x(), c.height(), c.width());
    m_crop = c;
    m_turns = t;
    emit geometryChanged();
    emit dirtyChanged();
}

void PhotoEditor::setFlipped(bool f)
{
    if (m_flipped == f)
        return;
    m_flipped = f;
    m_crop = QRectF(1 - m_crop.x() - m_crop.width(), m_crop.y(), m_crop.width(), m_crop.height());
    emit geometryChanged();
    emit dirtyChanged();
}

void PhotoEditor::setStraighten(qreal d)
{
    d = std::clamp(d, -45.0, 45.0);
    if (qFuzzyCompare(m_straighten + 100, d + 100))
        return;
    m_straighten = d;
    emit geometryChanged();
    emit dirtyChanged();
}

void PhotoEditor::setCrop(const QRectF &r)
{
    const QRectF c = r.normalized().intersected(QRectF(0, 0, 1, 1));
    if (c == m_crop || c.width() < .02 || c.height() < .02)
        return;
    m_crop = c;
    emit geometryChanged();
    emit dirtyChanged();
}

void PhotoEditor::setStrokes(const QVariantList &s)
{
    m_strokes = s;
    emit strokesChanged();
    emit dirtyChanged();
}

void PhotoEditor::undoStroke()
{
    if (m_strokes.isEmpty())
        return;
    m_strokes.removeLast();
    emit strokesChanged();
    emit dirtyChanged();
}

bool PhotoEditor::dirty() const
{
    if (m_filter != QLatin1String("none") || m_turns || m_flipped || !qFuzzyIsNull(m_straighten) || m_crop != QRectF(0, 0, 1, 1) || !m_strokes.isEmpty())
        return true;
    for (const QVariant &v : m_values)
        if (!qFuzzyIsNull(v.toDouble()))
            return true;
    return false;
}

void PhotoEditor::reset()
{
    m_values.clear();
    for (const char *id : Ids)
        m_values.insert(QString::fromLatin1(id), 0.0);
    m_filter = QStringLiteral("none");
    m_turns = 0;
    m_flipped = false;
    m_straighten = 0;
    m_crop = QRectF(0, 0, 1, 1);
    m_strokes.clear();
    emit valuesChanged();
    emit geometryChanged();
    emit strokesChanged();
    emit dirtyChanged();
}

void PhotoEditor::setBusy(bool b)
{
    if (m_busy != b) {
        m_busy = b;
        emit busyChanged();
    }
}

qreal PhotoEditor::coverScale(qreal w, qreal h, qreal degrees)
{
    if (w <= 0 || h <= 0)
        return 1;
    const qreal a = std::abs(degrees) * M_PI / 180, c = std::cos(a), s = std::sin(a);
    return std::max((w * c + h * s) / w, (w * s + h * c) / h);
}

PhotoEditor::Params PhotoEditor::paramsFrom(const QVariantMap &e)
{
    Params p;
    p.brightness = e.value(QStringLiteral("brightness")).toFloat();
    p.contrast = e.value(QStringLiteral("contrast")).toFloat();
    p.saturation = e.value(QStringLiteral("saturation")).toFloat();
    p.warmth = e.value(QStringLiteral("warmth")).toFloat();
    p.tint = e.value(QStringLiteral("tint")).toFloat();
    p.highlights = e.value(QStringLiteral("highlights")).toFloat();
    p.shadows = e.value(QStringLiteral("shadows")).toFloat();
    p.vignette = e.value(QStringLiteral("vignette")).toFloat();
    p.fade = e.value(QStringLiteral("fade")).toFloat();
    return p;
}

void PhotoEditor::adjustPixels(QImage &image, const Params &p)
{
    if (image.format() != QImage::Format_ARGB32)
        image = image.convertToFormat(QImage::Format_ARGB32);
    const int w = image.width(), h = image.height();
    QList<int> rows(h);
    std::iota(rows.begin(), rows.end(), 0);
    // Must match adjust.frag line for line.
    QtConcurrent::blockingMap(rows, [&](int y) {
        QRgb *line = reinterpret_cast<QRgb *>(image.scanLine(y));
        const float v = (y + .5f) / h;
        for (int x = 0; x < w; ++x) {
            const QRgb px = line[x];
            float r = qRed(px) / 255.f, g = qGreen(px) / 255.f, b = qBlue(px) / 255.f;
            auto bright = [&](float c) { return p.brightness >= 0 ? c + (1 - c) * p.brightness * .6f : c * (1 + p.brightness * .6f); };
            r = bright(r);
            g = bright(g);
            b = bright(b);
            float lum = .2126f * r + .7152f * g + .0722f * b;
            const float lift = p.shadows * .35f * (1 - smoothstep(0, .5f, lum)) + p.highlights * .35f * smoothstep(.5f, 1, lum);
            r += lift;
            g += lift;
            b += lift;
            r = (r - .5f) * (1 + p.contrast) + .5f;
            g = (g - .5f) * (1 + p.contrast) + .5f;
            b = (b - .5f) * (1 + p.contrast) + .5f;
            r += p.warmth * .08f;
            b -= p.warmth * .08f;
            g += p.tint * .06f;
            lum = .2126f * r + .7152f * g + .0722f * b;
            r = lum + (r - lum) * (1 + p.saturation);
            g = lum + (g - lum) * (1 + p.saturation);
            b = lum + (b - lum) * (1 + p.saturation);
            r = r * (1 - p.fade) + p.fade;
            g = g * (1 - p.fade) + p.fade;
            b = b * (1 - p.fade) + p.fade;
            const float u = (x + .5f) / w;
            const float d = std::sqrt((u - .5f) * (u - .5f) + (v - .5f) * (v - .5f)) * 1.41421f;
            const float f = smoothstep(.35f, 1.05f, d);
            if (p.vignette >= 0) {
                const float k = 1 - p.vignette * f * .75f;
                r *= k;
                g *= k;
                b *= k;
            } else {
                const float k = -p.vignette * f * .6f;
                r += (1 - r) * k;
                g += (1 - g) * k;
                b += (1 - b) * k;
            }
            auto out = [](float c) { return int(std::lround(std::clamp(c, 0.f, 1.f) * 255)); };
            line[x] = qRgba(out(r), out(g), out(b), qAlpha(px));
        }
    });
}

QImage PhotoEditor::render(const QImage &oriented, const Params &p, int quarterTurns, bool flipped, qreal straighten, const QRectF &crop, const QVariantList &strokes)
{
    QImage img = flipped ? oriented.flipped(Qt::Horizontal) : oriented;
    if (quarterTurns % 4)
        img = img.transformed(QTransform().rotate(90 * (quarterTurns % 4)));
    const int W = img.width(), H = img.height();
    if (!qFuzzyIsNull(straighten)) {
        QImage canvas(W, H, QImage::Format_ARGB32_Premultiplied);
        canvas.fill(Qt::black);
        QPainter painter(&canvas);
        painter.setRenderHints(QPainter::SmoothPixmapTransform | QPainter::Antialiasing);
        const qreal s = coverScale(W, H, straighten);
        painter.translate(W / 2.0, H / 2.0);
        painter.rotate(straighten);
        painter.scale(s, s);
        painter.drawImage(QPointF(-W / 2.0, -H / 2.0), img);
        painter.end();
        img = canvas;
    }
    const QRect rect = QRectF(crop.x() * W, crop.y() * H, crop.width() * W, crop.height() * H).toAlignedRect().intersected(QRect(0, 0, W, H));
    if (rect != QRect(0, 0, W, H))
        img = img.copy(rect);
    adjustPixels(img, p);
    if (!strokes.isEmpty()) {
        QPainter painter(&img);
        painter.setRenderHint(QPainter::Antialiasing);
        const qreal cw = img.width(), ch = img.height();
        for (const QVariant &v : strokes) {
            const QVariantMap s = v.toMap();
            const QVariantList pts = s.value(QStringLiteral("points")).toList();
            if (pts.size() < 2)
                continue;
            QColor colour(s.value(QStringLiteral("colour")).toString());
            const bool highlighter = s.value(QStringLiteral("highlighter")).toBool();
            if (highlighter)
                colour.setAlphaF(.4);
            QPen pen(colour, s.value(QStringLiteral("width")).toDouble() * cw, Qt::SolidLine, highlighter ? Qt::FlatCap : Qt::RoundCap, Qt::RoundJoin);
            painter.setPen(pen);
            QPainterPath path(QPointF(pts[0].toDouble() * cw, pts[1].toDouble() * ch));
            for (int i = 2; i + 1 < pts.size(); i += 2)
                path.lineTo(pts[i].toDouble() * cw, pts[i + 1].toDouble() * ch);
            painter.drawPath(path);
        }
    }
    return img;
}

QVariantMap PhotoEditor::autoValues(const QImage &source)
{
    const QImage img = source.scaled(256, 256, Qt::KeepAspectRatio, Qt::FastTransformation).convertToFormat(QImage::Format_RGB32);
    QList<int> hist(256, 0);
    double satSum = 0;
    int n = 0;
    for (int y = 0; y < img.height(); ++y) {
        const QRgb *line = reinterpret_cast<const QRgb *>(img.constScanLine(y));
        for (int x = 0; x < img.width(); ++x) {
            const QColor c(line[x]);
            ++hist[qGray(line[x])];
            satSum += c.hsvSaturationF();
            ++n;
        }
    }
    if (!n)
        return {};
    auto percentile = [&](double q) {
        int acc = 0;
        for (int i = 0; i < 256; ++i)
            if ((acc += hist[i]) >= q * n)
                return i;
        return 255;
    };
    const int lo = percentile(.01), hi = percentile(.99), mid = percentile(.5);
    const double range = std::max(1, hi - lo) / 255.0;
    // Stretch the levels gently, lift a dark median, and add saturation to dull photos.
    const double contrast = std::clamp((1 / range - 1) * .5, 0.0, .35);
    const double brightness = std::clamp((.5 - mid / 255.0) * .8, -.3, .4);
    const double saturation = satSum / n < .25 ? .15 : .05;
    const double shadows = lo > 20 ? 0.0 : .12;
    const double highlights = hi < 235 ? 0.0 : -.12;
    return {{QStringLiteral("brightness"), brightness}, {QStringLiteral("contrast"), contrast}, {QStringLiteral("saturation"), saturation},
            {QStringLiteral("shadows"), shadows}, {QStringLiteral("highlights"), highlights}};
}

void PhotoEditor::autoAdjust()
{
    if (m_path.isEmpty() || m_busy)
        return;
    setBusy(true);
    const QString path = m_path;
    QtConcurrent::run([path] { return autoValues(ImageIO::loadOriented(path).scaled(1024, 1024, Qt::KeepAspectRatio)); }).then(this, [this, path](const QVariantMap &v) {
        setBusy(false);
        if (path != m_path)
            return;
        for (auto it = v.cbegin(); it != v.cend(); ++it)
            m_values.insert(it.key(), it.value());
        emit valuesChanged();
        emit dirtyChanged();
    });
}

void PhotoEditor::save(bool asCopy)
{
    if (m_path.isEmpty() || m_busy)
        return;
    setBusy(true);
    const QString path = m_path;
    const Params params = paramsFrom(effective());
    const int turns = m_turns;
    const bool flipped = m_flipped;
    const qreal straighten = m_straighten;
    const QRectF crop = m_crop;
    const QVariantList strokes = m_strokes;
    struct Outcome { QString path; QString error; };
    QtConcurrent::run([=] {
        Outcome out;
        QString error;
        const QImage src = ImageIO::loadOriented(path, &error);
        if (src.isNull())
            return Outcome{{}, tr("Could not read the photo: %1").arg(error)};
        const QImage result = render(src, params, turns, flipped, straighten, crop, strokes);
        if (asCopy) {
            const QString dest = ImageIO::copyName(path);
            if (!ImageIO::save(result, dest, path, &error))
                return Outcome{{}, tr("Could not save a copy: %1").arg(error)};
            return Outcome{dest, {}};
        }
        // The original goes to the trash first, so a save can always be undone from there.
        const QFileInfo fi(path);
        const QString temp = fi.absolutePath() + QStringLiteral("/.") + fi.completeBaseName() + QStringLiteral(".opal-edit.") + fi.suffix();
        if (!ImageIO::save(result, temp, path, &error))
            return Outcome{{}, tr("Could not save: %1").arg(error)};
        if (!QFile::moveToTrash(path)) {
            QFile::remove(temp);
            return Outcome{{}, tr("Could not move the original to the trash. Use Save copy instead.")};
        }
        if (!QFile::rename(temp, path))
            return Outcome{{}, tr("Could not replace the photo; the edit is in %1").arg(temp)};
        return Outcome{path, {}};
    }).then(this, [this, asCopy](const Outcome &o) {
        setBusy(false);
        if (!o.error.isEmpty()) {
            emit failed(o.error);
            return;
        }
        PhotoLibrary::instance()->rescan();
        emit saved(o.path, asCopy);
    });
}
