#include "terminalthemes.h"

#include <QColor>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QSaveFile>
#include <QStandardPaths>
#include <QUrl>

namespace {
struct Builtin { const char *name; const char *bg, *fg, *cursor, *sel; const char *ansi[16]; };
const Builtin Builtins[] = {
    {"catppuccin-mocha", "#1e1e2e", "#cdd6f4", "#f5e0dc", "#585b70", {"#45475a","#f38ba8","#a6e3a1","#f9e2af","#89b4fa","#f5c2e7","#94e2d5","#bac2de","#585b70","#f38ba8","#a6e3a1","#f9e2af","#89b4fa","#f5c2e7","#94e2d5","#a6adc8"}},
    {"catppuccin-latte", "#eff1f5", "#4c4f69", "#dc8a78", "#acb0be", {"#5c5f77","#d20f39","#40a02b","#df8e1d","#1e66f5","#ea76cb","#179299","#acb0be","#6c6f85","#d20f39","#40a02b","#df8e1d","#1e66f5","#ea76cb","#179299","#bcc0cc"}},
    {"gruvbox-dark", "#282828", "#ebdbb2", "#ebdbb2", "#504945", {"#282828","#cc241d","#98971a","#d79921","#458588","#b16286","#689d6a","#a89984","#928374","#fb4934","#b8bb26","#fabd2f","#83a598","#d3869b","#8ec07c","#ebdbb2"}},
    {"tokyo-night", "#1a1b26", "#c0caf5", "#c0caf5", "#33467c", {"#15161e","#f7768e","#9ece6a","#e0af68","#7aa2f7","#bb9af7","#7dcfff","#a9b1d6","#414868","#f7768e","#9ece6a","#e0af68","#7aa2f7","#bb9af7","#7dcfff","#c0caf5"}},
    {"rose-pine", "#191724", "#e0def4", "#524f67", "#403d52", {"#26233a","#eb6f92","#31748f","#f6c177","#9ccfd8","#c4a7e7","#ebbcba","#e0def4","#6e6a86","#eb6f92","#31748f","#f6c177","#9ccfd8","#c4a7e7","#ebbcba","#e0def4"}},
    {"dracula", "#282a36", "#f8f8f2", "#f8f8f2", "#44475a", {"#21222c","#ff5555","#50fa7b","#f1fa8c","#bd93f9","#ff79c6","#8be9fd","#f8f8f2","#6272a4","#ff6e6e","#69ff94","#ffffa5","#d6acff","#ff92df","#a4ffff","#ffffff"}},
    {"nord", "#2e3440", "#d8dee9", "#d8dee9", "#434c5e", {"#3b4252","#bf616a","#a3be8c","#ebcb8b","#81a1c1","#b48ead","#88c0d0","#e5e9f0","#4c566a","#bf616a","#a3be8c","#ebcb8b","#81a1c1","#b48ead","#8fbcbb","#eceff4"}},
    {"solarized-dark", "#002b36", "#839496", "#93a1a1", "#073642", {"#073642","#dc322f","#859900","#b58900","#268bd2","#d33682","#2aa198","#eee8d5","#002b36","#cb4b16","#586e75","#657b83","#839496","#6c71c4","#93a1a1","#fdf6e3"}},
};
const char *const AlacrittyNames[] = {"black", "red", "green", "yellow", "blue", "magenta", "cyan", "white"};

QVariantMap fromBuiltin(const Builtin &b)
{
    QVariantList ansi;
    for (const char *c : b.ansi) ansi << QColor(QString::fromLatin1(c));
    return {{QStringLiteral("background"), QColor(QString::fromLatin1(b.bg))}, {QStringLiteral("foreground"), QColor(QString::fromLatin1(b.fg))},
            {QStringLiteral("cursor"), QColor(QString::fromLatin1(b.cursor))}, {QStringLiteral("selection"), QColor(QString::fromLatin1(b.sel))},
            {QStringLiteral("ansi"), ansi}};
}

// Accepts "#rrggbb", "rrggbb", "0xrrggbb" and quoted forms.
QColor colourOf(QString v)
{
    v = v.trimmed();
    v.remove(QLatin1Char('"')).remove(QLatin1Char('\''));
    if (v.startsWith(QLatin1String("0x"))) v = v.mid(2);
    if (!v.startsWith(QLatin1Char('#')) && QRegularExpression(QStringLiteral("^[0-9a-fA-F]{6}$")).match(v).hasMatch()) v.prepend(QLatin1Char('#'));
    const QColor c(v);
    return c.isValid() ? c : QColor();
}

QVariantMap summary(const QString &name, bool builtin, const QVariantMap &p)
{
    const QVariantList a = p.value(QStringLiteral("ansi")).toList();
    QVariantList accents;
    for (int i : {1, 2, 3, 4, 5, 6}) accents << a.value(i);
    return {{QStringLiteral("name"), name}, {QStringLiteral("builtin"), builtin}, {QStringLiteral("background"), p.value(QStringLiteral("background"))},
            {QStringLiteral("foreground"), p.value(QStringLiteral("foreground"))}, {QStringLiteral("accents"), accents}};
}
}

TerminalThemes::TerminalThemes(QObject *parent) : QObject(parent)
{
    m_dir = qEnvironmentVariable("OPAL_TERMINAL_THEMES");
    if (m_dir.isEmpty())
        m_dir = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + QStringLiteral("/opal/terminal-themes");
    scan();
}

void TerminalThemes::scan()
{
    m_imported.clear();
    const QFileInfoList files = QDir(m_dir).entryInfoList({QStringLiteral("*.theme")}, QDir::Files, QDir::Name);
    for (const QFileInfo &fi : files) {
        QFile f(fi.filePath());
        if (!f.open(QIODevice::ReadOnly)) continue;
        const QVariantMap p = parseTheme(QString::fromUtf8(f.readAll()));
        if (!p.isEmpty()) m_imported.insert(fi.completeBaseName(), p);
    }
}

QVariantList TerminalThemes::themes() const
{
    QVariantList out;
    for (const Builtin &b : Builtins) out << summary(QString::fromLatin1(b.name), true, fromBuiltin(b));
    for (auto it = m_imported.cbegin(); it != m_imported.cend(); ++it) out << summary(it.key(), false, it.value());
    return out;
}

QVariantMap TerminalThemes::palette(const QString &name) const
{
    for (const Builtin &b : Builtins)
        if (name == QLatin1String(b.name)) return fromBuiltin(b);
    return m_imported.value(name);
}

QVariantMap TerminalThemes::parseTheme(const QString &text)
{
    QColor bg, fg, cursor, sel;
    QVector<QColor> ansi(16);
    QString section;
    for (QString line : text.split(QLatin1Char('\n'))) {
        line = line.trimmed();
        if (line.isEmpty() || line.startsWith(QLatin1Char('#')) || line.startsWith(QLatin1Char(';'))) continue;
        if (line.startsWith(QLatin1Char('['))) { section = line.mid(1, line.indexOf(QLatin1Char(']')) - 1).trimmed().toLower(); continue; }
        // kitty uses "key value"; the others use "key = value".
        QString key, value;
        const int eq = line.indexOf(QLatin1Char('='));
        if (eq > 0) { key = line.left(eq).trimmed(); value = line.mid(eq + 1).trimmed(); }
        else { const int sp = line.indexOf(QRegularExpression(QStringLiteral("\\s"))); if (sp < 0) continue; key = line.left(sp); value = line.mid(sp).trimmed(); }
        key = key.toLower();
        const int hash = value.indexOf(QStringLiteral(" #"));
        if (hash > 0) value = value.left(hash).trimmed();
        if (key == QLatin1String("palette")) {
            // ghostty: palette = 3=#f9e2af
            const int e2 = value.indexOf(QLatin1Char('='));
            const int i = value.left(e2).trimmed().toInt();
            if (e2 > 0 && i >= 0 && i < 16) ansi[i] = colourOf(value.mid(e2 + 1));
            continue;
        }
        static const QRegularExpression colorN(QStringLiteral("^(?:color|regular|bright)(\\d+)$"));
        const auto m = colorN.match(key);
        if (m.hasMatch()) {
            int i = m.captured(1).toInt();
            if (key.startsWith(QLatin1String("bright")) && i < 8) i += 8;
            if (i >= 0 && i < 16) ansi[i] = colourOf(value);
            continue;
        }
        if (section == QLatin1String("colors.normal") || section == QLatin1String("colors.bright")) {
            for (int i = 0; i < 8; ++i)
                if (key == QLatin1String(AlacrittyNames[i])) ansi[i + (section.endsWith(QLatin1String("bright")) ? 8 : 0)] = colourOf(value);
            continue;
        }
        if (section == QLatin1String("colors.cursor") && key == QLatin1String("cursor")) { cursor = colourOf(value); continue; }
        if (section == QLatin1String("colors.selection") && key == QLatin1String("background")) { sel = colourOf(value); continue; }
        if (key == QLatin1String("background")) bg = colourOf(value);
        else if (key == QLatin1String("foreground")) fg = colourOf(value);
        else if (key == QLatin1String("cursor") || key == QLatin1String("cursor-color") || key == QLatin1String("cursor_color")) {
            // foot writes "cursor = <text> <cursor>"; the second colour is the cursor.
            const QStringList parts = value.split(QRegularExpression(QStringLiteral("\\s+")), Qt::SkipEmptyParts);
            cursor = colourOf(parts.size() > 1 ? parts.last() : value);
        }
        else if (key == QLatin1String("selection") || key == QLatin1String("selection_background") || key == QLatin1String("selection-background")) sel = colourOf(value);
    }
    int found = 0;
    for (const QColor &c : ansi) if (c.isValid()) ++found;
    if (!bg.isValid() || !fg.isValid() || found < 8)
        return {};
    // Missing bright colours repeat the normal ones.
    for (int i = 8; i < 16; ++i) if (!ansi[i].isValid()) ansi[i] = ansi[i - 8];
    QVariantList list;
    for (const QColor &c : ansi) list << c;
    return {{QStringLiteral("background"), bg}, {QStringLiteral("foreground"), fg},
            {QStringLiteral("cursor"), cursor.isValid() ? cursor : fg}, {QStringLiteral("selection"), sel.isValid() ? sel : ansi[8]},
            {QStringLiteral("ansi"), list}};
}

QString TerminalThemes::importFile(const QString &pathOrUrl)
{
    const QString path = pathOrUrl.startsWith(QLatin1String("file:")) ? QUrl(pathOrUrl).toLocalFile() : pathOrUrl;
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) { m_error = tr("Cannot open %1").arg(path); emit themesChanged(); return {}; }
    const QVariantMap p = parseTheme(QString::fromUtf8(f.readAll()));
    if (p.isEmpty()) { m_error = tr("No terminal colours found in %1").arg(QFileInfo(path).fileName()); emit themesChanged(); return {}; }
    QString name = QFileInfo(path).completeBaseName().toLower().replace(QRegularExpression(QStringLiteral("[^a-z0-9]+")), QStringLiteral("-"));
    if (name.isEmpty() || name == QLatin1String("theme") || name == QLatin1String("colors")) name = QFileInfo(path).dir().dirName().toLower();
    QDir().mkpath(m_dir);
    QSaveFile out(m_dir + QLatin1Char('/') + name + QStringLiteral(".theme"));
    if (!out.open(QIODevice::WriteOnly)) { m_error = tr("Cannot write the theme"); emit themesChanged(); return {}; }
    QStringList lines{QStringLiteral("# Imported from %1").arg(path)};
    lines << QStringLiteral("background = %1").arg(p.value(QStringLiteral("background")).value<QColor>().name())
          << QStringLiteral("foreground = %1").arg(p.value(QStringLiteral("foreground")).value<QColor>().name())
          << QStringLiteral("cursor = %1").arg(p.value(QStringLiteral("cursor")).value<QColor>().name())
          << QStringLiteral("selection = %1").arg(p.value(QStringLiteral("selection")).value<QColor>().name());
    const QVariantList a = p.value(QStringLiteral("ansi")).toList();
    for (int i = 0; i < a.size(); ++i) lines << QStringLiteral("color%1 = %2").arg(i).arg(a[i].value<QColor>().name());
    out.write((lines.join(QLatin1Char('\n')) + QLatin1Char('\n')).toUtf8());
    out.commit();
    m_error.clear();
    scan();
    emit themesChanged();
    return name;
}

bool TerminalThemes::remove(const QString &name)
{
    if (!m_imported.contains(name)) return false;
    QFile::remove(m_dir + QLatin1Char('/') + name + QStringLiteral(".theme"));
    scan();
    emit themesChanged();
    return true;
}
