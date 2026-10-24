#pragma once
#include <QImage>
#include <QObject>
#include <QRectF>
#include <QVariantList>
#include <QVariantMap>
#include <QtQml/qqmlregistration.h>

// Edit state for one photo, and the full-resolution render that saves it.
// The preview is the same maths on the GPU (shaders/adjust.frag); keep the two in step.
// Geometry order: mirror, quarter turns clockwise, straighten (with a cover zoom), then crop in normalised frame units.
// QML: PhotoEditor { path }.
// Properties: values (user adjustments by id, each -1..1), effective (values plus the filter), filter, filters ([{id, name}]),
//   adjustments ([{id, name, icon}]), quarterTurns, flipped, straighten (degrees, -45..45), crop (normalised rect),
//   strokes ([{colour, width, highlighter, points: [x, y, ...]}], normalised to the crop), dirty, busy.
// Methods: setValue(id, v), reset(), autoAdjust(), save(asCopy), undoStroke(), coverScale(w, h, degrees).
// Signals: saved(path, asCopy), failed(message).
class PhotoEditor : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QString path READ path WRITE setPath NOTIFY pathChanged)
    Q_PROPERTY(QVariantMap values READ values NOTIFY valuesChanged)
    Q_PROPERTY(QVariantMap effective READ effective NOTIFY valuesChanged)
    Q_PROPERTY(QString filter READ filter WRITE setFilter NOTIFY valuesChanged)
    Q_PROPERTY(QVariantList filters READ filters CONSTANT)
    Q_PROPERTY(QVariantList adjustments READ adjustments CONSTANT)
    Q_PROPERTY(int quarterTurns READ quarterTurns WRITE setQuarterTurns NOTIFY geometryChanged)
    Q_PROPERTY(bool flipped READ flipped WRITE setFlipped NOTIFY geometryChanged)
    Q_PROPERTY(qreal straighten READ straighten WRITE setStraighten NOTIFY geometryChanged)
    Q_PROPERTY(QRectF crop READ crop WRITE setCrop NOTIFY geometryChanged)
    Q_PROPERTY(QVariantList strokes READ strokes WRITE setStrokes NOTIFY strokesChanged)
    Q_PROPERTY(bool dirty READ dirty NOTIFY dirtyChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)

public:
    struct Params
    {
        float brightness = 0, contrast = 0, saturation = 0, warmth = 0, tint = 0, highlights = 0, shadows = 0, vignette = 0, fade = 0;
    };

    explicit PhotoEditor(QObject *parent = nullptr);

    QString path() const { return m_path; }
    void setPath(const QString &p);
    QVariantMap values() const { return m_values; }
    QVariantMap effective() const;
    QString filter() const { return m_filter; }
    void setFilter(const QString &f);
    static QVariantList filters();
    static QVariantList adjustments();
    int quarterTurns() const { return m_turns; }
    void setQuarterTurns(int t);
    bool flipped() const { return m_flipped; }
    void setFlipped(bool f);
    qreal straighten() const { return m_straighten; }
    void setStraighten(qreal d);
    QRectF crop() const { return m_crop; }
    void setCrop(const QRectF &r);
    QVariantList strokes() const { return m_strokes; }
    void setStrokes(const QVariantList &s);
    bool dirty() const;
    bool busy() const { return m_busy; }

    Q_INVOKABLE void setValue(const QString &id, qreal v);
    Q_INVOKABLE void reset();
    Q_INVOKABLE void autoAdjust();
    Q_INVOKABLE void save(bool asCopy);
    Q_INVOKABLE void undoStroke();
    // Zoom that makes a w x h image rotated by degrees cover its own w x h frame.
    Q_INVOKABLE static qreal coverScale(qreal w, qreal h, qreal degrees);

    // The full pipeline on the CPU, as the save does it. Public for tests.
    static QImage render(const QImage &oriented, const Params &p, int quarterTurns, bool flipped, qreal straighten, const QRectF &crop, const QVariantList &strokes);
    static void adjustPixels(QImage &image, const Params &p);
    static Params paramsFrom(const QVariantMap &effective);
    // Auto: levels and exposure from the luminance histogram.
    static QVariantMap autoValues(const QImage &image);

signals:
    void pathChanged();
    void valuesChanged();
    void geometryChanged();
    void strokesChanged();
    void dirtyChanged();
    void busyChanged();
    void saved(const QString &path, bool asCopy);
    void failed(const QString &message);

private:
    void setBusy(bool b);

    QString m_path;
    QVariantMap m_values;
    QString m_filter = QStringLiteral("none");
    int m_turns = 0;
    bool m_flipped = false;
    qreal m_straighten = 0;
    QRectF m_crop{0, 0, 1, 1};
    QVariantList m_strokes;
    bool m_busy = false;
};
