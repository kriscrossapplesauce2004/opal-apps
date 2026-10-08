#include "terminalconfig.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QStandardPaths>
#include <QTimer>

namespace {
QString keyOf(const QString &section, const QString &key) { return section + QLatin1Char('/') + key; }

const char *DefaultConfig = R"(# Opal Terminal configuration. Changes apply live when you save.
# Colours: "opal" follows the Opal palette; or a built-in theme (catppuccin-mocha, gruvbox-dark, tokyo-night,
# rose-pine, dracula, nord, solarized-dark, catppuccin-latte), or any theme you imported in Appearance.

[theme]
name = opal

[font]
family = CaskaydiaCove Nerd Font Mono
size = 14
line-height = 1.2

[window]
padding = 14
# 1 is solid; lower values let the wallpaper or background image through.
opacity = 1
blur = 0
background-image =
background-dim = 0.6

[cursor]
# block, bar or underline
style = block
blink = true
trail = false

[effects]
# none, crt, bloom, edge-glow, or a path to your own .frag shader
shader = none

[shell]
# Empty uses $SHELL.
program =
scrollback = 10000
command-marks = true

[keys]
# Extra key row on touch screens, left to right.
key-row = Esc, Tab, Ctrl, Alt, |, ~, /, -, Arrows

[snippets]
Build and test = cmake --build build && ctest --test-dir build
Update system = sudo pacman -Syu

# Saved sessions appear in the sessions sidebar. Example:
# [profile Home server]
# command = ssh user@nas.local
# icon = dns
)";
}

TerminalConfig::TerminalConfig(QObject *parent) : QObject(parent)
{
    m_path = qEnvironmentVariable("OPAL_TERMINAL_CONFIG");
    if (m_path.isEmpty())
        m_path = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + QStringLiteral("/opal/terminal.conf");
    if (!QFileInfo::exists(m_path))
        writeDefault();
    // Editors replace the file on save, so the folder is watched too and the file is added again after each change.
    connect(&m_watcher, &QFileSystemWatcher::fileChanged, this, [this] { if (!m_saving) QTimer::singleShot(60, this, &TerminalConfig::reload); });
    connect(&m_watcher, &QFileSystemWatcher::directoryChanged, this, [this] { if (!m_saving) QTimer::singleShot(60, this, &TerminalConfig::reload); });
    reload();
}

void TerminalConfig::writeDefault()
{
    QDir().mkpath(QFileInfo(m_path).absolutePath());
    QSaveFile f(m_path);
    if (f.open(QIODevice::WriteOnly)) {
        f.write(DefaultConfig);
        f.commit();
    }
}

void TerminalConfig::reload()
{
    const QString dir = QFileInfo(m_path).absolutePath();
    if (!m_watcher.directories().contains(dir) && QFileInfo::exists(dir))
        m_watcher.addPath(dir);
    if (!m_watcher.files().contains(m_path) && QFileInfo::exists(m_path))
        m_watcher.addPath(m_path);
    QFile f(m_path);
    if (!f.open(QIODevice::ReadOnly))
        return;
    const QString content = QString::fromUtf8(f.readAll());
    QHash<QString, QString> before = m_values;
    parse(content);
    if (before != m_values || m_revision == 0) {
        ++m_revision;
        emit changed();
    }
}

void TerminalConfig::parse(const QString &content)
{
    m_lines.clear();
    m_values.clear();
    QString section;
    for (const QString &raw : content.split(QLatin1Char('\n'))) {
        Line line{raw, section, QString()};
        const QString t = raw.trimmed();
        if (t.startsWith(QLatin1Char('[')) && t.endsWith(QLatin1Char(']'))) {
            section = t.mid(1, t.size() - 2).trimmed();
            line.section = section;
        } else if (!t.isEmpty() && !t.startsWith(QLatin1Char('#')) && !t.startsWith(QLatin1Char(';'))) {
            const int eq = t.indexOf(QLatin1Char('='));
            if (eq > 0) {
                line.key = t.left(eq).trimmed();
                m_values.insert(keyOf(section, line.key), t.mid(eq + 1).trimmed());
            }
        }
        m_lines << line;
    }
    if (!m_lines.isEmpty() && m_lines.last().text.isEmpty())
        m_lines.removeLast();
}

QVariant TerminalConfig::get(const QString &section, const QString &key, const QVariant &fallback) const
{
    const auto it = m_values.constFind(keyOf(section, key));
    return it == m_values.constEnd() ? fallback : QVariant(*it);
}

QString TerminalConfig::str(const char *section, const char *key, const QString &fallback) const
{
    const auto it = m_values.constFind(keyOf(QString::fromLatin1(section), QString::fromLatin1(key)));
    return it == m_values.constEnd() || it->isEmpty() ? fallback : *it;
}

qreal TerminalConfig::num(const char *section, const char *key, qreal fallback) const
{
    bool ok = false;
    const qreal v = str(section, key, QString()).toDouble(&ok);
    return ok ? v : fallback;
}

bool TerminalConfig::flag(const char *section, const char *key, bool fallback) const
{
    const QString v = str(section, key, QString()).toLower();
    if (v == QLatin1String("true") || v == QLatin1String("yes") || v == QLatin1String("on") || v == QLatin1String("1")) return true;
    if (v == QLatin1String("false") || v == QLatin1String("no") || v == QLatin1String("off") || v == QLatin1String("0")) return false;
    return fallback;
}

void TerminalConfig::set(const QString &section, const QString &key, const QVariant &value)
{
    const QString v = value.typeId() == QMetaType::Bool ? (value.toBool() ? QStringLiteral("true") : QStringLiteral("false")) : value.toString();
    if (get(section, key).toString() == v && m_values.contains(keyOf(section, key)))
        return;
    int lastInSection = -1;
    bool done = false;
    for (int i = 0; i < m_lines.size(); ++i) {
        Line &l = m_lines[i];
        if (l.section == section) lastInSection = i;
        if (l.section == section && l.key == key) {
            // Keep the original indentation and key spelling; only the value changes.
            const int eq = l.text.indexOf(QLatin1Char('='));
            l.text = l.text.left(eq + 1) + QLatin1Char(' ') + v;
            done = true;
            break;
        }
    }
    if (!done) {
        if (lastInSection < 0) {
            m_lines << Line{QString(), QString(), QString()} << Line{QStringLiteral("[%1]").arg(section), section, QString()};
            lastInSection = m_lines.size() - 1;
        }
        m_lines.insert(lastInSection + 1, Line{key + QStringLiteral(" = ") + v, section, key});
    }
    m_values.insert(keyOf(section, key), v);
    save();
    ++m_revision;
    emit changed();
}

void TerminalConfig::save()
{
    m_saving = true;
    QSaveFile f(m_path);
    if (f.open(QIODevice::WriteOnly)) {
        QStringList out;
        for (const Line &l : std::as_const(m_lines)) out << l.text;
        f.write((out.join(QLatin1Char('\n')) + QLatin1Char('\n')).toUtf8());
        f.commit();
    }
    // The watcher sees our own write; the flag clears after it has passed.
    QTimer::singleShot(200, this, [this] { m_saving = false; if (!m_watcher.files().contains(m_path)) m_watcher.addPath(m_path); });
}

QStringList TerminalConfig::keyRow() const
{
    QStringList out;
    for (const QString &k : str("keys", "key-row", QStringLiteral("Esc, Tab, Ctrl, Alt, |, ~, /, -, Arrows")).split(QLatin1Char(',')))
        if (!k.trimmed().isEmpty()) out << k.trimmed();
    return out;
}

QVariantList TerminalConfig::snippets() const
{
    QVariantList out;
    for (const Line &l : m_lines)
        if (l.section == QLatin1String("snippets") && !l.key.isEmpty())
            out << QVariantMap{{QStringLiteral("name"), l.key}, {QStringLiteral("command"), m_values.value(keyOf(l.section, l.key))}};
    return out;
}

QVariantList TerminalConfig::profiles() const
{
    QVariantList out;
    QStringList seen;
    for (const Line &l : m_lines) {
        if (!l.section.startsWith(QLatin1String("profile ")) || seen.contains(l.section))
            continue;
        seen << l.section;
        const QString s = l.section;
        out << QVariantMap{{QStringLiteral("name"), s.mid(8).trimmed()},
                           {QStringLiteral("command"), m_values.value(keyOf(s, QStringLiteral("command")))},
                           {QStringLiteral("cwd"), m_values.value(keyOf(s, QStringLiteral("cwd")))},
                           {QStringLiteral("icon"), m_values.value(keyOf(s, QStringLiteral("icon")), QStringLiteral("terminal"))}};
    }
    return out;
}
