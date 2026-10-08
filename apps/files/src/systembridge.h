#pragma once
#include <QObject>
#include <QQmlEngine>
#include <QStringList>

// Desktop integration for the Files UI: the system clipboard with file lists, and opening a terminal in a folder.
//
// QML: singleton "SystemBridge".
// Properties: clipboardUris (list), clipboardCut (bool, true when the files were cut), hasClipboardFiles.
// Methods: setClipboard(uris, cut) writes text/uri-list, x-special/gnome-copied-files and plain text, so Dolphin,
//   Nautilus and terminals understand it; clearClipboard(); copyText(text); openTerminal(folderPath) starts
//   opal-terminal, else $TERMINAL, else the first known terminal found in PATH; terminalName (what openTerminal will start).
class SystemBridge : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(QStringList clipboardUris READ clipboardUris NOTIFY clipboardChanged)
    Q_PROPERTY(bool clipboardCut READ clipboardCut NOTIFY clipboardChanged)
    Q_PROPERTY(bool hasClipboardFiles READ hasClipboardFiles NOTIFY clipboardChanged)
    Q_PROPERTY(QString terminalName READ terminalName CONSTANT)
public:
    explicit SystemBridge(QObject *parent = nullptr);

    QStringList clipboardUris() const { return m_uris; }
    bool clipboardCut() const { return m_cut; }
    bool hasClipboardFiles() const { return !m_uris.isEmpty(); }
    QString terminalName() const;

    Q_INVOKABLE void setClipboard(const QStringList &uris, bool cut);
    Q_INVOKABLE void clearClipboard();
    Q_INVOKABLE void copyText(const QString &text);
    Q_INVOKABLE bool openTerminal(const QString &folderPath);

signals:
    void clipboardChanged();

private:
    void readClipboard();
    QStringList m_uris;
    bool m_cut = false;
};
