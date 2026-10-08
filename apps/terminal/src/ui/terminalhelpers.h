#pragma once
#include <QObject>
#include <QQmlEngine>
#include <QStringList>
#include <QVariantList>

// Small helpers for the Terminal interface.
// QML: singleton "TerminalHelpers". sshHosts() -> [{name, host, user}] from ~/.ssh/config (wildcards skipped);
// monospaceFonts() -> installed fixed-pitch families; openFile(path) opens a file with the default app;
// copyText(text) puts text on the clipboard; homePath; userShell (the program a new session runs when terminal.conf does not set one).
class TerminalHelpers : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(QString homePath READ homePath CONSTANT)
    Q_PROPERTY(QString userShell READ userShell CONSTANT)
    Q_PROPERTY(QString startDirectory READ startDirectory CONSTANT)
public:
    explicit TerminalHelpers(QObject *parent = nullptr) : QObject(parent) {}
    QString homePath() const;
    QString userShell() const;
    // From --working-directory or $OPAL_TERMINAL_CWD, else empty (the home folder).
    QString startDirectory() const;
    Q_INVOKABLE QVariantList sshHosts() const;
    Q_INVOKABLE QStringList monospaceFonts() const;
    Q_INVOKABLE bool openFile(const QString &path) const;
    Q_INVOKABLE void copyText(const QString &text) const;
};
