#include "terminalpalette.h"

TerminalPalette::TerminalPalette(QObject *parent)
    : QObject(parent)
{
    resetToDefault();
}

void TerminalPalette::resetToDefault()
{
    // The dark default follows the tones of the TermView prototype.
    static const char *const ansi[16] = {
        "#2b2f38", "#f28b82", "#81c995", "#fdd663", "#8ab4f8", "#c58af9", "#78d9ec", "#d7dae0",
        "#5b6270", "#f6a8a1", "#a3dbb1", "#fee391", "#abc9fa", "#d6aefb", "#a2e6f4", "#ffffff",
    };
    for (int i = 0; i < 16; ++i)
        m_table[i] = QColor::fromString(QLatin1String(ansi[i])).rgb();
    static const int levels[6] = {0, 95, 135, 175, 215, 255};
    for (int i = 0; i < 216; ++i)
        m_table[16 + i] = qRgb(levels[i / 36], levels[(i / 6) % 6], levels[i % 6]);
    for (int i = 0; i < 24; ++i)
        m_table[232 + i] = qRgb(8 + i * 10, 8 + i * 10, 8 + i * 10);
    m_fg = QColor(QStringLiteral("#d7dae0"));
    m_bg = QColor(QStringLiteral("#15171c"));
    m_cursor = QColor(QStringLiteral("#8fd4c4"));
    m_cursorText = QColor(QStringLiteral("#15171c"));
    m_selection = QColor(143, 212, 196, 90);
    emit changed();
}

#define OPAL_SET(name, member) \
    void TerminalPalette::name(const QColor &c) \
    { \
        if (member == c) \
            return; \
        member = c; \
        emit changed(); \
    }
OPAL_SET(setForeground, m_fg)
OPAL_SET(setBackground, m_bg)
OPAL_SET(setCursor, m_cursor)
OPAL_SET(setCursorText, m_cursorText)
OPAL_SET(setSelection, m_selection)
#undef OPAL_SET

QVariantList TerminalPalette::ansi() const
{
    QVariantList out;
    for (int i = 0; i < 16; ++i)
        out.append(QColor::fromRgb(m_table[i]));
    return out;
}

void TerminalPalette::setAnsi(const QVariantList &colors)
{
    for (int i = 0; i < colors.size() && i < 16; ++i)
        m_table[i] = colors.at(i).value<QColor>().rgb();
    emit changed();
}

QColor TerminalPalette::color(int index) const
{
    return QColor::fromRgb(indexed(index));
}

void TerminalPalette::setColor(int index, const QColor &c)
{
    if (index < 0 || index > 255)
        return;
    m_table[index] = c.rgb();
    emit changed();
}

QRgb TerminalPalette::indexed(int index) const
{
    return m_table[static_cast<size_t>(qBound(0, index, 255))];
}
