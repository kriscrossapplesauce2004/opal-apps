#include "terminalhelpers.h"

#include <QClipboard>
#include <QDesktopServices>
#include <QGuiApplication>
#include <QDir>
#include <QFile>
#include <QFontDatabase>
#include <QRegularExpression>
#include <QUrl>
#include <pwd.h>
#include <unistd.h>

QString TerminalHelpers::homePath() const { return QDir::homePath(); }

QString TerminalHelpers::userShell() const
{
    const QString env = qEnvironmentVariable("SHELL");
    if (!env.isEmpty()) return env;
    if (const passwd *pw = getpwuid(getuid()); pw && pw->pw_shell) return QString::fromLocal8Bit(pw->pw_shell);
    return QStringLiteral("/bin/sh");
}

QString TerminalHelpers::startDirectory() const { return qEnvironmentVariable("OPAL_TERMINAL_CWD"); }

QVariantList TerminalHelpers::sshHosts() const
{
    QVariantList out;
    QFile f(QDir::homePath() + QStringLiteral("/.ssh/config"));
    if (!f.open(QIODevice::ReadOnly)) return out;
    QVariantMap cur;
    auto flush = [&] { if (!cur.isEmpty()) out << cur; cur.clear(); };
    for (QString line : QString::fromUtf8(f.readAll()).split(QLatin1Char('\n'))) {
        line = line.trimmed();
        if (line.isEmpty() || line.startsWith(QLatin1Char('#'))) continue;
        const QStringList parts = line.split(QRegularExpression(QStringLiteral("[\\s=]+")), Qt::SkipEmptyParts);
        if (parts.size() < 2) continue;
        const QString key = parts[0].toLower();
        if (key == QLatin1String("host")) {
            flush();
            // Each pattern without wildcards is its own entry; the first one names the block.
            const QString name = parts[1];
            if (!name.contains(QLatin1Char('*')) && !name.contains(QLatin1Char('?')) && !name.startsWith(QLatin1Char('!')))
                cur = {{QStringLiteral("name"), name}, {QStringLiteral("host"), name}, {QStringLiteral("user"), QString()}};
        } else if (!cur.isEmpty() && key == QLatin1String("hostname")) cur[QStringLiteral("host")] = parts[1];
        else if (!cur.isEmpty() && key == QLatin1String("user")) cur[QStringLiteral("user")] = parts[1];
    }
    flush();
    return out;
}

QStringList TerminalHelpers::monospaceFonts() const
{
    QStringList out;
    for (const QString &f : QFontDatabase::families())
        if (QFontDatabase::isFixedPitch(f) && !f.startsWith(QLatin1Char('.'))) out << f;
    return out;
}

bool TerminalHelpers::openFile(const QString &path) const
{
    return QDesktopServices::openUrl(QUrl::fromLocalFile(path));
}

void TerminalHelpers::copyText(const QString &text) const
{
    QGuiApplication::clipboard()->setText(text);
}
