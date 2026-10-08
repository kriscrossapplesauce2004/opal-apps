#pragma once

#include <QColor>
#include <QObject>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

#include <array>

// Colours of one terminal: 16 ANSI colours plus foreground, background, cursor and selection.
class TerminalPalette : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QColor foreground READ foreground WRITE setForeground NOTIFY changed)
    Q_PROPERTY(QColor background READ background WRITE setBackground NOTIFY changed)
    Q_PROPERTY(QColor cursor READ cursor WRITE setCursor NOTIFY changed)
    Q_PROPERTY(QColor cursorText READ cursorText WRITE setCursorText NOTIFY changed)
    Q_PROPERTY(QColor selection READ selection WRITE setSelection NOTIFY changed)
    Q_PROPERTY(QVariantList ansi READ ansi WRITE setAnsi NOTIFY changed)
public:
    explicit TerminalPalette(QObject *parent = nullptr);

    QColor foreground() const { return m_fg; }
    QColor background() const { return m_bg; }
    QColor cursor() const { return m_cursor; }
    QColor cursorText() const { return m_cursorText; }
    QColor selection() const { return m_selection; }
    void setForeground(const QColor &c);
    void setBackground(const QColor &c);
    void setCursor(const QColor &c);
    void setCursorText(const QColor &c);
    void setSelection(const QColor &c);

    QVariantList ansi() const;
    // A list shorter than 16 sets only the colours it has.
    void setAnsi(const QVariantList &colors);
    Q_INVOKABLE QColor color(int index) const;
    Q_INVOKABLE void setColor(int index, const QColor &c);
    Q_INVOKABLE void resetToDefault();

    // Indexed colour 0..255: the ANSI 16 first, then the 6x6x6 cube and the grey ramp.
    QRgb indexed(int index) const;

signals:
    void changed();

private:
    QColor m_fg, m_bg, m_cursor, m_cursorText, m_selection;
    std::array<QRgb, 256> m_table {};
};
