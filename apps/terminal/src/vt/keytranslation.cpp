#include <QList>
#include "keytranslation.h"

#include <QtCore/qnamespace.h>
#include <vterm.h>

namespace {

struct Entry
{
    int qt;
    VTermKey key;
};

constexpr Entry kSpecial[] = {
    {Qt::Key_Return, VTERM_KEY_ENTER},
    {Qt::Key_Enter, VTERM_KEY_ENTER},
    {Qt::Key_Tab, VTERM_KEY_TAB},
    {Qt::Key_Backspace, VTERM_KEY_BACKSPACE},
    {Qt::Key_Escape, VTERM_KEY_ESCAPE},
    {Qt::Key_Up, VTERM_KEY_UP},
    {Qt::Key_Down, VTERM_KEY_DOWN},
    {Qt::Key_Left, VTERM_KEY_LEFT},
    {Qt::Key_Right, VTERM_KEY_RIGHT},
    {Qt::Key_Insert, VTERM_KEY_INS},
    {Qt::Key_Delete, VTERM_KEY_DEL},
    {Qt::Key_Home, VTERM_KEY_HOME},
    {Qt::Key_End, VTERM_KEY_END},
    {Qt::Key_PageUp, VTERM_KEY_PAGEUP},
    {Qt::Key_PageDown, VTERM_KEY_PAGEDOWN},
};

VTermKey keypadKey(int qtKey)
{
    if (qtKey >= Qt::Key_0 && qtKey <= Qt::Key_9)
        return static_cast<VTermKey>(VTERM_KEY_KP_0 + (qtKey - Qt::Key_0));
    switch (qtKey) {
    case Qt::Key_Asterisk: return VTERM_KEY_KP_MULT;
    case Qt::Key_Plus: return VTERM_KEY_KP_PLUS;
    case Qt::Key_Comma: return VTERM_KEY_KP_COMMA;
    case Qt::Key_Minus: return VTERM_KEY_KP_MINUS;
    case Qt::Key_Period: return VTERM_KEY_KP_PERIOD;
    case Qt::Key_Slash: return VTERM_KEY_KP_DIVIDE;
    case Qt::Key_Equal: return VTERM_KEY_KP_EQUAL;
    default: return VTERM_KEY_NONE;
    }
}

} // namespace

KeyTranslation KeyTranslator::translate(int qtKey, int qtModifiers, const QString &text)
{
    KeyTranslation out;
    int mods = VTERM_MOD_NONE;
    if (qtModifiers & Qt::ShiftModifier)
        mods |= VTERM_MOD_SHIFT;
    if (qtModifiers & Qt::AltModifier)
        mods |= VTERM_MOD_ALT;
    if (qtModifiers & Qt::ControlModifier)
        mods |= VTERM_MOD_CTRL;
    out.vtermMods = mods;

    auto asKey = [&](VTermKey k) {
        out.kind = KeyTranslation::Key;
        out.vtermKey = k;
        return out;
    };

    if (qtKey == Qt::Key_Backtab) {
        out.vtermMods |= VTERM_MOD_SHIFT;
        return asKey(VTERM_KEY_TAB);
    }
    if (qtKey >= Qt::Key_F1 && qtKey <= Qt::Key_F35)
        return asKey(static_cast<VTermKey>(VTERM_KEY_FUNCTION(qtKey - Qt::Key_F1 + 1)));
    if (qtModifiers & Qt::KeypadModifier) {
        const VTermKey kp = keypadKey(qtKey);
        if (kp != VTERM_KEY_NONE)
            return asKey(kp);
    }
    for (const Entry &e : kSpecial) {
        if (e.qt == qtKey)
            return asKey(e.key);
    }

    // With Ctrl the text is a control code, so the character comes from the key itself.
    if ((mods & VTERM_MOD_CTRL) && qtKey >= 0x20 && qtKey <= 0x7e) {
        uint32_t c = static_cast<uint32_t>(qtKey);
        if (c >= 'A' && c <= 'Z')
            c += 'a' - 'A';
        out.kind = KeyTranslation::Char;
        out.unichar = c;
        return out;
    }

    const QList<uint> cps = text.toUcs4();
    if (cps.isEmpty() || cps.first() < 0x20 || cps.first() == 0x7f)
        return out;
    if (cps.size() == 1) {
        out.kind = KeyTranslation::Char;
        out.unichar = cps.first();
        return out;
    }
    // Several code points come from compose or input methods and carry no modifiers.
    out.kind = KeyTranslation::Text;
    out.text = text;
    return out;
}
