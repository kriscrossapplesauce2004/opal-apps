#include <QTest>
#include "tonal.h"

// Checks the tonal-spot scheme: tones stay in order and text keeps M3 contrast against its surface.
class TonalTest : public QObject
{
    Q_OBJECT
    static double luminance(const QColor &c)
    {
        auto lin = [](double v) { return v <= 0.04045 ? v / 12.92 : std::pow((v + 0.055) / 1.055, 2.4); };
        return 0.2126 * lin(c.redF()) + 0.7152 * lin(c.greenF()) + 0.0722 * lin(c.blueF());
    }
    static double contrast(const QColor &a, const QColor &b)
    {
        const double x = luminance(a), y = luminance(b);
        return (std::max(x, y) + 0.05) / (std::min(x, y) + 0.05);
    }
private slots:
    void tonesAreOrdered()
    {
        const opal::TonalPalette p{280, 40};
        double last = -1;
        for (int t = 0; t <= 100; t += 10) {
            const double l = luminance(p.tone(t));
            QVERIFY(l >= last);
            last = l;
        }
    }
    void textContrast_data()
    {
        QTest::addColumn<QColor>("seed");
        QTest::addColumn<bool>("light");
        for (const char *s : {"#7355ad", "#006575", "#95512d", "#ff0000", "#00ff00", "#3584e4", "#ffd700"})
            for (bool light : {false, true})
                QTest::newRow(qPrintable(QStringLiteral("%1 %2").arg(QLatin1String(s), light ? QStringLiteral("light") : QStringLiteral("dark")))) << QColor(QLatin1String(s)) << light;
    }
    void textContrast()
    {
        QFETCH(QColor, seed);
        QFETCH(bool, light);
        const QVariantMap m = opal::schemeFromSeed(seed, light);
        auto c = [&m](const char *k) { return m.value(QLatin1String(k)).value<QColor>(); };
        QVERIFY(contrast(c("on_surface"), c("surface")) >= 7.0);
        QVERIFY(contrast(c("on_primary"), c("primary")) >= 4.5);
        QVERIFY(contrast(c("on_primary_container"), c("primary_container")) >= 4.5);
        QVERIFY(contrast(c("on_surface_variant"), c("surface_container_high")) >= 4.5);
    }
};

QTEST_GUILESS_MAIN(TonalTest)
#include "tst_tonal.moc"
