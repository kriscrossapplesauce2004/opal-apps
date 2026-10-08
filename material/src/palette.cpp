#include "palette.h"
#include "tonal.h"

#include <QDBusArgument>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusReply>
#include <QDBusVariant>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>

namespace {

constexpr auto PortalService = "org.freedesktop.portal.Desktop";
constexpr auto PortalPath = "/org/freedesktop/portal/desktop";
constexpr auto PortalSettings = "org.freedesktop.portal.Settings";

// Opal's four static palettes, the same values as the shell's Theme.qml.
struct OpalStatic { const char *primary, *onPrimary, *secondary, *tertiary, *dark, *light, *seed; };
constexpr OpalStatic OpalPalettes[] = {
    {"#cfbdff", "#31205f", "#c6d6ae", "#efb8c8", "#17141f", "#f7f0ff", "#7355ad"},
    {"#a5efff", "#005462", "#bcecf2", "#dfd6bb", "#101b1e", "#effbfc", "#006575"},
    {"#ffc09e", "#54220a", "#d5c1f0", "#c8dba5", "#211711", "#fff2e9", "#95512d"},
    {"#b3ccff", "#13305f", "#d4c4ef", "#a6dfc2", "#131923", "#eff3ff", "#435f91"},
};

// Removes nested QDBusVariant and QDBusArgument layers until a plain value or a structure is left.
QVariant unwrap(QVariant v)
{
    while (v.canConvert<QDBusVariant>() && v.metaType() == QMetaType::fromType<QDBusVariant>())
        v = v.value<QDBusVariant>().variant();
    return v;
}

QColor accentFrom(const QVariant &raw)
{
    QVariant v = unwrap(raw);
    if (v.metaType() != QMetaType::fromType<QDBusArgument>()) return {};
    const QDBusArgument arg = v.value<QDBusArgument>();
    if (arg.currentType() != QDBusArgument::StructureType) return {};
    double r = -1, g = -1, b = -1;
    arg.beginStructure();
    arg >> r >> g >> b;
    arg.endStructure();
    // The portal sends values outside 0..1 when no accent colour is set.
    if (r < 0 || r > 1 || g < 0 || g > 1 || b < 0 || b > 1) return {};
    return QColor::fromRgbF(r, g, b);
}

}

Palette::Palette(QObject *parent) : QObject(parent)
{
    const QByteArray seedEnv = qgetenv("OPAL_SEED");
    if (!seedEnv.isEmpty() && QColor::isValidColorName(QString::fromUtf8(seedEnv)))
        m_seed = QColor(QString::fromUtf8(seedEnv));
    connect(&m_watcher, &QFileSystemWatcher::fileChanged, this, &Palette::preferencesTouched);
    connect(&m_watcher, &QFileSystemWatcher::directoryChanged, this, &Palette::preferencesTouched);
    QDBusConnection::sessionBus().connect(QString::fromLatin1(PortalService), QString::fromLatin1(PortalPath),
        QString::fromLatin1(PortalSettings), QStringLiteral("SettingChanged"), this,
        SLOT(portalSettingChanged(QString,QString,QDBusVariant)));
    readPortal();
    readPreferences();
    resolve();
}

Palette *Palette::instance()
{
    static Palette *palette = new Palette;
    return palette;
}

Palette *Palette::create(QQmlEngine *, QJSEngine *engine)
{
    Palette *p = instance();
    engine->setObjectOwnership(p, QJSEngine::CppOwnership);
    return p;
}

QString Palette::preferencesPath()
{
    const QByteArray env = qgetenv("OPAL_PREFERENCES");
    if (!env.isEmpty()) return QString::fromUtf8(env);
    QByteArray state = qgetenv("XDG_STATE_HOME");
    const QString base = state.isEmpty() ? QDir::homePath() + QStringLiteral("/.local/state") : QString::fromUtf8(state);
    return base + QStringLiteral("/opal/preferences.json");
}

void Palette::setSeed(const QColor &seed)
{
    if (!seed.isValid() || seed == m_seed || !qgetenv("OPAL_SEED").isEmpty()) return;
    m_seed = seed;
    resolve();
}

QVariantMap Palette::schemeFor(const QColor &seed, bool light) const
{
    return opal::schemeFromSeed(seed, light);
}

void Palette::readPortal()
{
    auto read = [](const char *ns, const char *key) -> QVariant {
        QDBusMessage msg = QDBusMessage::createMethodCall(QString::fromLatin1(PortalService), QString::fromLatin1(PortalPath),
            QString::fromLatin1(PortalSettings), QStringLiteral("ReadOne"));
        msg << QString::fromLatin1(ns) << QString::fromLatin1(key);
        QDBusMessage reply = QDBusConnection::sessionBus().call(msg, QDBus::Block, 800);
        if (reply.type() != QDBusMessage::ReplyMessage) {
            // Portals older than version 2 only have Read, which wraps the value one more time.
            msg = QDBusMessage::createMethodCall(QString::fromLatin1(PortalService), QString::fromLatin1(PortalPath),
                QString::fromLatin1(PortalSettings), QStringLiteral("Read"));
            msg << QString::fromLatin1(ns) << QString::fromLatin1(key);
            reply = QDBusConnection::sessionBus().call(msg, QDBus::Block, 800);
        }
        if (reply.type() != QDBusMessage::ReplyMessage || reply.arguments().isEmpty()) return {};
        return reply.arguments().first();
    };
    m_portalAccent = accentFrom(read("org.freedesktop.appearance", "accent-color"));
    m_portalScheme = unwrap(read("org.freedesktop.appearance", "color-scheme")).toInt();
}

void Palette::portalSettingChanged(const QString &ns, const QString &key, const QDBusVariant &value)
{
    if (ns != QLatin1String("org.freedesktop.appearance")) return;
    if (key == QLatin1String("accent-color")) m_portalAccent = accentFrom(QVariant::fromValue(value));
    else if (key == QLatin1String("color-scheme")) m_portalScheme = unwrap(QVariant::fromValue(value)).toInt();
    else return;
    resolve();
}

void Palette::preferencesTouched()
{
    readPreferences();
    resolve();
}

void Palette::readPreferences()
{
    const QString path = preferencesPath();
    // Opal replaces the file atomically, so watch the folder and add the file again after each change.
    const QString dir = QFileInfo(path).absolutePath();
    if (QFileInfo::exists(dir) && !m_watcher.directories().contains(dir)) m_watcher.addPath(dir);
    if (QFileInfo::exists(path) && !m_watcher.files().contains(path)) m_watcher.addPath(path);
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) { m_hasOpal = false; m_prefs.clear(); return; }
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    // A partial write gives invalid JSON; keep the last good values until the next change.
    if (!doc.isObject()) return;
    m_prefs = doc.object().toVariantMap();
    m_hasOpal = true;
}

void Palette::resolve()
{
    QString forced = QString::fromUtf8(qgetenv("OPAL_THEME"));
    QString source;
    if (forced == QLatin1String("opal") || forced == QLatin1String("portal") || forced == QLatin1String("seed")) source = forced;
    else if (m_hasOpal) source = QStringLiteral("opal");
    else if (m_portalAccent.isValid()) source = QStringLiteral("portal");
    else source = QStringLiteral("seed");
    if (source == QLatin1String("opal") && !m_hasOpal) source = m_portalAccent.isValid() ? QStringLiteral("portal") : QStringLiteral("seed");
    if (source == QLatin1String("portal") && !m_portalAccent.isValid()) source = QStringLiteral("seed");

    // Light or dark: forced, then Opal, then the portal (1 is dark, 2 is light, 0 has no preference).
    const QString scheme = QString::fromUtf8(qgetenv("OPAL_COLOR_SCHEME"));
    bool light;
    if (scheme == QLatin1String("light") || scheme == QLatin1String("dark")) light = scheme == QLatin1String("light");
    else if (m_hasOpal && source == QLatin1String("opal")) light = m_prefs.value(QStringLiteral("light")).toBool();
    else light = m_portalScheme == 2;

    QColor seed = m_seed;
    QVariantMap overlay;
    if (source == QLatin1String("opal")) {
        const int index = std::clamp(m_prefs.value(QStringLiteral("palette")).toInt(), 0, 3);
        const OpalStatic &st = OpalPalettes[index];
        seed = QColor(QString::fromLatin1(st.seed));
        const QVariantMap dyn = m_prefs.value(QStringLiteral("dynamicPalette")).toMap().value(light ? QStringLiteral("light") : QStringLiteral("dark")).toMap();
        if (m_prefs.value(QStringLiteral("dynamicColors")).toBool() && !dyn.isEmpty()) {
            for (auto it = dyn.cbegin(); it != dyn.cend(); ++it) overlay.insert(it.key(), QColor(it.value().toString()));
            if (overlay.contains(QStringLiteral("primary"))) seed = overlay.value(QStringLiteral("primary")).value<QColor>();
        } else {
            // Match the shell's fallbacks for the static palettes so apps and shell look the same.
            auto c = [](const char *s) { return QColor(QString::fromLatin1(s)); };
            overlay.insert(QStringLiteral("primary"), light ? c(st.seed) : c(st.primary));
            overlay.insert(QStringLiteral("on_primary"), light ? QColor(Qt::white) : c(st.onPrimary));
            overlay.insert(QStringLiteral("primary_container"), light ? c(st.primary) : c(st.onPrimary));
            overlay.insert(QStringLiteral("on_primary_container"), light ? c(st.onPrimary) : c(st.primary));
            overlay.insert(QStringLiteral("background"), light ? c(st.light) : c(st.dark));
        }
    } else if (source == QLatin1String("portal")) {
        seed = m_portalAccent;
    }
    QVariantMap roles = opal::schemeFromSeed(seed, light);
    for (auto it = overlay.cbegin(); it != overlay.cend(); ++it) roles.insert(it.key(), it.value());

    const QString envMotion = QString::fromUtf8(qgetenv("OPAL_REDUCE_MOTION"));
    const bool reduce = envMotion == QLatin1String("1") || (m_hasOpal && m_prefs.value(QStringLiteral("reduceMotion")).toBool());
    const QString mode = m_hasOpal ? m_prefs.value(QStringLiteral("mode")).toString() : QString();

    m_roles = roles;
    m_light = light;
    m_reduceMotion = reduce;
    m_source = source;
    m_opalMode = mode;
    emit changed();
}
