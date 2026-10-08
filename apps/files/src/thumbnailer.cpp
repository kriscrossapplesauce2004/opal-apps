#include "thumbnailer.h"

#include "gobj.h"

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImageReader>
#include <QMimeDatabase>
#include <QProcess>
#include <QQmlEngine>
#include <QQuickAsyncImageProvider>
#include <QQuickImageResponse>
#include <QQuickTextureFactory>
#include <QStandardPaths>
#include <QTemporaryFile>
#include <QtConcurrent/QtConcurrentRun>
#ifdef OPAL_HAVE_POPPLER
#include <poppler-qt6.h>
#endif

namespace {

QString bucketDir(int bucket)
{
    return bucket <= 128 ? QStringLiteral("normal") : bucket <= 256 ? QStringLiteral("large") : QStringLiteral("x-large");
}

QString ffmpegThumbnailer()
{
    static const QString path = QStandardPaths::findExecutable(QStringLiteral("ffmpegthumbnailer"));
    return path;
}

void ownerOnly(const QString &dir)
{
    QFile::setPermissions(dir, QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner);
}

// Makes a directory chain below the cache root with mode 0700, as the spec asks.
void makePrivateDir(const QString &dir)
{
    if (QFileInfo::exists(dir))
        return;
    QDir().mkpath(dir);
    const QString root = Thumbnailer::cacheRoot();
    QString cur = dir;
    while (cur.startsWith(root) && cur.size() >= root.size()) {
        ownerOnly(cur);
        const int slash = cur.lastIndexOf(QLatin1Char('/'));
        if (slash <= 0 || cur == root)
            break;
        cur.truncate(slash);
    }
}

// Reads a thumbnail file and checks its Thumb::MTime against the source file.
QImage readValid(const QString &thumb, qint64 mtime)
{
    QImage img;
    if (!QFileInfo::exists(thumb) || !img.load(thumb, "PNG"))
        return {};
    if (img.text(QStringLiteral("Thumb::MTime")).toLongLong() != mtime)
        return {};
    return img;
}

bool savePng(QImage img, const QString &finalPath, const QString &uri, qint64 mtime, qint64 fileSize)
{
    makePrivateDir(QFileInfo(finalPath).absolutePath());
    img.setText(QStringLiteral("Thumb::URI"), uri);
    img.setText(QStringLiteral("Thumb::MTime"), QString::number(mtime));
    img.setText(QStringLiteral("Thumb::Size"), QString::number(fileSize));
    img.setText(QStringLiteral("Software"), QStringLiteral("Opal Files"));
    // QTemporaryFile is created with mode 0600 and renamed into place, so readers never see a partial file.
    QTemporaryFile tmp(QFileInfo(finalPath).absolutePath() + QStringLiteral("/.opal-XXXXXX.png"));
    if (!tmp.open() || !img.save(&tmp, "PNG"))
        return false;
    tmp.close();
    // Without this the temporary file object would delete the renamed file when it dies.
    tmp.setAutoRemove(false);
    QFile::remove(finalPath);
    if (tmp.rename(finalPath))
        return true;
    QFile::remove(tmp.fileName());
    return false;
}

QImage renderImage(const QString &path, int bucket)
{
    QImageReader r(path);
    r.setAutoTransform(true);
    if (!r.canRead())
        return {};
    const QSize full = r.size();
    if (full.isValid() && (full.width() > bucket || full.height() > bucket))
        r.setScaledSize(full.scaled(bucket, bucket, Qt::KeepAspectRatio));
    QImage img = r.read();
    if (img.isNull())
        return {};
    if (img.width() > bucket || img.height() > bucket)
        img = img.scaled(bucket, bucket, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    return img;
}

QImage renderVideo(const QString &path, int bucket)
{
    const QString tool = ffmpegThumbnailer();
    if (tool.isEmpty())
        return {};
    QTemporaryFile out(QDir::tempPath() + QStringLiteral("/opal-files-video-XXXXXX.png"));
    if (!out.open())
        return {};
    out.close();
    QProcess p;
    p.start(tool, {QStringLiteral("-i"), path, QStringLiteral("-o"), out.fileName(), QStringLiteral("-s"), QString::number(bucket),
                   QStringLiteral("-q"), QStringLiteral("8")});
    if (!p.waitForFinished(20000)) {
        p.kill();
        p.waitForFinished(1000);
        return {};
    }
    QImage img;
    if (p.exitStatus() != QProcess::NormalExit || p.exitCode() != 0 || !img.load(out.fileName()))
        return {};
    return img;
}

QImage renderPdf(const QString &path, int bucket)
{
#ifdef OPAL_HAVE_POPPLER
    auto doc = Poppler::Document::load(path);
    if (!doc || doc->isLocked() || doc->numPages() < 1)
        return {};
    std::unique_ptr<Poppler::Page> page(doc->page(0));
    if (!page)
        return {};
    const QSizeF pts = page->pageSizeF();
    if (pts.isEmpty())
        return {};
    const double dpi = 72.0 * bucket / std::max(pts.width(), pts.height());
    QImage img = page->renderToImage(dpi, dpi);
    return img;
#else
    Q_UNUSED(path)
    Q_UNUSED(bucket)
    return {};
#endif
}

// Response of the image://thumb provider. The decode runs on the thumbnailer pool.
class ThumbResponse : public QQuickImageResponse
{
public:
    ThumbResponse(const QString &uri, int size)
    {
        auto *t = Thumbnailer::instance();
        QtConcurrent::run(t->pool(), [t, uri, size] { return t->loadOrGenerate(uri, size); }).then(this, [this](const QImage &img) {
            m_image = img;
            if (img.isNull())
                m_error = QStringLiteral("No thumbnail");
            emit finished();
        });
    }
    QQuickTextureFactory *textureFactory() const override { return QQuickTextureFactory::textureFactoryForImage(m_image); }
    QString errorString() const override { return m_error; }

private:
    QImage m_image;
    QString m_error;
};

class ThumbProvider : public QQuickAsyncImageProvider
{
public:
    QQuickImageResponse *requestImageResponse(const QString &id, const QSize &requestedSize) override
    {
        const int edge = std::max(requestedSize.width(), requestedSize.height());
        return new ThumbResponse(Thumbnailer::uriFromId(id), edge > 0 ? edge : 256);
    }
};

}

Thumbnailer::Thumbnailer(QObject *parent)
    : QObject(parent)
{
    // Four threads keep a folder of photos from starving the machine.
    m_pool.setMaxThreadCount(4);
}

Thumbnailer *Thumbnailer::instance()
{
    static Thumbnailer *t = new Thumbnailer(QCoreApplication::instance());
    return t;
}

Thumbnailer *Thumbnailer::create(QQmlEngine *engine, QJSEngine *)
{
    auto *t = instance();
    QJSEngine::setObjectOwnership(t, QJSEngine::CppOwnership);
    attach(engine);
    return t;
}

void Thumbnailer::attach(QQmlEngine *engine)
{
    if (engine && !engine->imageProvider(QStringLiteral("thumb")))
        engine->addImageProvider(QStringLiteral("thumb"), new ThumbProvider);
}

QString Thumbnailer::cacheRoot()
{
    return QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation) + QStringLiteral("/thumbnails");
}

int Thumbnailer::bucket(int size)
{
    return size <= 128 ? 128 : size <= 256 ? 256 : 512;
}

QString Thumbnailer::canonicalUri(const QString &uri)
{
    QString u = uri;
    if (u.startsWith(QLatin1Char('/')))
        return Gio::uriOf(Gio::fileForPath(u).get());
    // Qt may hand over a decoded form; encode it again before GIO parses it.
    if (!u.isEmpty() && u != QString::fromLatin1(QUrl(u, QUrl::TolerantMode).toEncoded()))
        u = QString::fromLatin1(QUrl(u, QUrl::TolerantMode).toEncoded());
    auto f = Gio::fileForUri(u);
    return f ? Gio::uriOf(f.get()) : u;
}

QString Thumbnailer::md5Of(const QString &canonicalUri)
{
    return QString::fromLatin1(QCryptographicHash::hash(canonicalUri.toUtf8(), QCryptographicHash::Md5).toHex());
}

QString Thumbnailer::thumbnailPath(const QString &uri, int size)
{
    return cacheRoot() + QLatin1Char('/') + bucketDir(bucket(size)) + QLatin1Char('/') + md5Of(canonicalUri(uri)) + QStringLiteral(".png");
}

QString Thumbnailer::failPath(const QString &uri)
{
    return cacheRoot() + QStringLiteral("/fail/opal-files/") + md5Of(canonicalUri(uri)) + QStringLiteral(".png");
}

QUrl Thumbnailer::url(const QString &uri, qint64 mtimeSecs)
{
    return QUrl(QStringLiteral("image://thumb/") + QString::fromLatin1(QUrl(uri, QUrl::TolerantMode).toEncoded()) + QStringLiteral("?v=") + QString::number(mtimeSecs));
}

QString Thumbnailer::uriFromId(const QString &id)
{
    const int q = id.lastIndexOf(QStringLiteral("?v="));
    return q >= 0 ? id.left(q) : id;
}

bool Thumbnailer::videoSupported() const { return !ffmpegThumbnailer().isEmpty(); }

bool Thumbnailer::pdfSupported() const
{
#ifdef OPAL_HAVE_POPPLER
    return true;
#else
    return false;
#endif
}

void Thumbnailer::request(const QString &uriIn, int size)
{
    const QString uri = canonicalUri(uriIn);
    const int b = bucket(size);
    const QString key = uri + QLatin1Char('|') + QString::number(b);
    if (uri.isEmpty() || m_inflight.contains(key))
        return;
    m_inflight.insert(key);
    QPointer<Thumbnailer> self(this);
    QtConcurrent::run(&m_pool, [this, uri, b] { return !loadOrGenerate(uri, b).isNull(); }).then(this, [self, uri, b, key](bool ok) {
        if (!self)
            return;
        self->m_inflight.remove(key);
        if (ok)
            emit self->ready(uri, b);
        else
            emit self->failed(uri);
    });
}

QImage Thumbnailer::loadOrGenerate(const QString &uriIn, int size)
{
    const QString uri = canonicalUri(uriIn);
    auto file = Gio::fileForUri(uri);
    // GIO gives a FUSE path for gvfs mounts, so remote files work when gvfs-fuse runs.
    const QString path = file ? Gio::pathOf(file.get()) : QString();
    if (path.isEmpty())
        return {};
    const QFileInfo fi(path);
    if (!fi.isFile())
        return {};
    const qint64 mtime = fi.lastModified().toSecsSinceEpoch();
    const int b = bucket(size);

    // A larger thumbnail on disk also serves a smaller request.
    for (int s = b; s <= 512; s *= 2) {
        QImage cached = readValid(cacheRoot() + QLatin1Char('/') + bucketDir(s) + QLatin1Char('/') + md5Of(uri) + QStringLiteral(".png"), mtime);
        if (!cached.isNull())
            return cached;
    }
    const QString fail = failPath(uri);
    if (QFileInfo::exists(fail)) {
        QImage marker;
        if (marker.load(fail, "PNG") && marker.text(QStringLiteral("Thumb::MTime")).toLongLong() == mtime)
            return {};
    }
    // The cache itself never gets thumbnails.
    if (path.startsWith(cacheRoot()))
        return {};

    const QString mime = QMimeDatabase().mimeTypeForFile(path, QMimeDatabase::MatchExtension).name();
    QImage img;
    if (mime.startsWith(QLatin1String("image/")))
        img = renderImage(path, b);
    else if (mime.startsWith(QLatin1String("video/")))
        img = renderVideo(path, b);
    else if (mime == QLatin1String("application/pdf"))
        img = renderPdf(path, b);

    if (img.isNull()) {
        // A 1x1 marker records the failure so the next listing does not retry.
        QImage marker(1, 1, QImage::Format_ARGB32);
        marker.fill(Qt::transparent);
        savePng(marker, fail, uri, mtime, fi.size());
        return {};
    }
    if (img.width() > b || img.height() > b)
        img = img.scaled(b, b, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    savePng(img, cacheRoot() + QLatin1Char('/') + bucketDir(b) + QLatin1Char('/') + md5Of(uri) + QStringLiteral(".png"), uri, mtime, fi.size());
    return img;
}
