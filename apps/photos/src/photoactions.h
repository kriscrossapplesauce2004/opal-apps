#pragma once
#include <QObject>
#include <QStringList>
#include <QtQml/qqmlregistration.h>

// What the viewer and the selection bar do to photos outside the app: wallpaper, clipboard, Files, trash, rotate, import.
// QML: singleton "PhotoActions".
// Properties: busy, wallpaperTarget ("Opal", "your desktop" or "" when no way is known).
// Methods: setWallpaper(path), copy(paths) (the image for one photo, files for many), copyPath(paths), showInFiles(path),
//   openWith(path) (the system's default app), trash(paths), rotate(paths, quarterTurns), importPhotos(paths) (copies photos
//   from a phone or USB source into Pictures/Imported/<date>).
// Signals: done(message), failed(message), trashed(paths), imported(folder, count).
class PhotoActions : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(QString wallpaperTarget READ wallpaperTarget CONSTANT)

public:
    explicit PhotoActions(QObject *parent = nullptr);

    bool busy() const { return m_busy > 0; }
    QString wallpaperTarget() const;

    Q_INVOKABLE void setWallpaper(const QString &path);
    Q_INVOKABLE void copy(const QStringList &paths);
    Q_INVOKABLE void copyPath(const QStringList &paths);
    Q_INVOKABLE void showInFiles(const QString &path);
    Q_INVOKABLE void openWith(const QString &path);
    Q_INVOKABLE void trash(const QStringList &paths);
    Q_INVOKABLE void rotate(const QStringList &paths, int quarterTurns);
    Q_INVOKABLE void importPhotos(const QStringList &paths);

    // The folder an import of today goes to. Public for tests.
    static QString importFolder(const QString &pictures);
    // Where Opal's command lives, or "" when Opal is not installed.
    static QString opalCommand();

signals:
    void busyChanged();
    void done(const QString &message);
    void failed(const QString &message);
    void trashed(const QStringList &paths);
    void imported(const QString &folder, int count);

private:
    void begin();
    void end();
    void portalWallpaper(const QString &path);
    int m_busy = 0;
};
