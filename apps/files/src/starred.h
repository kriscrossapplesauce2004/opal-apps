#pragma once
#include <QObject>
#include <QSet>
#include <QStringList>
#include <QtQml/qqmlregistration.h>

class QQmlEngine;
class QJSEngine;

// Starred files, shared by every model.
// QML: singleton "Starred". Properties: count (int), loaded (bool).
// Methods: isStarred(uri), star(uri), unstar(uri), toggle(uri), uris() -> list of strings.
// Signals: changed() after any change, including the first load.
// The list lives in $XDG_DATA_HOME/opal/files/starred.json and is written atomically on a worker thread.
class Starred : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(int count READ count NOTIFY changed)
    Q_PROPERTY(bool loaded READ loaded NOTIFY changed)

public:
    // Creates the store. An empty path selects the default location.
    explicit Starred(const QString &path = QString(), QObject *parent = nullptr);
    static Starred *instance();
    static Starred *create(QQmlEngine *, QJSEngine *);

    int count() const { return m_uris.size(); }
    bool loaded() const { return m_loaded; }
    QString path() const { return m_path; }

    Q_INVOKABLE bool isStarred(const QString &uri) const { return m_set.contains(uri); }
    Q_INVOKABLE void star(const QString &uri);
    Q_INVOKABLE void unstar(const QString &uri);
    Q_INVOKABLE void toggle(const QString &uri);
    Q_INVOKABLE QStringList uris() const { return m_uris; }

    // Waits for the first load; for tests only.
    bool waitForLoaded(int msecs = 3000) const;

signals:
    void changed();

private:
    void load();
    void save();
    QString m_path;
    QStringList m_uris;
    QSet<QString> m_set;
    bool m_loaded = false;
    bool m_dirtyBeforeLoad = false;
    int m_saveGeneration = 0;
};
