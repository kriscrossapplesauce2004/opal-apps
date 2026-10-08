#include "systembridge.h"

#include <QClipboard>
#include <QGuiApplication>
#include <QMimeData>
#include <QProcess>
#include <QStandardPaths>
#include <QUrl>

namespace {
const char *const GnomeFiles = "x-special/gnome-copied-files";
// Terminals tried in order when neither opal-terminal nor $TERMINAL is available, with the flag that sets the folder.
struct Term { const char *bin; const char *dirFlag; };
const Term Terminals[] = {{"kitty", "--directory"}, {"foot", "--working-directory"}, {"ghostty", "--working-directory="}, {"alacritty", "--working-directory"},
                          {"wezterm", "start --cwd"}, {"konsole", "--workdir"}, {"gnome-terminal", "--working-directory="}, {"xterm", ""}};
}

SystemBridge::SystemBridge(QObject *parent) : QObject(parent)
{
    connect(QGuiApplication::clipboard(), &QClipboard::dataChanged, this, &SystemBridge::readClipboard);
    readClipboard();
}

void SystemBridge::readClipboard()
{
    const QMimeData *md = QGuiApplication::clipboard()->mimeData();
    QStringList uris;
    bool cut = false;
    if (md && md->hasFormat(QString::fromLatin1(GnomeFiles))) {
        // The GNOME format starts with "copy" or "cut", then one URI per line.
        const QStringList lines = QString::fromUtf8(md->data(QString::fromLatin1(GnomeFiles))).split(QLatin1Char('\n'), Qt::SkipEmptyParts);
        if (!lines.isEmpty()) {
            cut = lines.first().trimmed() == QLatin1String("cut");
            uris = lines.mid(1);
        }
    } else if (md && md->hasUrls()) {
        for (const QUrl &u : md->urls())
            uris << u.toString();
    }
    if (uris == m_uris && cut == m_cut)
        return;
    m_uris = uris;
    m_cut = cut;
    emit clipboardChanged();
}

void SystemBridge::setClipboard(const QStringList &uris, bool cut)
{
    auto *md = new QMimeData;
    QList<QUrl> urls;
    QStringList paths;
    for (const QString &u : uris) {
        const QUrl url(u);
        urls << url;
        paths << (url.isLocalFile() ? url.toLocalFile() : u);
    }
    md->setUrls(urls);
    md->setData(QString::fromLatin1(GnomeFiles), (QString::fromLatin1(cut ? "cut\n" : "copy\n") + uris.join(QLatin1Char('\n'))).toUtf8());
    md->setText(paths.join(QLatin1Char('\n')));
    // KDE reads this marker to know the files were cut.
    if (cut)
        md->setData(QStringLiteral("application/x-kde-cutselection"), "1");
    QGuiApplication::clipboard()->setMimeData(md);
}

void SystemBridge::clearClipboard()
{
    QGuiApplication::clipboard()->clear();
}

void SystemBridge::copyText(const QString &text)
{
    QGuiApplication::clipboard()->setText(text);
}

QString SystemBridge::terminalName() const
{
    if (!QStandardPaths::findExecutable(QStringLiteral("opal-terminal")).isEmpty())
        return QStringLiteral("Opal Terminal");
    const QString env = qEnvironmentVariable("TERMINAL");
    if (!env.isEmpty() && !QStandardPaths::findExecutable(env).isEmpty())
        return env;
    for (const Term &t : Terminals)
        if (!QStandardPaths::findExecutable(QString::fromLatin1(t.bin)).isEmpty())
            return QString::fromLatin1(t.bin);
    return QString();
}

bool SystemBridge::openTerminal(const QString &folderPath)
{
    if (!QStandardPaths::findExecutable(QStringLiteral("opal-terminal")).isEmpty())
        return QProcess::startDetached(QStringLiteral("opal-terminal"), {QStringLiteral("--working-directory"), folderPath});
    const QString env = qEnvironmentVariable("TERMINAL");
    // An unknown $TERMINAL gets no flag; the working directory of the process is the folder instead.
    if (!env.isEmpty() && !QStandardPaths::findExecutable(env).isEmpty())
        return QProcess::startDetached(env, {}, folderPath);
    for (const Term &t : Terminals) {
        const QString bin = QString::fromLatin1(t.bin);
        if (QStandardPaths::findExecutable(bin).isEmpty())
            continue;
        QStringList args;
        const QString flag = QString::fromLatin1(t.dirFlag);
        if (flag.endsWith(QLatin1Char('=')))
            args << flag + folderPath;
        else if (!flag.isEmpty())
            args << flag.split(QLatin1Char(' ')) << folderPath;
        return QProcess::startDetached(bin, args, folderPath);
    }
    return false;
}
