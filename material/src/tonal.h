#pragma once
#include <QColor>
#include <QVariantMap>

namespace opal {

// Hue and chroma of one key colour; tone() gives the colour at a lightness from 0 to 100.
struct TonalPalette {
    double hue = 0;
    double chroma = 0;
    QColor tone(double t) const;
};

// Tonal-spot M3 scheme from a seed colour. Keys are M3 role names in snake case (primary, on_primary, surface_container_high...). chroma scales colourfulness; below 1 gives a calmer scheme.
QVariantMap schemeFromSeed(const QColor &seed, bool light, double chroma = 1.0);

}
