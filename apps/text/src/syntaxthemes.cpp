#include "syntaxthemes.h"

#include <KSyntaxHighlighting/Repository>
#include <KSyntaxHighlighting/Theme>
#include <QCoreApplication>
#include <QDir>
#include <QJSEngine>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QStandardPaths>

namespace {

QString themeDir()
{
    return QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation) + QStringLiteral("/opal/text/syntax");
}

QColor role(const QVariantMap &roles, const char *name, const QColor &fallback)
{
    const QColor c(roles.value(QString::fromLatin1(name)).toString());
    return c.isValid() ? c : fallback;
}

QColor mix(const QColor &a, const QColor &b, qreal t)
{
    return QColor::fromRgbF(a.redF() + (b.redF() - a.redF()) * t, a.greenF() + (b.greenF() - a.greenF()) * t, a.blueF() + (b.blueF() - a.blueF()) * t);
}

// Moves a colour's hue, keeping it readable on the background: strings lean green, numbers lean orange.
QColor hueShift(const QColor &base, int hue, bool light)
{
    QColor c = QColor::fromHsl(hue, std::max(110, base.hslSaturation()), light ? 90 : 175);
    return c;
}

}

SyntaxThemes::SyntaxThemes(QObject *parent)
    : QObject(parent), m_repo(std::make_unique<KSyntaxHighlighting::Repository>())
{
    m_repo->addCustomSearchPath(themeDir());
}

SyntaxThemes::~SyntaxThemes() = default;

SyntaxThemes *SyntaxThemes::instance()
{
    static SyntaxThemes *t = new SyntaxThemes(QCoreApplication::instance());
    return t;
}

SyntaxThemes *SyntaxThemes::create(QQmlEngine *, QJSEngine *)
{
    auto *t = instance();
    QJSEngine::setObjectOwnership(t, QJSEngine::CppOwnership);
    return t;
}

KSyntaxHighlighting::Repository &SyntaxThemes::repository()
{
    return *m_repo;
}

KSyntaxHighlighting::Theme SyntaxThemes::theme()
{
    if (!m_name.isEmpty()) {
        const auto t = m_repo->theme(m_name);
        if (t.isValid())
            return t;
    }
    return m_repo->defaultTheme(KSyntaxHighlighting::Repository::DarkTheme);
}

QByteArray SyntaxThemes::themeJson(const QString &name, const QVariantMap &roles, bool light)
{
    const QColor bg = role(roles, "surface_container_lowest", light ? QColor("#ffffff") : QColor("#101114"));
    const QColor text = role(roles, "on_surface", light ? QColor("#1b1b1f") : QColor("#e3e2e6"));
    const QColor sub = role(roles, "on_surface_variant", light ? QColor("#46464f") : QColor("#c6c5d0"));
    const QColor primary = role(roles, "primary", light ? QColor("#4355b9") : QColor("#bac3ff"));
    const QColor secondary = role(roles, "secondary", light ? QColor("#5b5d72") : QColor("#c3c5dd"));
    const QColor tertiary = role(roles, "tertiary", light ? QColor("#77536d") : QColor("#e6bad7"));
    const QColor error = role(roles, "error", light ? QColor("#ba1a1a") : QColor("#ffb4ab"));
    const QColor strings = hueShift(primary, 105, light);
    const QColor numbers = hueShift(primary, 28, light);
    const QColor comment = mix(sub, bg, .3);
    auto style = [](const QColor &c, bool bold = false, bool italic = false) {
        QJsonObject o{{QStringLiteral("text-color"), c.name()}};
        if (bold)
            o.insert(QStringLiteral("bold"), true);
        if (italic)
            o.insert(QStringLiteral("italic"), true);
        return o;
    };
    const QJsonObject styles{
        {QStringLiteral("Normal"), style(text)},
        {QStringLiteral("Keyword"), style(primary, true)},
        {QStringLiteral("ControlFlow"), style(primary, true)},
        {QStringLiteral("Function"), style(tertiary)},
        {QStringLiteral("Variable"), style(mix(text, secondary, .5))},
        {QStringLiteral("Operator"), style(sub)},
        {QStringLiteral("BuiltIn"), style(secondary, true)},
        {QStringLiteral("Extension"), style(tertiary)},
        {QStringLiteral("Preprocessor"), style(numbers)},
        {QStringLiteral("Attribute"), style(tertiary)},
        {QStringLiteral("Char"), style(strings)},
        {QStringLiteral("SpecialChar"), style(numbers)},
        {QStringLiteral("String"), style(strings)},
        {QStringLiteral("VerbatimString"), style(strings)},
        {QStringLiteral("SpecialString"), style(numbers)},
        {QStringLiteral("Import"), style(primary)},
        {QStringLiteral("DataType"), style(secondary)},
        {QStringLiteral("DecVal"), style(numbers)},
        {QStringLiteral("BaseN"), style(numbers)},
        {QStringLiteral("Float"), style(numbers)},
        {QStringLiteral("Constant"), style(numbers, true)},
        {QStringLiteral("Comment"), style(comment, false, true)},
        {QStringLiteral("Documentation"), style(comment, false, true)},
        {QStringLiteral("Annotation"), style(tertiary)},
        {QStringLiteral("CommentVar"), style(tertiary, false, true)},
        {QStringLiteral("RegionMarker"), style(primary)},
        {QStringLiteral("Information"), style(secondary)},
        {QStringLiteral("Warning"), style(numbers, true)},
        {QStringLiteral("Alert"), style(error, true)},
        {QStringLiteral("Error"), style(error)},
        {QStringLiteral("Others"), style(tertiary)},
    };
    const QJsonObject editor{
        {QStringLiteral("BackgroundColor"), bg.name()},
        {QStringLiteral("TextSelection"), mix(bg, primary, .3).name()},
        {QStringLiteral("CurrentLine"), mix(bg, primary, .07).name()},
        {QStringLiteral("LineNumbers"), comment.name()},
        {QStringLiteral("CurrentLineNumber"), text.name()},
        {QStringLiteral("SearchHighlight"), mix(bg, tertiary, .35).name()},
    };
    return QJsonDocument(QJsonObject{{QStringLiteral("metadata"), QJsonObject{{QStringLiteral("name"), name}, {QStringLiteral("revision"), 1}}},
                                     {QStringLiteral("text-styles"), styles}, {QStringLiteral("editor-colors"), editor}})
        .toJson(QJsonDocument::Compact);
}

void SyntaxThemes::apply(const QVariantMap &roles, bool light)
{
    const QString name = light ? QStringLiteral("Opal Light") : QStringLiteral("Opal Dark");
    const QByteArray json = themeJson(name, roles, light);
    if (json == m_last && name == m_name)
        return;
    QDir().mkpath(themeDir() + QStringLiteral("/themes"));
    QSaveFile f(themeDir() + QStringLiteral("/themes/") + name + QStringLiteral(".theme"));
    if (!f.open(QIODevice::WriteOnly) || f.write(json) < 0 || !f.commit())
        return;
    m_last = json;
    m_name = name;
    m_repo->reload();
    ++m_revision;
    emit changed();
}
