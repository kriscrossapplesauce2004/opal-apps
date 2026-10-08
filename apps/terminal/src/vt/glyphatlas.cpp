#include "glyphatlas.h"

#include <QPainter>

namespace {
constexpr int kAtlasWidth = 2048;
constexpr int kInitialHeight = 1024;
constexpr int kMaxHeight = 8192;
}

GlyphAtlas::GlyphAtlas()
{
    reset(kInitialHeight);
}

void GlyphAtlas::reset(int height)
{
    m_image = QImage(kAtlasWidth, height, QImage::Format_ARGB32_Premultiplied);
    m_image.fill(Qt::transparent);
    m_glyphs.clear();
    // The first slot holds the solid block that rectangles sample.
    m_solid = QRect(0, 0, std::max(4, m_cellW), std::max(4, m_cellH));
    QPainter p(&m_image);
    p.setCompositionMode(QPainter::CompositionMode_Source);
    p.fillRect(m_solid, Qt::white);
    p.end();
    m_x = m_solid.width();
    m_y = 0;
    m_dirty = true;
    ++m_generation;
}

void GlyphAtlas::configure(const QFont &font, int cellWidth, int cellHeight, int baseline)
{
    m_font = font;
    m_cellW = std::max(1, cellWidth);
    m_cellH = std::max(1, cellHeight);
    m_baseline = baseline;
    reset(kInitialHeight);
}

bool GlyphAtlas::allocate(int cells, QPoint *pos)
{
    const int w = cells * m_cellW;
    if (m_x + w > kAtlasWidth) {
        m_x = 0;
        m_y += m_cellH;
    }
    if (m_y + m_cellH > m_image.height()) {
        if (m_image.height() >= kMaxHeight)
            return false;
        const int newHeight = std::min(kMaxHeight, m_image.height() * 2);
        QImage grown(kAtlasWidth, newHeight, QImage::Format_ARGB32_Premultiplied);
        grown.fill(Qt::transparent);
        QPainter p(&grown);
        p.setCompositionMode(QPainter::CompositionMode_Source);
        p.drawImage(0, 0, m_image);
        p.end();
        m_image = grown;
        // Texture coordinates depend on the height, so quads built earlier are stale.
        ++m_generation;
    }
    *pos = QPoint(m_x, m_y);
    m_x += w;
    return true;
}

GlyphAtlas::Glyph GlyphAtlas::glyph(const QString &text, quint8 style, int cells)
{
    const Key key{text, style, static_cast<quint8>(cells)};
    if (auto it = m_glyphs.constFind(key); it != m_glyphs.constEnd())
        return *it;

    QPoint pos;
    if (!allocate(cells, &pos)) {
        // The atlas is full, so it starts again; callers see the new generation and rebuild.
        reset(kMaxHeight);
        allocate(cells, &pos);
    }

    QImage tile(cells * m_cellW, m_cellH, QImage::Format_ARGB32_Premultiplied);
    tile.fill(Qt::transparent);
    QFont f = m_font;
    f.setBold(style & Bold);
    f.setItalic(style & Italic);
    {
        QPainter p(&tile);
        p.setRenderHint(QPainter::TextAntialiasing, true);
        p.setFont(f);
        p.setPen(Qt::white);
        p.drawText(QPointF(0, m_baseline), text);
    }

    Glyph g;
    for (int y = 0; y < tile.height() && g.empty; ++y) {
        const QRgb *line = reinterpret_cast<const QRgb *>(tile.constScanLine(y));
        for (int x = 0; x < tile.width(); ++x) {
            if (qAlpha(line[x]))
                g.empty = false;
        }
    }
    // A colour font ignores the pen, so any tinted pixel marks the glyph as a colour bitmap.
    for (int y = 0; y < tile.height() && !g.empty && !g.color; ++y) {
        const QRgb *line = reinterpret_cast<const QRgb *>(tile.constScanLine(y));
        for (int x = 0; x < tile.width(); ++x) {
            if (qRed(line[x]) != qGreen(line[x]) || qGreen(line[x]) != qBlue(line[x])) {
                g.color = true;
                break;
            }
        }
    }
    g.rect = QRect(pos, tile.size());
    if (!g.empty) {
        QPainter p(&m_image);
        p.setCompositionMode(QPainter::CompositionMode_Source);
        p.drawImage(pos, tile);
        m_dirty = true;
    }
    m_glyphs.insert(key, g);
    return g;
}
