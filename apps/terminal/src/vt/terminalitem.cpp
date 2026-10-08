#include "terminalitem.h"

#include "terminalpty.h"

#include <QClipboard>
#include <QDateTime>
#include <QFontDatabase>
#include <QFontMetricsF>
#include <QGuiApplication>
#include <QInputMethodEvent>
#include <QKeyEvent>
#include <QQuickWindow>
#include <QSGGeometryNode>
#include <QSGMaterial>
#include <QSGMaterialShader>
#include <QSGTexture>
#include <QStyleHints>

#include <algorithm>
#include <cmath>

struct TerminalItem::Vertex
{
    float x, y, u, v;
    uchar r, g, b, a;
};

namespace {

// One material draws every quad: glyphs sample their atlas slot, rectangles sample the solid block.
class CellMaterial : public QSGMaterial
{
public:
    CellMaterial() { setFlag(Blending); }
    QSGMaterialType *type() const override
    {
        static QSGMaterialType t;
        return &t;
    }
    QSGMaterialShader *createShader(QSGRendererInterface::RenderMode) const override;
    QSGTexture *texture = nullptr;
};

class CellShader : public QSGMaterialShader
{
public:
    CellShader()
    {
        setShaderFileName(VertexStage, QStringLiteral(":/opal/terminal/shaders/cells.vert.qsb"));
        setShaderFileName(FragmentStage, QStringLiteral(":/opal/terminal/shaders/cells.frag.qsb"));
    }
    bool updateUniformData(RenderState &state, QSGMaterial *, QSGMaterial *) override
    {
        QByteArray *buf = state.uniformData();
        bool changed = false;
        if (state.isMatrixDirty()) {
            const QMatrix4x4 m = state.combinedMatrix();
            memcpy(buf->data(), m.constData(), 64);
            changed = true;
        }
        if (state.isOpacityDirty()) {
            const float opacity = state.opacity();
            memcpy(buf->data() + 64, &opacity, 4);
            changed = true;
        }
        return changed;
    }
    void updateSampledImage(RenderState &state, int binding, QSGTexture **texture, QSGMaterial *newMaterial, QSGMaterial *) override
    {
        if (binding != 1)
            return;
        auto *m = static_cast<CellMaterial *>(newMaterial);
        if (m->texture) {
            m->texture->commitTextureOperations(state.rhi(), state.resourceUpdateBatch());
            *texture = m->texture;
        }
    }
};

QSGMaterialShader *CellMaterial::createShader(QSGRendererInterface::RenderMode) const
{
    return new CellShader;
}

} // namespace

// Scene-graph tree: full background, per-row cell backgrounds, selection, per-row text.
class TerminalRootNode : public QSGNode
{
public:
    TerminalRootNode()
    {
        bgFull = makeNode();
        bgGroup = new QSGNode;
        selection = makeNode();
        textGroup = new QSGNode;
        appendChildNode(bgFull);
        appendChildNode(bgGroup);
        appendChildNode(selection);
        appendChildNode(textGroup);
    }
    ~TerminalRootNode() override
    {
        // Nodes borrow the material, so they go before it.
        while (firstChild())
            delete firstChild();
        delete texture;
    }

    QSGGeometryNode *makeNode()
    {
        static const QSGGeometry::Attribute attrs[] = {
            QSGGeometry::Attribute::create(0, 2, QSGGeometry::FloatType, true),
            QSGGeometry::Attribute::create(1, 2, QSGGeometry::FloatType),
            QSGGeometry::Attribute::create(2, 4, QSGGeometry::UnsignedByteType),
        };
        static const QSGGeometry::AttributeSet set = {3, 20, attrs};
        auto *node = new QSGGeometryNode;
        auto *g = new QSGGeometry(set, 0, 0, QSGGeometry::UnsignedShortType);
        g->setDrawingMode(QSGGeometry::DrawTriangles);
        node->setGeometry(g);
        node->setFlag(QSGNode::OwnsGeometry);
        node->setMaterial(&material);
        node->setFlag(QSGNode::OwnsMaterial, false);
        return node;
    }

    void setRows(int rows)
    {
        while (bgGroup->firstChild())
            delete bgGroup->firstChild();
        while (textGroup->firstChild())
            delete textGroup->firstChild();
        bgRows.clear();
        textRows.clear();
        for (int i = 0; i < rows; ++i) {
            bgRows.append(makeNode());
            textRows.append(makeNode());
            bgGroup->appendChildNode(bgRows.last());
            textGroup->appendChildNode(textRows.last());
        }
    }

    void setTexture(QSGTexture *tex)
    {
        delete texture;
        texture = tex;
        material.texture = tex;
        // A new texture object means every node must bind it again.
        auto dirty = [](QSGGeometryNode *n) { n->markDirty(QSGNode::DirtyMaterial); };
        dirty(bgFull);
        dirty(selection);
        for (QSGGeometryNode *n : std::as_const(bgRows))
            dirty(n);
        for (QSGGeometryNode *n : std::as_const(textRows))
            dirty(n);
    }

    CellMaterial material;
    QSGTexture *texture = nullptr;
    QSGGeometryNode *bgFull = nullptr;
    QSGGeometryNode *selection = nullptr;
    QSGNode *bgGroup = nullptr;
    QSGNode *textGroup = nullptr;
    QList<QSGGeometryNode *> bgRows;
    QList<QSGGeometryNode *> textRows;
};

namespace {

struct Rgba
{
    uchar r, g, b, a;
};

Rgba premul(QRgb c, float alpha = 1.0f)
{
    const float a = alpha * (qAlpha(c) / 255.0f);
    return {uchar(qRed(c) * a + .5f), uchar(qGreen(c) * a + .5f), uchar(qBlue(c) * a + .5f), uchar(a * 255.0f + .5f)};
}

QRgb mix(QRgb a, QRgb b, float t)
{
    return qRgb(int(qRed(a) * (1 - t) + qRed(b) * t), int(qGreen(a) * (1 - t) + qGreen(b) * t), int(qBlue(a) * (1 - t) + qBlue(b) * t));
}

} // namespace

static void pushQuad(QList<TerminalItem::Vertex> &out, float x0, float y0, float x1, float y1, float u0, float v0, float u1, float v1, Rgba c)
{
    out.append({x0, y0, u0, v0, c.r, c.g, c.b, c.a});
    out.append({x1, y0, u1, v0, c.r, c.g, c.b, c.a});
    out.append({x0, y1, u0, v1, c.r, c.g, c.b, c.a});
    out.append({x1, y1, u1, v1, c.r, c.g, c.b, c.a});
}

TerminalItem::TerminalItem(QQuickItem *parent)
    : QQuickItem(parent)
{
    setFlag(ItemHasContents);
    setFlag(ItemAcceptsInputMethod);
    setFlag(ItemIsFocusScope, false);
    setAcceptedMouseButtons(Qt::LeftButton | Qt::MiddleButton | Qt::RightButton);
    setAcceptHoverEvents(true);
    setActiveFocusOnTab(true);
    setCursor(Qt::IBeamCursor);

    m_ascii.resize(128);
    for (int i = 0; i < 128; ++i)
        m_ascii[i] = QString(QChar(i));

    m_blinkTimer.setInterval(530);
    connect(&m_blinkTimer, &QTimer::timeout, this, [this] {
        m_blinkOn = !m_blinkOn;
        markCursorDirty();
        update();
    });
    m_scrollTimer.setInterval(40);
    connect(&m_scrollTimer, &QTimer::timeout, this, [this] {
        if (!m_session || !m_dragging || m_autoScroll == 0)
            return;
        m_session->scrollViewport(m_autoScroll > 0 ? 1 : -1);
        const int viewRow = m_autoScroll > 0 ? 0 : m_session->rows() - 1;
        extendSelection(m_session->viewTopLine() + viewRow, m_headCol);
    });
    connect(this, &QQuickItem::activeFocusChanged, this, [this] {
        resetBlink();
        markCursorDirty();
        update();
    });
    rebuildMetrics();
}

TerminalItem::~TerminalItem() = default;

void TerminalItem::setSession(TerminalSession *session)
{
    if (session == m_session)
        return;
    if (m_session)
        disconnect(m_session, nullptr, this, nullptr);
    m_session = session;
    if (m_session) {
        connect(m_session, &TerminalSession::damaged, this, [this] { update(); });
        connect(m_session, &TerminalSession::cursorChanged, this, [this] { resetBlink(); });
        connect(m_session, &TerminalSession::viewportChanged, this, &TerminalItem::contentYChanged);
        connect(m_session, &TerminalSession::scrollbackChanged, this, &TerminalItem::contentYChanged);
        connect(m_session, &TerminalSession::sizeChanged, this, [this] {
            m_extraDirty = QBitArray();
            emit gridChanged();
            emit contentYChanged();
            update();
        });
        recomputeGrid();
    }
    m_forceAll = true;
    emit sessionChanged();
    emit gridChanged();
    update();
}

void TerminalItem::setFontFamily(const QString &family)
{
    if (family == m_fontFamily)
        return;
    m_fontFamily = family;
    rebuildMetrics();
}

void TerminalItem::setFontPixelSize(qreal size)
{
    size = std::max<qreal>(4, size);
    if (qFuzzyCompare(size, m_fontPixelSize))
        return;
    m_fontPixelSize = size;
    rebuildMetrics();
}

void TerminalItem::setLineHeightFactor(qreal factor)
{
    factor = std::clamp<qreal>(factor, 0.5, 3);
    if (qFuzzyCompare(factor, m_lineHeightFactor))
        return;
    m_lineHeightFactor = factor;
    rebuildMetrics();
}

void TerminalItem::setPadding(qreal padding)
{
    if (qFuzzyCompare(padding, m_padding))
        return;
    m_padding = std::max<qreal>(0, padding);
    m_forceAll = true;
    recomputeGrid();
    emit paddingChanged();
    update();
}

void TerminalItem::setCursorStyle(CursorStyle style)
{
    if (style == m_cursorStyle)
        return;
    m_cursorStyle = style;
    markCursorDirty();
    emit cursorStyleChanged();
    update();
}

void TerminalItem::setCursorBlink(bool blink)
{
    if (blink == m_cursorBlink)
        return;
    m_cursorBlink = blink;
    resetBlink();
    emit cursorStyleChanged();
}

void TerminalItem::setBackgroundAlpha(qreal alpha)
{
    alpha = std::clamp<qreal>(alpha, 0, 1);
    if (qFuzzyCompare(alpha, m_backgroundAlpha))
        return;
    m_backgroundAlpha = alpha;
    emit backgroundChanged();
    update();
}

void TerminalItem::setDrawBackground(bool draw)
{
    if (draw == m_drawBackground)
        return;
    m_drawBackground = draw;
    emit backgroundChanged();
    update();
}

void TerminalItem::setAppShortcuts(const QStringList &shortcuts)
{
    m_appShortcutStrings = shortcuts;
    m_appShortcuts.clear();
    for (const QString &s : shortcuts)
        m_appShortcuts.append(QKeySequence::fromString(s));
    emit appShortcutsChanged();
}

void TerminalItem::rebuildMetrics()
{
    m_dpr = window() ? window()->effectiveDevicePixelRatio() : 1.0;
    QFont f = m_fontFamily.isEmpty() ? QFontDatabase::systemFont(QFontDatabase::FixedFont) : QFont(m_fontFamily);
    f.setPixelSize(std::max(1, qRound(m_fontPixelSize * m_dpr)));
    f.setStyleStrategy(QFont::PreferAntialias);
    f.setKerning(false);
    const QFontMetricsF fm(f);
    m_cellW = std::max(1, qRound(fm.horizontalAdvance(QLatin1Char('M'))));
    const qreal natural = fm.ascent() + fm.descent();
    m_cellH = std::max(1, qRound(std::ceil(natural) * m_lineHeightFactor));
    m_baseline = qRound((m_cellH - natural) / 2.0 + fm.ascent());
    m_lineThickness = std::max(1, qRound(m_fontPixelSize * m_dpr / 14.0));
    m_underlineY = std::min(m_cellH - m_lineThickness, m_baseline + std::max(1, qRound(fm.descent() * 0.35)));
    m_strikeY = std::max(0, m_baseline - qRound(fm.ascent() * 0.32));
    m_atlas.configure(f, m_cellW, m_cellH, m_baseline);
    m_metricsDirty = false;
    m_forceAll = true;
    recomputeGrid();
    emit fontChanged();
    emit metricsChanged();
    emit contentYChanged();
    update();
}

void TerminalItem::recomputeGrid()
{
    if (!m_session || width() <= 0 || height() <= 0)
        return;
    const double availW = (width() - 2 * m_padding) * m_dpr;
    const double availH = (height() - 2 * m_padding) * m_dpr;
    const int cols = std::max(2, int(std::floor(availW / m_cellW + 1e-6)));
    const int rows = std::max(1, int(std::floor(availH / m_cellH + 1e-6)));
    m_session->resize(cols, rows, qRound(width() * m_dpr), qRound(height() * m_dpr));
}

void TerminalItem::geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    QQuickItem::geometryChange(newGeometry, oldGeometry);
    if (newGeometry.size() != oldGeometry.size()) {
        recomputeGrid();
        update();
    }
}

void TerminalItem::itemChange(ItemChange change, const ItemChangeData &value)
{
    QQuickItem::itemChange(change, value);
    if ((change == ItemSceneChange && value.window) || change == ItemDevicePixelRatioHasChanged) {
        if (!qFuzzyCompare(m_dpr, value.window ? value.window->effectiveDevicePixelRatio() : m_dpr) || change == ItemSceneChange)
            rebuildMetrics();
    }
}

qreal TerminalItem::contentY() const
{
    return m_session ? m_session->viewTopLine() * cellHeight() : 0;
}

qreal TerminalItem::yForLine(int absoluteLine) const
{
    if (!m_session)
        return -1e9;
    return m_padding + (absoluteLine - m_session->viewTopLine()) * cellHeight();
}

int TerminalItem::lineForY(qreal y) const
{
    if (!m_session)
        return 0;
    return m_session->viewTopLine() + int(std::floor((y - m_padding) / cellHeight()));
}

void TerminalItem::markCursorDirty()
{
    if (!m_session)
        return;
    if (m_extraDirty.size() != m_session->rows())
        m_extraDirty = QBitArray(m_session->rows(), false);
    const int r = m_session->cursorRow() + m_session->viewOffset();
    if (r >= 0 && r < m_extraDirty.size())
        m_extraDirty.setBit(r);
}

void TerminalItem::resetBlink()
{
    m_blinkOn = true;
    const bool blinking = m_cursorBlink && hasActiveFocus() && (!m_session || m_session->cursorBlink());
    if (blinking)
        m_blinkTimer.start();
    else
        m_blinkTimer.stop();
    markCursorDirty();
    update();
}

QRgb TerminalItem::resolveColor(quint32 word, QRgb fallback) const
{
    switch (word >> 24) {
    case 1: return m_session->palette()->indexed(int(word & 0xff));
    case 2: return 0xff000000u | (word & 0xffffff);
    default: return fallback;
    }
}

bool TerminalItem::cursorShownInRow(int viewRow, int *col, int *width, CursorStyle *style, bool *filled) const
{
    if (!m_session->cursorVisible())
        return false;
    if (m_session->cursorRow() + m_session->viewOffset() != viewRow)
        return false;
    CursorStyle st = m_cursorStyle;
    if (st == CursorDefault) {
        switch (m_session->cursorShape()) {
        case TerminalSession::Underline: st = CursorUnderline; break;
        case TerminalSession::Bar: st = CursorBar; break;
        default: st = CursorBlock; break;
        }
    }
    const bool focused = hasActiveFocus();
    if (focused && m_cursorBlink && m_session->cursorBlink() && !m_blinkOn)
        return false;
    *style = st;
    *filled = focused && st == CursorBlock;
    *col = m_session->cursorColumn();
    *width = 1;
    return true;
}

void TerminalItem::buildRow(int r, QList<Vertex> &bg, QList<Vertex> &text)
{
    m_session->fetchViewRow(r, m_rowCells);
    const TerminalPalette *pal = m_session->palette();
    const QRgb defFg = pal->foreground().rgb();
    const QRgb defBg = pal->background().rgb();
    const int cols = m_session->cols();
    const float cw = float(m_cellW / m_dpr);
    const float ch = float(m_cellH / m_dpr);
    const float x0 = float(m_padding);
    const float y = float(m_padding) + r * ch;
    const QSize asz = m_atlas.size();
    const QRect sr = m_atlas.solidRect();
    const float su = (sr.x() + sr.width() / 2.0f) / asz.width();
    const float sv = (sr.y() + sr.height() / 2.0f) / asz.height();
    auto solid = [&](QList<Vertex> &v, float xa, float ya, float xb, float yb, QRgb c) { pushQuad(v, xa, ya, xb, yb, su, sv, su, sv, premul(c)); };

    int curCol = 0, curW = 1;
    CursorStyle curStyle = CursorBlock;
    bool curFilled = false;
    const bool hasCursor = cursorShownInRow(r, &curCol, &curW, &curStyle, &curFilled);
    if (hasCursor && curCol >= 0 && curCol < cols && m_rowCells[curCol].width == 2)
        curW = 2;
    auto inCursor = [&](int c) { return hasCursor && curFilled && c >= curCol && c < curCol + curW; };

    auto colors = [&](const TermCell &cell, QRgb &fg, QRgb &bgc, bool &hasBg) {
        fg = resolveColor(cell.fg, defFg);
        bgc = resolveColor(cell.bg, defBg);
        hasBg = cell.bg != 0;
        if (cell.attrs & TermCell::Reverse) {
            std::swap(fg, bgc);
            hasBg = true;
        }
        if (cell.attrs & TermCell::Faint)
            fg = mix(fg, bgc, 0.45f);
    };

    // Cell backgrounds merge into runs of one colour.
    int runStart = -1;
    QRgb runColor = 0;
    for (int c = 0; c <= cols; ++c) {
        bool has = false;
        QRgb color = 0;
        if (c < cols) {
            QRgb fg, b;
            colors(m_rowCells[c], fg, b, has);
            color = b;
            if (inCursor(c)) {
                has = true;
                color = pal->cursor().rgb();
            }
        }
        if (runStart >= 0 && (!has || color != runColor)) {
            solid(bg, x0 + runStart * cw, y, x0 + c * cw, y + ch, runColor);
            runStart = -1;
        }
        if (has && runStart < 0) {
            runStart = c;
            runColor = color;
        }
    }

    const float aw = float(asz.width());
    const float ah = float(asz.height());
    const float thick = float(m_lineThickness / m_dpr);
    for (int c = 0; c < cols; ++c) {
        const TermCell &cell = m_rowCells[c];
        if (cell.width == 0)
            continue;
        QRgb fg, b;
        bool hasBg;
        colors(cell, fg, b, hasBg);
        if (inCursor(c))
            fg = pal->cursorText().rgb();
        const int span = cell.width == 2 ? 2 : 1;
        const float xa = x0 + c * cw;
        const float xb = xa + span * cw;

        if (cell.ch && cell.ch != ' ' && !(cell.attrs & TermCell::Conceal)) {
            quint8 style = GlyphAtlas::Regular;
            if (cell.attrs & TermCell::Bold)
                style |= GlyphAtlas::Bold;
            if (cell.attrs & TermCell::Italic)
                style |= GlyphAtlas::Italic;
            const QString &str = cell.ch < 128 ? m_ascii[int(cell.ch)] : QString();
            const GlyphAtlas::Glyph g = m_atlas.glyph(cell.ch < 128 ? str : m_session->clusterText(cell.ch), style, span);
            if (!g.empty) {
                const Rgba col = g.color ? Rgba{255, 255, 255, 255} : premul(fg);
                pushQuad(text, xa, y, xb, y + ch, g.rect.x() / aw, g.rect.y() / ah, (g.rect.x() + g.rect.width()) / aw, (g.rect.y() + g.rect.height()) / ah, col);
            }
        }
        const int ul = (cell.attrs & TermCell::UnderlineMask) >> 2;
        if (ul) {
            const float uy = y + float(m_underlineY / m_dpr);
            solid(text, xa, uy, xb, uy + thick, fg);
            if (ul == 2)
                solid(text, xa, uy + 2 * thick, xb, uy + 3 * thick, fg);
        }
        if (cell.attrs & TermCell::Strike) {
            const float sy = y + float(m_strikeY / m_dpr);
            solid(text, xa, sy, xb, sy + thick, fg);
        }
    }

    // Shapes other than the filled block draw above the text.
    if (hasCursor && !curFilled && curCol >= 0 && curCol < cols) {
        const QRgb cc = pal->cursor().rgb();
        const float xa = x0 + curCol * cw;
        const float xb = xa + curW * cw;
        if (curStyle == CursorBar) {
            solid(text, xa, y, xa + std::max(thick, 2.0f / float(m_dpr)), y + ch, cc);
        } else if (curStyle == CursorUnderline) {
            solid(text, xa, y + ch - 2 * thick, xb, y + ch, cc);
        } else {
            solid(text, xa, y, xb, y + thick, cc);
            solid(text, xa, y + ch - thick, xb, y + ch, cc);
            solid(text, xa, y + thick, xa + thick, y + ch - thick, cc);
            solid(text, xb - thick, y + thick, xb, y + ch - thick, cc);
        }
    }
}

TerminalItem::Selection TerminalItem::normalizedSelection() const
{
    int sl = m_anchorLine, sc = m_anchorCol, el = m_headLine, ec = m_headCol;
    if (sl > el || (sl == el && sc > ec)) {
        std::swap(sl, el);
        std::swap(sc, ec);
    }
    return {sl, sc, el, ec + 1};
}

void TerminalItem::buildSelection(QList<Vertex> &out)
{
    if (!m_hasSelection || !m_session)
        return;
    const Selection s = normalizedSelection();
    const int top = m_session->viewTopLine();
    const int cols = m_session->cols();
    const QSize asz = m_atlas.size();
    const QRect sr = m_atlas.solidRect();
    const float su = (sr.x() + sr.width() / 2.0f) / asz.width();
    const float sv = (sr.y() + sr.height() / 2.0f) / asz.height();
    const float cw = float(m_cellW / m_dpr);
    const float ch = float(m_cellH / m_dpr);
    const QColor sel = m_session->palette()->selection();
    const Rgba color = premul(sel.rgb(), float(sel.alphaF()));
    for (int r = 0; r < m_session->rows(); ++r) {
        const int line = top + r;
        if (line < s.startLine || line > s.endLine)
            continue;
        const int c0 = line == s.startLine ? s.startCol : 0;
        const int c1 = line == s.endLine ? s.endCol : cols;
        if (c1 <= c0)
            continue;
        const float y = float(m_padding) + r * ch;
        pushQuad(out, float(m_padding) + c0 * cw, y, float(m_padding) + std::min(c1, cols) * cw, y + ch, su, sv, su, sv, color);
    }
}

void TerminalItem::updateNodeGeometry(QSGGeometryNode *node, const QList<Vertex> &vertices)
{
    QSGGeometry *g = node->geometry();
    const int quads = int(vertices.size() / 4);
    // Quads index 16-bit vertices, so one node holds at most 16000 of them.
    const int capped = std::min(quads, 16000);
    g->allocate(capped * 4, capped * 6);
    if (capped > 0) {
        memcpy(g->vertexData(), vertices.constData(), size_t(capped) * 4 * sizeof(Vertex));
        quint16 *idx = g->indexDataAsUShort();
        for (int q = 0; q < capped; ++q) {
            const quint16 b = quint16(q * 4);
            idx[q * 6 + 0] = b;
            idx[q * 6 + 1] = b + 1;
            idx[q * 6 + 2] = b + 2;
            idx[q * 6 + 3] = b + 2;
            idx[q * 6 + 4] = b + 1;
            idx[q * 6 + 5] = b + 3;
        }
    }
    node->markDirty(QSGNode::DirtyGeometry);
}

QSGNode *TerminalItem::updatePaintNode(QSGNode *old, UpdatePaintNodeData *)
{
    auto *root = static_cast<TerminalRootNode *>(old);
    if (!m_session || !window() || width() <= 0 || height() <= 0) {
        delete root;
        return nullptr;
    }
    const int rows = m_session->rows();
    bool all = m_forceAll || m_session->allDirty();
    if (!root) {
        root = new TerminalRootNode;
        all = true;
    }
    if (root->bgRows.size() != rows) {
        root->setRows(rows);
        all = true;
    }
    if (m_atlas.generation() != m_builtGeneration)
        all = true;

    QBitArray dirty = m_session->dirtyRows();
    if (m_extraDirty.size() == dirty.size())
        dirty |= m_extraDirty;
    QList<Vertex> bg, text;
    for (int attempt = 0; attempt < 2; ++attempt) {
        const int gen = m_atlas.generation();
        for (int r = 0; r < rows; ++r) {
            if (!all && !(r < dirty.size() && dirty.testBit(r)))
                continue;
            bg.clear();
            text.clear();
            buildRow(r, bg, text);
            updateNodeGeometry(root->bgRows[r], bg);
            updateNodeGeometry(root->textRows[r], text);
        }
        // New glyphs may have grown or reset the atlas, which moves slots under quads built earlier.
        if (m_atlas.generation() == gen)
            break;
        all = true;
    }
    m_builtGeneration = m_atlas.generation();

    if (m_atlas.takeDirty() || !root->texture) {
        QSGTexture *tex = window()->createTextureFromImage(m_atlas.image(), QQuickWindow::TextureHasAlphaChannel);
        tex->setFiltering(QSGTexture::Linear);
        tex->setHorizontalWrapMode(QSGTexture::ClampToEdge);
        tex->setVerticalWrapMode(QSGTexture::ClampToEdge);
        root->setTexture(tex);
    }

    // The full background and the selection are tiny, so they are rebuilt every time.
    bg.clear();
    if (m_drawBackground) {
        const QSize asz = m_atlas.size();
        const QRect sr = m_atlas.solidRect();
        const float su = (sr.x() + sr.width() / 2.0f) / asz.width();
        const float sv = (sr.y() + sr.height() / 2.0f) / asz.height();
        pushQuad(bg, 0, 0, float(width()), float(height()), su, sv, su, sv, premul(m_session->palette()->background().rgb(), float(m_backgroundAlpha)));
    }
    updateNodeGeometry(root->bgFull, bg);
    text.clear();
    buildSelection(text);
    updateNodeGeometry(root->selection, text);

    m_session->clearDirty();
    m_extraDirty.fill(false);
    m_forceAll = false;
    return root;
}

// ---------------------------------------------------------------------------------------------
// Input

bool TerminalItem::event(QEvent *e)
{
    if (e->type() == QEvent::ShortcutOverride && hasActiveFocus()) {
        auto *ke = static_cast<QKeyEvent *>(e);
        const QKeySequence ks(ke->keyCombination());
        if (m_appShortcuts.contains(ks))
            return QQuickItem::event(e);
        // The terminal keeps every other key, so Ctrl+C and friends reach the shell.
        e->accept();
        return true;
    }
    return QQuickItem::event(e);
}

void TerminalItem::keyPressEvent(QKeyEvent *event)
{
    if (!m_session) {
        event->ignore();
        return;
    }
    const int key = event->key();
    const Qt::KeyboardModifiers mods = event->modifiers();
    const Qt::KeyboardModifiers cs = Qt::ControlModifier | Qt::ShiftModifier;

    if (m_appShortcuts.contains(QKeySequence(event->keyCombination()))) {
        emit shortcutFilter(key, int(mods));
        event->ignore();
        return;
    }
    if ((mods & ~Qt::KeypadModifier) == cs && key == Qt::Key_C) {
        copy();
        event->accept();
        return;
    }
    if (((mods & ~Qt::KeypadModifier) == cs && key == Qt::Key_V) || ((mods & ~Qt::KeypadModifier) == Qt::ShiftModifier && key == Qt::Key_Insert)) {
        pasteFromClipboard();
        event->accept();
        return;
    }
    if ((mods & ~Qt::KeypadModifier) == Qt::ShiftModifier && !m_session->altScreen()) {
        if (key == Qt::Key_PageUp || key == Qt::Key_PageDown) {
            m_session->scrollViewport((key == Qt::Key_PageUp ? 1 : -1) * std::max(1, m_session->rows() - 1));
            event->accept();
            return;
        }
        if (key == Qt::Key_Home || key == Qt::Key_End) {
            if (key == Qt::Key_Home)
                m_session->scrollViewport(m_session->scrollbackCount());
            else
                m_session->scrollToBottom();
            event->accept();
            return;
        }
    }
    if (key == Qt::Key_Shift || key == Qt::Key_Control || key == Qt::Key_Alt || key == Qt::Key_Meta || key == Qt::Key_AltGr
        || key == Qt::Key_CapsLock || key == Qt::Key_NumLock) {
        event->accept();
        return;
    }
    clearSelection();
    resetBlink();
    m_session->sendKey(key, int(mods), event->text());
    event->accept();
}

void TerminalItem::focusInEvent(QFocusEvent *event)
{
    QQuickItem::focusInEvent(event);
    if (m_session)
        m_session->focusChanged(true);
}

void TerminalItem::focusOutEvent(QFocusEvent *event)
{
    QQuickItem::focusOutEvent(event);
    if (m_session)
        m_session->focusChanged(false);
}

void TerminalItem::inputMethodEvent(QInputMethodEvent *event)
{
    if (m_session && !event->commitString().isEmpty())
        m_session->sendText(event->commitString());
    event->accept();
}

QVariant TerminalItem::inputMethodQuery(Qt::InputMethodQuery query) const
{
    switch (query) {
    case Qt::ImEnabled: return true;
    case Qt::ImCursorRectangle:
        if (m_session)
            return QRectF(m_padding + m_session->cursorColumn() * cellWidth(), m_padding + m_session->cursorRow() * cellHeight(), cellWidth(), cellHeight());
        return QRectF();
    default: return QQuickItem::inputMethodQuery(query);
    }
}

void TerminalItem::cellFromPos(const QPointF &p, int *viewRow, int *col, bool clamp) const
{
    int r = int(std::floor((p.y() - m_padding) / cellHeight()));
    int c = int(std::floor((p.x() - m_padding) / cellWidth()));
    if (clamp) {
        r = std::clamp(r, 0, std::max(0, m_session->rows() - 1));
        c = std::clamp(c, 0, std::max(0, m_session->cols() - 1));
    }
    *viewRow = r;
    *col = c;
}

void TerminalItem::sendMouse(QMouseEvent *event, bool pressed, bool move)
{
    int r, c;
    cellFromPos(event->position(), &r, &c);
    const int mods = int(event->modifiers());
    m_session->mouseMove(r, c, mods);
    if (!move) {
        const int btn = event->button() == Qt::LeftButton ? 1 : event->button() == Qt::MiddleButton ? 2 : 3;
        m_session->mouseButton(btn, pressed, mods);
    }
}

void TerminalItem::mousePressEvent(QMouseEvent *event)
{
    forceActiveFocus(Qt::MouseFocusReason);
    if (!m_session) {
        event->ignore();
        return;
    }
    const bool reporting = m_session->mouseMode() != TerminalSession::MouseNone && !(event->modifiers() & Qt::ShiftModifier);
    if (reporting) {
        sendMouse(event, true, false);
        event->accept();
        return;
    }
    if (event->button() == Qt::MiddleButton) {
        QClipboard *cb = QGuiApplication::clipboard();
        const QString t = cb->text(cb->supportsSelection() ? QClipboard::Selection : QClipboard::Clipboard);
        if (!t.isEmpty())
            m_session->paste(t);
        event->accept();
        return;
    }
    if (event->button() != Qt::LeftButton) {
        event->ignore();
        return;
    }
    int r, c;
    cellFromPos(event->position(), &r, &c);
    const int line = m_session->viewTopLine() + r;
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    const bool near = (event->position() - m_lastClickPos).manhattanLength() < 6;
    if (near && now - m_lastClickMs <= QGuiApplication::styleHints()->mouseDoubleClickInterval())
        m_clickCount = m_clickCount % 3 + 1;
    else
        m_clickCount = 1;
    m_lastClickMs = now;
    m_lastClickPos = event->position();

    if (m_clickCount == 1) {
        clearSelection();
        beginSelection(line, c);
    } else if (m_clickCount == 2) {
        selectWordAt(line, c);
    } else {
        m_anchorLine = m_headLine = line;
        m_anchorCol = 0;
        m_headCol = m_session->cols() - 1;
        m_hasSelection = true;
        m_dragging = true;
        emit selectionChanged();
        update();
    }
    event->accept();
}

void TerminalItem::mouseDoubleClickEvent(QMouseEvent *event)
{
    mousePressEvent(event);
}

void TerminalItem::mouseMoveEvent(QMouseEvent *event)
{
    if (!m_session)
        return;
    const bool reporting = m_session->mouseMode() >= TerminalSession::MouseDrag && !(event->modifiers() & Qt::ShiftModifier);
    if (reporting && !m_dragging) {
        sendMouse(event, false, true);
        event->accept();
        return;
    }
    if (!m_dragging)
        return;
    int r, c;
    cellFromPos(event->position(), &r, &c, false);
    m_autoScroll = r < 0 ? 1 : r >= m_session->rows() ? -1 : 0;
    if (m_autoScroll != 0)
        m_scrollTimer.start();
    else
        m_scrollTimer.stop();
    r = std::clamp(r, 0, m_session->rows() - 1);
    c = std::clamp(c, 0, m_session->cols() - 1);
    extendSelection(m_session->viewTopLine() + r, c);
    event->accept();
}

void TerminalItem::mouseReleaseEvent(QMouseEvent *event)
{
    if (!m_session)
        return;
    const bool reporting = m_session->mouseMode() != TerminalSession::MouseNone && !(event->modifiers() & Qt::ShiftModifier);
    if (reporting && !m_dragging) {
        sendMouse(event, false, false);
        event->accept();
        return;
    }
    m_scrollTimer.stop();
    m_autoScroll = 0;
    if (m_dragging && m_hasSelection) {
        QClipboard *cb = QGuiApplication::clipboard();
        if (cb->supportsSelection())
            cb->setText(selectedText(), QClipboard::Selection);
    }
    m_dragging = false;
    event->accept();
}

void TerminalItem::wheelEvent(QWheelEvent *event)
{
    if (!m_session)
        return;
    qreal lines;
    if (!event->pixelDelta().isNull())
        lines = event->pixelDelta().y() / cellHeight();
    else
        lines = event->angleDelta().y() / 120.0 * 3.0;
    m_wheelRemainder += lines;
    const int steps = int(m_wheelRemainder);
    if (steps == 0) {
        event->accept();
        return;
    }
    m_wheelRemainder -= steps;
    const bool report = m_session->mouseMode() != TerminalSession::MouseNone && !(event->modifiers() & Qt::ShiftModifier);
    if (report) {
        int r, c;
        cellFromPos(event->position(), &r, &c);
        m_session->mouseMove(r, c, int(event->modifiers()));
        for (int i = 0; i < std::abs(steps); ++i) {
            m_session->mouseButton(steps > 0 ? 4 : 5, true, int(event->modifiers()));
            m_session->mouseButton(steps > 0 ? 4 : 5, false, int(event->modifiers()));
        }
    } else if (m_session->altScreen()) {
        // Full screen programs without mouse support get arrow keys, like other terminals do.
        for (int i = 0; i < std::abs(steps); ++i)
            m_session->sendKey(steps > 0 ? Qt::Key_Up : Qt::Key_Down, 0, QString());
    } else {
        m_session->scrollViewport(steps);
    }
    event->accept();
}

void TerminalItem::beginSelection(int line, int col)
{
    m_anchorLine = m_headLine = line;
    m_anchorCol = m_headCol = col;
    m_dragging = true;
}

void TerminalItem::extendSelection(int line, int col)
{
    m_headLine = line;
    m_headCol = col;
    const bool had = m_hasSelection;
    m_hasSelection = !(m_anchorLine == m_headLine && m_anchorCol == m_headCol) || m_clickCount > 1;
    if (had != m_hasSelection || m_hasSelection)
        emit selectionChanged();
    update();
}

void TerminalItem::selectWordAt(int line, int col)
{
    QVector<TermCell> cells;
    if (!m_session->fetchLine(line, cells))
        return;
    auto isWordCell = [&](int c) {
        const TermCell &t = cells[c];
        if (t.width == 0)
            return true;
        if (t.ch == 0 || t.ch == ' ')
            return false;
        if (t.ch < 128)
            return !QStringLiteral("\"'`()[]{}<>|;,").contains(QChar(char16_t(t.ch)));
        return true;
    };
    int a = col, b = col;
    if (isWordCell(col)) {
        while (a > 0 && isWordCell(a - 1))
            --a;
        while (b + 1 < cells.size() && isWordCell(b + 1))
            ++b;
    }
    m_anchorLine = m_headLine = line;
    m_anchorCol = a;
    m_headCol = b;
    m_hasSelection = true;
    m_dragging = true;
    emit selectionChanged();
    update();
}

void TerminalItem::clearSelection()
{
    if (!m_hasSelection)
        return;
    m_hasSelection = false;
    emit selectionChanged();
    update();
}

void TerminalItem::selectAll()
{
    if (!m_session)
        return;
    m_anchorLine = m_session->firstLine();
    m_anchorCol = 0;
    m_headLine = m_session->screenTopLine() + m_session->rows() - 1;
    m_headCol = m_session->cols() - 1;
    m_hasSelection = true;
    emit selectionChanged();
    update();
}

QString TerminalItem::selectedText() const
{
    if (!m_hasSelection || !m_session)
        return {};
    const Selection s = normalizedSelection();
    return m_session->textBetween(s.startLine, s.startCol, s.endLine, s.endCol);
}

void TerminalItem::copy()
{
    const QString t = selectedText();
    if (!t.isEmpty())
        QGuiApplication::clipboard()->setText(t);
}

void TerminalItem::pasteFromClipboard()
{
    if (!m_session)
        return;
    const QString t = QGuiApplication::clipboard()->text();
    if (!t.isEmpty()) {
        clearSelection();
        m_session->paste(t);
    }
}
