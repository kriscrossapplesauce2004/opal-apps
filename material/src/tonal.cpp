#include "tonal.h"
#include <algorithm>
#include <cmath>
#include <numbers>

namespace opal {
namespace {

// D65 reference white for CIELAB.
constexpr double Xn = 0.95047, Yn = 1.0, Zn = 1.08883;

double toLinear(double c) { return c <= 0.04045 ? c / 12.92 : std::pow((c + 0.055) / 1.055, 2.4); }
double fromLinear(double c) { return c <= 0.0031308 ? 12.92 * c : 1.055 * std::pow(c, 1.0 / 2.4) - 0.055; }
double labF(double t) { return t > 216.0 / 24389.0 ? std::cbrt(t) : (24389.0 / 27.0 * t + 16.0) / 116.0; }
double labFInv(double t) { const double t3 = t * t * t; return t3 > 216.0 / 24389.0 ? t3 : (116.0 * t - 16.0) / (24389.0 / 27.0); }

struct Lab { double l, a, b; };

Lab toLab(const QColor &c)
{
    const double r = toLinear(c.redF()), g = toLinear(c.greenF()), b = toLinear(c.blueF());
    const double x = 0.4124564 * r + 0.3575761 * g + 0.1804375 * b;
    const double y = 0.2126729 * r + 0.7151522 * g + 0.0721750 * b;
    const double z = 0.0193339 * r + 0.1191920 * g + 0.9503041 * b;
    const double fx = labF(x / Xn), fy = labF(y / Yn), fz = labF(z / Zn);
    return {116.0 * fy - 16.0, 500.0 * (fx - fy), 200.0 * (fy - fz)};
}

// Returns false when the colour is outside the sRGB gamut.
bool fromLab(const Lab &lab, double rgb[3])
{
    const double fy = (lab.l + 16.0) / 116.0, fx = fy + lab.a / 500.0, fz = fy - lab.b / 200.0;
    const double x = Xn * labFInv(fx), y = Yn * labFInv(fy), z = Zn * labFInv(fz);
    const double lin[3] = {
        3.2404542 * x - 1.5371385 * y - 0.4985314 * z,
        -0.9692660 * x + 1.8760108 * y + 0.0415560 * z,
        0.0556434 * x - 0.2040259 * y + 1.0572252 * z};
    bool inside = true;
    for (int i = 0; i < 3; ++i) {
        if (lin[i] < -1e-4 || lin[i] > 1.0001) inside = false;
        rgb[i] = std::clamp(fromLinear(std::clamp(lin[i], 0.0, 1.0)), 0.0, 1.0);
    }
    return inside;
}

}

QColor TonalPalette::tone(double t) const
{
    if (t <= 0) return QColor(0, 0, 0);
    if (t >= 100) return QColor(255, 255, 255);
    const double h = hue * std::numbers::pi / 180.0;
    double rgb[3];
    // Keep the lightness exact and reduce chroma until the colour fits in sRGB.
    if (fromLab({t, chroma * std::cos(h), chroma * std::sin(h)}, rgb))
        return QColor::fromRgbF(rgb[0], rgb[1], rgb[2]);
    double lo = 0, hi = chroma;
    for (int i = 0; i < 18; ++i) {
        const double mid = (lo + hi) / 2;
        if (fromLab({t, mid * std::cos(h), mid * std::sin(h)}, rgb)) lo = mid; else hi = mid;
    }
    fromLab({t, lo * std::cos(h), lo * std::sin(h)}, rgb);
    return QColor::fromRgbF(rgb[0], rgb[1], rgb[2]);
}

QVariantMap schemeFromSeed(const QColor &seed, bool light)
{
    const Lab lab = toLab(seed.isValid() ? seed : QColor("#7355ad"));
    double hue = std::atan2(lab.b, lab.a) * 180.0 / std::numbers::pi;
    if (hue < 0) hue += 360;
    // CIELAB chroma runs about 1.3 times CAM16 chroma, so the M3 tonal-spot chroma values are scaled.
    const double k = 1.3;
    const TonalPalette p{hue, 36 * k}, s{hue, 16 * k}, t{std::fmod(hue + 60, 360), 24 * k};
    const TonalPalette n{hue, 5 * k}, nv{hue, 9 * k}, e{25, 70 * k};
    QVariantMap m;
    auto set = [&m](const char *key, const QColor &c) { m.insert(QString::fromLatin1(key), c); };
    auto accent = [&](const char *name, const TonalPalette &tp) {
        const QString b = QString::fromLatin1(name);
        m.insert(b, tp.tone(light ? 40 : 80));
        m.insert(QStringLiteral("on_") + b, tp.tone(light ? 100 : 20));
        m.insert(b + QStringLiteral("_container"), tp.tone(light ? 90 : 30));
        m.insert(QStringLiteral("on_") + b + QStringLiteral("_container"), tp.tone(light ? 10 : 90));
    };
    accent("primary", p);
    accent("secondary", s);
    accent("tertiary", t);
    accent("error", e);
    set("background", n.tone(light ? 98 : 6));
    set("on_background", n.tone(light ? 10 : 90));
    set("surface", n.tone(light ? 98 : 6));
    set("surface_dim", n.tone(light ? 87 : 6));
    set("surface_bright", n.tone(light ? 98 : 24));
    set("surface_container_lowest", n.tone(light ? 100 : 4));
    set("surface_container_low", n.tone(light ? 96 : 10));
    set("surface_container", n.tone(light ? 94 : 12));
    set("surface_container_high", n.tone(light ? 92 : 17));
    set("surface_container_highest", n.tone(light ? 90 : 22));
    set("on_surface", n.tone(light ? 10 : 90));
    set("on_surface_variant", nv.tone(light ? 30 : 80));
    set("outline", nv.tone(light ? 50 : 60));
    set("outline_variant", nv.tone(light ? 80 : 30));
    set("inverse_surface", n.tone(light ? 20 : 90));
    set("inverse_on_surface", n.tone(light ? 95 : 20));
    set("inverse_primary", p.tone(light ? 80 : 40));
    set("scrim", QColor(0, 0, 0));
    set("shadow", QColor(0, 0, 0));
    return m;
}

}
