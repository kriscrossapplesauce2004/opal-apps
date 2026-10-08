#include "foldermodel.h"
#include "glibloop.h"
#include "thumbnailer.h"

#include <QCryptographicHash>
#include <QDir>
#include <QImage>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>

class TstThumbnailer : public QObject
{
    Q_OBJECT
    QTemporaryDir m_cache;

private slots:
    void initTestCase()
    {
        GlibLoop::checkDispatcher();
        QVERIFY(m_cache.isValid());
        qputenv("XDG_CACHE_HOME", m_cache.path().toUtf8());
    }
    void followsSpec();
    void failMarker();
    void onlyVisibleRows();
};

void TstThumbnailer::followsSpec()
{
    QTemporaryDir dir;
    const QString img = dir.filePath(QStringLiteral("big picture.png"));
    QImage src(900, 600, QImage::Format_RGB32);
    src.fill(Qt::darkGreen);
    QVERIFY(src.save(img));
    const QString uri = QUrl::fromLocalFile(img).toString(QUrl::FullyEncoded);

    auto *t = Thumbnailer::instance();
    QSignalSpy ready(t, &Thumbnailer::ready);
    t->request(uri, 256);
    QVERIFY(ready.wait(5000));

    // MD5 of the canonical URI names the file, in the bucket folder.
    const QString md5 = QString::fromLatin1(QCryptographicHash::hash(uri.toUtf8(), QCryptographicHash::Md5).toHex());
    const QString path = m_cache.path() + QStringLiteral("/thumbnails/large/") + md5 + QStringLiteral(".png");
    QCOMPARE(Thumbnailer::thumbnailPath(uri, 256), path);
    QVERIFY(QFile::exists(path));
    QCOMPARE(QFileInfo(path).permissions() & (QFile::ReadGroup | QFile::WriteGroup | QFile::ReadOther | QFile::WriteOther | QFile::ExeOwner), QFileDevice::Permissions());
    QVERIFY(QFileInfo(path).permission(QFile::ReadOwner | QFile::WriteOwner));
    QImage th(path);
    QVERIFY(!th.isNull());
    QCOMPARE(th.text(QStringLiteral("Thumb::URI")), uri);
    QCOMPARE(th.text(QStringLiteral("Thumb::MTime")).toLongLong(), QFileInfo(img).lastModified().toSecsSinceEpoch());
    QVERIFY(th.width() <= 256 && th.height() <= 256);
    QCOMPARE(th.width(), 256);
    QVERIFY(qAbs(th.height() - 171) <= 1);
    // The folders are private, as the spec asks.
    QCOMPARE(QFileInfo(m_cache.path() + QStringLiteral("/thumbnails/large")).permissions() & (QFile::ReadGroup | QFile::ReadOther), QFileDevice::Permissions());

    // A changed file makes the stored thumbnail stale and a new one replaces it.
    QTest::qSleep(1100);
    QImage changed(300, 300, QImage::Format_RGB32);
    changed.fill(Qt::red);
    QVERIFY(changed.save(img));
    QImage again = t->loadOrGenerate(uri, 256);
    QCOMPARE(again.width(), 256);
    QCOMPARE(QImage(path).text(QStringLiteral("Thumb::MTime")).toLongLong(), QFileInfo(img).lastModified().toSecsSinceEpoch());
}

void TstThumbnailer::failMarker()
{
    QTemporaryDir dir;
    const QString bad = dir.filePath(QStringLiteral("broken.png"));
    QFile f(bad);
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write("not a png");
    f.close();
    const QString uri = QUrl::fromLocalFile(bad).toString(QUrl::FullyEncoded);
    auto *t = Thumbnailer::instance();
    QSignalSpy failed(t, &Thumbnailer::failed);
    t->request(uri, 128);
    QVERIFY(failed.wait(5000));
    const QString fail = Thumbnailer::failPath(uri);
    QVERIFY(fail.contains(QStringLiteral("/thumbnails/fail/opal-files/")));
    QVERIFY(QFile::exists(fail));
    QVERIFY(!QFile::exists(Thumbnailer::thumbnailPath(uri, 128)));
}

void TstThumbnailer::onlyVisibleRows()
{
    QTemporaryDir dir;
    for (int i = 0; i < 12; ++i) {
        QImage im(64, 64, QImage::Format_RGB32);
        im.fill(Qt::blue);
        QVERIFY(im.save(dir.filePath(QStringLiteral("p%1.png").arg(i, 2, 10, QLatin1Char('0')))));
    }
    FolderModel m;
    m.setLocation(QUrl::fromLocalFile(dir.path()));
    QTRY_VERIFY(!m.loading() && m.count() == 12);
    m.requestThumbnails(2, 4);
    QTRY_VERIFY(m.data(m.index(3), FolderModel::ThumbnailRole).toUrl().isValid());
    QTRY_VERIFY(m.data(m.index(2), FolderModel::ThumbnailRole).toUrl().isValid());
    QTRY_VERIFY(m.data(m.index(4), FolderModel::ThumbnailRole).toUrl().isValid());
    QTest::qWait(200);
    // Rows outside the range never got a request.
    for (int i : {0, 1, 5, 11})
        QVERIFY(!m.data(m.index(i), FolderModel::ThumbnailRole).toUrl().isValid());
    QVERIFY(!QFile::exists(Thumbnailer::thumbnailPath(QUrl::fromLocalFile(dir.filePath(QStringLiteral("p00.png"))).toString(QUrl::FullyEncoded), 256)));
}

QTEST_MAIN(TstThumbnailer)
#include "tst_thumbnailer.moc"
