#pragma once

#include <QFont>
#include <QHash>
#include <QImage>
#include <QRect>
#include <QString>
#include <utility>

// Shelf-packed texture atlas of rasterised glyphs, one slot per glyph, keyed by text and style.
class GlyphAtlas
{
public:
    enum Style : quint8 { Regular = 0, Bold = 1, Italic = 2 };

    struct Glyph
    {
        QRect rect;          // pixels inside the atlas
        bool color = false;  // colour bitmap such as emoji, drawn without tinting
        bool empty = true;   // nothing visible, so no quad is needed
    };

    GlyphAtlas();

    // Sets the font in device pixels and the cell size. This drops every cached glyph.
    void configure(const QFont &font, int cellWidth, int cellHeight, int baseline);
    // Looks the glyph up and rasterises it on the first use. cells is 1 or 2.
    Glyph glyph(const QString &text, quint8 style, int cells);

    const QImage &image() const { return m_image; }
    QSize size() const { return m_image.size(); }
    // The solid block used for rectangles; the centre of this rect is fully opaque.
    QRect solidRect() const { return m_solid; }
    // True once after pixels changed, so the texture needs a new upload.
    bool takeDirty() { return std::exchange(m_dirty, false); }
    // Changes whenever existing slots moved or the image was replaced, so cached quads are stale.
    int generation() const { return m_generation; }

private:
    struct Key
    {
        QString text;
        quint8 style;
        quint8 cells;
        bool operator==(const Key &o) const = default;
    };
    friend size_t qHash(const Key &k, size_t seed) { return qHashMulti(seed, k.text, k.style, k.cells); }

    void reset(int height);
    bool allocate(int cells, QPoint *pos);

    QFont m_font;
    int m_cellW = 8;
    int m_cellH = 16;
    int m_baseline = 12;
    QImage m_image;
    QRect m_solid;
    int m_x = 0;
    int m_y = 0;
    bool m_dirty = true;
    int m_generation = 0;
    QHash<Key, Glyph> m_glyphs;
    Glyph m_none;
};
