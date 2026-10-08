#pragma once
#include <QHash>
#include <QImage>
#include <QObject>
#include <QSet>
#include <QThreadPool>
#include <QUrl>
#include <QtQml/qqmlregistration.h>

class QQmlEngine;
class QJSEngine;

// Freedesktop thumbnail engine, shared with Photos.
// QML: singleton "Thumbnailer". Properties: videoSupported, pdfSupported (bool, constant).
// Methods: request(uri, size = 256) queues one thumbnail; url(uri, mtimeSecs) returns the image://thumb URL.
// Signals: ready(uri, size) when the thumbnail file exists and image://thumb/<uri> will load at once; failed(uri).
// Cache layout follows the thumbnail spec: $XDG_CACHE_HOME/thumbnails/{normal,large,x-large}/<md5 of URI>.png with
// Thumb::URI and Thumb::MTime text chunks, mode 0600. Failures go to thumbnails/fail/opal-files/.
// All disk and decode work runs on a private pool of at most four threads.
class Thumbnailer : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(bool videoSupported READ videoSupported CONSTANT)
    Q_PROPERTY(bool pdfSupported READ pdfSupported CONSTANT)

public:
    explicit Thumbnailer(QObject *parent = nullptr);
    static Thumbnailer *instance();
    static Thumbnailer *create(QQmlEngine *, QJSEngine *);
    // Registers image://thumb on an engine, once. Models call this when they are created in QML.
    static void attach(QQmlEngine *engine);

    static QString cacheRoot();
    // Returns the 128, 256 or 512 bucket for a requested edge length.
    static int bucket(int size);
    static QString canonicalUri(const QString &uri);
    static QString md5Of(const QString &canonicalUri);
    static QString thumbnailPath(const QString &uri, int size);
    static QString failPath(const QString &uri);
    // Returns the image://thumb URL for a file. The mtime keeps the QML image cache honest.
    Q_INVOKABLE static QUrl url(const QString &uri, qint64 mtimeSecs = 0);
    // Splits an image://thumb id back into the URI.
    static QString uriFromId(const QString &id);

    bool videoSupported() const;
    bool pdfSupported() const;

    Q_INVOKABLE void request(const QString &uri, int size = 256);

    // Blocking. Returns the cached thumbnail or makes it. Call from a worker thread only.
    QImage loadOrGenerate(const QString &uri, int size);
    QThreadPool *pool() { return &m_pool; }

signals:
    void ready(const QString &uri, int size);
    void failed(const QString &uri);

private:
    QThreadPool m_pool;
    QSet<QString> m_inflight;
};
