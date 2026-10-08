#pragma once
#include <QFileSystemWatcher>
#include <QObject>
#include <QQmlEngine>
#include <QVariantMap>

// The user's terminal.conf: an INI-style file with comments, read on start and again on every change (live reload).
// QML: singleton "TerminalConfig". Values: get(section, key, fallback); typed properties below for the common ones.
// set(section, key, value) edits the line in place (comments and order stay) or adds it to the section.
// Lists: snippets [{name, command}] from [snippets]; profiles [{name, command, cwd, icon}] from [profile NAME] sections;
// keyRow (phone extra keys) from [keys] key-row. OPAL_TERMINAL_CONFIG overrides the path.
class TerminalConfig : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(QString path READ path CONSTANT)
    Q_PROPERTY(int revision READ revision NOTIFY changed)
    Q_PROPERTY(QString theme READ theme NOTIFY changed)
    Q_PROPERTY(QString fontFamily READ fontFamily NOTIFY changed)
    Q_PROPERTY(qreal fontSize READ fontSize NOTIFY changed)
    Q_PROPERTY(qreal lineHeight READ lineHeight NOTIFY changed)
    Q_PROPERTY(qreal padding READ padding NOTIFY changed)
    Q_PROPERTY(qreal opacity READ opacity NOTIFY changed)
    Q_PROPERTY(qreal blur READ blur NOTIFY changed)
    Q_PROPERTY(QString backgroundImage READ backgroundImage NOTIFY changed)
    Q_PROPERTY(qreal backgroundDim READ backgroundDim NOTIFY changed)
    Q_PROPERTY(QString cursorStyle READ cursorStyle NOTIFY changed)
    Q_PROPERTY(bool cursorBlink READ cursorBlink NOTIFY changed)
    Q_PROPERTY(bool cursorTrail READ cursorTrail NOTIFY changed)
    Q_PROPERTY(QString shader READ shader NOTIFY changed)
    Q_PROPERTY(QString shell READ shell NOTIFY changed)
    Q_PROPERTY(int scrollback READ scrollback NOTIFY changed)
    Q_PROPERTY(bool commandMarks READ commandMarks NOTIFY changed)
    Q_PROPERTY(QStringList keyRow READ keyRow NOTIFY changed)
    Q_PROPERTY(QVariantList snippets READ snippets NOTIFY changed)
    Q_PROPERTY(QVariantList profiles READ profiles NOTIFY changed)
public:
    explicit TerminalConfig(QObject *parent = nullptr);
    QString path() const { return m_path; }
    int revision() const { return m_revision; }

    Q_INVOKABLE QVariant get(const QString &section, const QString &key, const QVariant &fallback = {}) const;
    Q_INVOKABLE void set(const QString &section, const QString &key, const QVariant &value);
    Q_INVOKABLE void reload();

    QString theme() const { return str("theme", "name", QStringLiteral("opal")); }
    QString fontFamily() const { return str("font", "family", QStringLiteral("monospace")); }
    qreal fontSize() const { return num("font", "size", 14); }
    qreal lineHeight() const { return num("font", "line-height", 1.2); }
    qreal padding() const { return num("window", "padding", 14); }
    qreal opacity() const { return num("window", "opacity", 1.0); }
    qreal blur() const { return num("window", "blur", 0); }
    QString backgroundImage() const { return str("window", "background-image", QString()); }
    qreal backgroundDim() const { return num("window", "background-dim", 0.6); }
    QString cursorStyle() const { return str("cursor", "style", QStringLiteral("block")); }
    bool cursorBlink() const { return flag("cursor", "blink", true); }
    bool cursorTrail() const { return flag("cursor", "trail", false); }
    QString shader() const { return str("effects", "shader", QStringLiteral("none")); }
    QString shell() const { return str("shell", "program", QString()); }
    int scrollback() const { return int(num("shell", "scrollback", 10000)); }
    bool commandMarks() const { return flag("shell", "command-marks", true); }
    QStringList keyRow() const;
    QVariantList snippets() const;
    QVariantList profiles() const;

signals:
    void changed();

private:
    struct Line { QString text; QString section; QString key; };
    QString str(const char *section, const char *key, const QString &fallback) const;
    qreal num(const char *section, const char *key, qreal fallback) const;
    bool flag(const char *section, const char *key, bool fallback) const;
    void parse(const QString &content);
    void save();
    void writeDefault();

    QString m_path;
    QList<Line> m_lines;
    QHash<QString, QString> m_values;
    QFileSystemWatcher m_watcher;
    int m_revision = 0;
    bool m_saving = false;
};
