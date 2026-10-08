#pragma once
#include <QColor>
#include <QFileSystemWatcher>
#include <QObject>
#include <QQmlEngine>
#include <QVariantMap>

#include <QDBusVariant>

// Resolves the colour scheme: Opal's live palette, then the portal accent colour, then the app's seed.
class Palette : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(QVariantMap roles READ roles NOTIFY changed)
    Q_PROPERTY(bool light READ light NOTIFY changed)
    Q_PROPERTY(bool reduceMotion READ reduceMotion NOTIFY changed)
    Q_PROPERTY(QString source READ source NOTIFY changed)
    Q_PROPERTY(QString opalMode READ opalMode NOTIFY changed)
    Q_PROPERTY(QColor seed READ seed WRITE setSeed NOTIFY changed)
    Q_PROPERTY(QColor accent READ accent NOTIFY changed)
public:
    explicit Palette(QObject *parent = nullptr);
    static Palette *instance();
    static Palette *create(QQmlEngine *, QJSEngine *);

    QVariantMap roles() const { return m_roles; }
    bool light() const { return m_light; }
    bool reduceMotion() const { return m_reduceMotion; }
    QString source() const { return m_source; }
    QString opalMode() const { return m_opalMode; }
    QColor seed() const { return m_seed; }
    void setSeed(const QColor &seed);
    QColor accent() const { return m_portalAccent; }

    // Full M3 scheme from any seed, for content-tinted surfaces such as a player that follows its album art.
    Q_INVOKABLE QVariantMap schemeFor(const QColor &seed, bool light) const;

    // Path of Opal's preferences file; OPAL_PREFERENCES overrides it.
    static QString preferencesPath();

signals:
    void changed();

private slots:
    void portalSettingChanged(const QString &ns, const QString &key, const QDBusVariant &value);
    void preferencesTouched();

private:
    void readPortal();
    void readPreferences();
    void resolve();

    QFileSystemWatcher m_watcher;
    QVariantMap m_prefs;
    bool m_hasOpal = false;
    QColor m_portalAccent;
    int m_portalScheme = 0;
    QColor m_seed {0x73, 0x55, 0xad};
    QVariantMap m_roles;
    bool m_light = false;
    bool m_reduceMotion = false;
    QString m_source;
    QString m_opalMode;
};
