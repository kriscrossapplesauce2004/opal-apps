#pragma once
#include <QObject>
#include <QQmlEngine>
#include <QVariantList>

// Terminal colour themes: built-ins plus themes imported from kitty, alacritty, foot and ghostty files.
// QML: singleton "TerminalThemes". Properties: themes (list of {name, builtin, background, foreground, accents[6]}),
//   themesDir. Methods: palette(name) -> {background, foreground, cursor, selection, ansi[16]} (empty map when unknown);
//   importFile(pathOrUrl) -> imported theme name, or "" with lastError set; remove(name) for imported themes.
// Imported themes are stored normalized as <themesDir>/<name>.theme (key = #rrggbb lines).
class TerminalThemes : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(QVariantList themes READ themes NOTIFY themesChanged)
    Q_PROPERTY(QString themesDir READ themesDir CONSTANT)
    Q_PROPERTY(QString lastError READ lastError NOTIFY themesChanged)
public:
    explicit TerminalThemes(QObject *parent = nullptr);
    QVariantList themes() const;
    QString themesDir() const { return m_dir; }
    QString lastError() const { return m_error; }
    Q_INVOKABLE QVariantMap palette(const QString &name) const;
    Q_INVOKABLE QString importFile(const QString &pathOrUrl);
    Q_INVOKABLE bool remove(const QString &name);
    // Parses theme text in any supported format; exposed for tests.
    static QVariantMap parseTheme(const QString &text);

signals:
    void themesChanged();

private:
    void scan();
    QString m_dir;
    QString m_error;
    QMap<QString, QVariantMap> m_imported;
};
