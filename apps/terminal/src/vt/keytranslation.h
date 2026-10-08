#pragma once

#include <QString>

#include <cstdint>

// Result of mapping one Qt key event to libvterm input.
struct KeyTranslation
{
    enum Kind { None, Key, Char, Text };
    Kind kind = None;
    int vtermKey = 0;       // VTermKey, valid for Key
    uint32_t unichar = 0;   // valid for Char
    int vtermMods = 0;      // VTermModifier bits
    QString text;           // valid for Text, sent as is
};

namespace KeyTranslator {
// qtKey is a Qt::Key value and modifiers a Qt::KeyboardModifiers value.
KeyTranslation translate(int qtKey, int qtModifiers, const QString &text);
}
