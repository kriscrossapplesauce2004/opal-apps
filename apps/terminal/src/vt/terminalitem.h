#pragma once

#include "glyphatlas.h"
#include "terminalsession.h"

#include <QBitArray>
#include <QKeySequence>
#include <QPointer>
#include <QQuickItem>
#include <QTimer>
#include <QtQml/qqmlregistration.h>

class QSGGeometryNode;
class TerminalRootNode;

// Scene-graph view of a TerminalSession: textured glyph quads from an atlas plus coloured cell rectangles.
class TerminalItem : public QQuickItem
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(TerminalSession *session READ session WRITE setSession NOTIFY sessionChanged)
    Q_PROPERTY(QString fontFamily READ fontFamily WRITE setFontFamily NOTIFY fontChanged)
    Q_PROPERTY(qreal fontPixelSize READ fontPixelSize WRITE setFontPixelSize NOTIFY fontChanged)
    Q_PROPERTY(qreal lineHeightFactor READ lineHeightFactor WRITE setLineHeightFactor NOTIFY fontChanged)
    Q_PROPERTY(qreal padding READ padding WRITE setPadding NOTIFY paddingChanged)
    Q_PROPERTY(CursorStyle cursorStyle READ cursorStyle WRITE setCursorStyle NOTIFY cursorStyleChanged)
    Q_PROPERTY(bool cursorBlink READ cursorBlink WRITE setCursorBlink NOTIFY cursorStyleChanged)
    Q_PROPERTY(qreal backgroundAlpha READ backgroundAlpha WRITE setBackgroundAlpha NOTIFY backgroundChanged)
    Q_PROPERTY(bool drawBackground READ drawBackground WRITE setDrawBackground NOTIFY backgroundChanged)
    Q_PROPERTY(QStringList appShortcuts READ appShortcuts WRITE setAppShortcuts NOTIFY appShortcutsChanged)
    Q_PROPERTY(int cols READ cols NOTIFY gridChanged)
    Q_PROPERTY(int rows READ rows NOTIFY gridChanged)
    Q_PROPERTY(qreal cellWidth READ cellWidth NOTIFY metricsChanged)
    Q_PROPERTY(qreal cellHeight READ cellHeight NOTIFY metricsChanged)
    Q_PROPERTY(qreal contentY READ contentY NOTIFY contentYChanged)
    Q_PROPERTY(bool hasSelection READ hasSelection NOTIFY selectionChanged)
    Q_PROPERTY(QString selectedText READ selectedText NOTIFY selectionChanged)
public:
    // Default follows the program; the others force a shape.
    enum CursorStyle { CursorDefault, CursorBlock, CursorUnderline, CursorBar };
    Q_ENUM(CursorStyle)

    struct Vertex;
    explicit TerminalItem(QQuickItem *parent = nullptr);
    ~TerminalItem() override;

    TerminalSession *session() const { return m_session; }
    void setSession(TerminalSession *session);
    QString fontFamily() const { return m_fontFamily; }
    void setFontFamily(const QString &family);
    qreal fontPixelSize() const { return m_fontPixelSize; }
    void setFontPixelSize(qreal size);
    qreal lineHeightFactor() const { return m_lineHeightFactor; }
    void setLineHeightFactor(qreal factor);
    qreal padding() const { return m_padding; }
    void setPadding(qreal padding);
    CursorStyle cursorStyle() const { return m_cursorStyle; }
    void setCursorStyle(CursorStyle style);
    bool cursorBlink() const { return m_cursorBlink; }
    void setCursorBlink(bool blink);
    qreal backgroundAlpha() const { return m_backgroundAlpha; }
    void setBackgroundAlpha(qreal alpha);
    bool drawBackground() const { return m_drawBackground; }
    void setDrawBackground(bool draw);
    QStringList appShortcuts() const { return m_appShortcutStrings; }
    void setAppShortcuts(const QStringList &shortcuts);

    int cols() const { return m_session ? m_session->cols() : 0; }
    int rows() const { return m_session ? m_session->rows() : 0; }
    qreal cellWidth() const { return m_cellW / m_dpr; }
    qreal cellHeight() const { return m_cellH / m_dpr; }
    // Scroll position in pixels: the absolute line at the top of the view times the cell height.
    qreal contentY() const;
    bool hasSelection() const { return m_hasSelection; }
    QString selectedText() const;

    // Y of the top of an absolute line in item coordinates, or a large negative number when it is not shown.
    Q_INVOKABLE qreal yForLine(int absoluteLine) const;
    Q_INVOKABLE int lineForY(qreal y) const;
    Q_INVOKABLE void copy();
    Q_INVOKABLE void pasteFromClipboard();
    Q_INVOKABLE void selectAll();
    Q_INVOKABLE void clearSelection();

signals:
    void sessionChanged();
    void fontChanged();
    void paddingChanged();
    void cursorStyleChanged();
    void backgroundChanged();
    void appShortcutsChanged();
    void gridChanged();
    void metricsChanged();
    void contentYChanged();
    void selectionChanged();
    // Emitted for a key that matches appShortcuts; the item then leaves the key to the app.
    void shortcutFilter(int key, int modifiers);

protected:
    QSGNode *updatePaintNode(QSGNode *old, UpdatePaintNodeData *data) override;
    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;
    void itemChange(ItemChange change, const ItemChangeData &value) override;
    bool event(QEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void focusInEvent(QFocusEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void inputMethodEvent(QInputMethodEvent *event) override;
    QVariant inputMethodQuery(Qt::InputMethodQuery query) const override;

private:

    struct Selection
    {
        int startLine, startCol, endLine, endCol; // end column is exclusive
    };

    void rebuildMetrics();
    void recomputeGrid();
    void markCursorDirty();
    void resetBlink();
    void buildRow(int viewRow, QList<Vertex> &bg, QList<Vertex> &text);
    void buildSelection(QList<Vertex> &out);
    void updateNodeGeometry(QSGGeometryNode *node, const QList<Vertex> &vertices);
    QRgb resolveColor(quint32 word, QRgb fallback) const;
    bool cursorShownInRow(int viewRow, int *col, int *width, CursorStyle *style, bool *filled) const;
    Selection normalizedSelection() const;
    void cellFromPos(const QPointF &p, int *viewRow, int *col, bool clamp = true) const;
    void beginSelection(int line, int col);
    void extendSelection(int line, int col);
    void selectWordAt(int line, int col);
    void sendMouse(QMouseEvent *event, bool pressed, bool move);
    void scrollByWheel(QWheelEvent *event);

    QPointer<TerminalSession> m_session;
    GlyphAtlas m_atlas;
    QString m_fontFamily;
    qreal m_fontPixelSize = 14;
    qreal m_lineHeightFactor = 1.0;
    qreal m_padding = 14;
    CursorStyle m_cursorStyle = CursorDefault;
    bool m_cursorBlink = true;
    qreal m_backgroundAlpha = 1.0;
    bool m_drawBackground = true;
    QStringList m_appShortcutStrings;
    QList<QKeySequence> m_appShortcuts;

    qreal m_dpr = 1;
    int m_cellW = 8;  // device pixels
    int m_cellH = 16;
    int m_baseline = 12;
    int m_underlineY = 14;
    int m_strikeY = 8;
    int m_lineThickness = 1;
    bool m_metricsDirty = true;
    bool m_forceAll = true;
    QBitArray m_extraDirty;
    int m_builtGeneration = -1;
    bool m_blinkOn = true;
    QTimer m_blinkTimer;
    QTimer m_scrollTimer;
    QList<QString> m_ascii;

    bool m_hasSelection = false;
    int m_anchorLine = 0, m_anchorCol = 0, m_headLine = 0, m_headCol = 0;
    bool m_dragging = false;
    int m_clickCount = 0;
    qint64 m_lastClickMs = 0;
    QPointF m_lastClickPos;
    int m_autoScroll = 0;
    qreal m_wheelRemainder = 0;
    QVector<TermCell> m_rowCells;
    int m_lastTopLine = -1;
};
