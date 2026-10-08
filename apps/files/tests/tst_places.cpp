#include "glibloop.h"
#include "places.h"

#include <QDir>
#include <QFile>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>

static QStringList column(Places &p, int role)
{
    QStringList out;
    for (int i = 0; i < p.rowCount(); ++i)
        out << p.data(p.index(i), role).toString();
    return out;
}

class TstPlaces : public QObject
{
    Q_OBJECT
private slots:
    void initTestCase() { GlibLoop::checkDispatcher(); }
    void fixedAndXdgDirs();
    void bookmarksReadWriteReorder();
    void trashAndNetworkRows();
    void storageInfo();
};

void TstPlaces::fixedAndXdgDirs()
{
    QTemporaryDir home, cfg;
    for (const char *d : {"Documents", "Downloads", "Music", "Pictures"})
        QDir().mkpath(home.filePath(QLatin1String(d)));
    QFile u(cfg.filePath(QStringLiteral("user-dirs.dirs")));
    QVERIFY(u.open(QIODevice::WriteOnly));
    u.write("XDG_DOCUMENTS_DIR=\"$HOME/Documents\"\nXDG_DOWNLOAD_DIR=\"$HOME/Downloads\"\nXDG_MUSIC_DIR=\"$HOME/Music\"\nXDG_PICTURES_DIR=\"$HOME/Pictures\"\nXDG_VIDEOS_DIR=\"$HOME/Videos\"\n");
    u.close();
    Places p;
    p.setHomePath(home.path());
    p.setConfigDir(cfg.path());
    QTRY_VERIFY(column(p, Places::NameRole).contains(QStringLiteral("Pictures")));
    const QStringList names = column(p, Places::NameRole);
    QCOMPARE(names.mid(0, 3), (QStringList{"Home", "Recent", "Starred"}));
    QVERIFY(names.contains(QStringLiteral("Documents")));
    QVERIFY(names.contains(QStringLiteral("Downloads")));
    QVERIFY(names.contains(QStringLiteral("Music")));
    // Videos is listed in the file but missing on disk, so it stays out.
    QVERIFY(!names.contains(QStringLiteral("Videos")));
    const int docs = int(names.indexOf(QStringLiteral("Documents")));
    QCOMPARE(p.data(p.index(docs), Places::SectionRole).toString(), QStringLiteral("folders"));
    QCOMPARE(p.data(p.index(docs), Places::PathRole).toString(), home.filePath(QStringLiteral("Documents")));
}

void TstPlaces::bookmarksReadWriteReorder()
{
    QTemporaryDir home, cfg;
    QDir().mkpath(home.filePath(QStringLiteral("Projects")));
    QDir().mkpath(home.filePath(QStringLiteral("Work")));
    QDir().mkpath(cfg.filePath(QStringLiteral("gtk-3.0")));
    const QString file = cfg.filePath(QStringLiteral("gtk-3.0/bookmarks"));
    QFile f(file);
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write(QUrl::fromLocalFile(home.filePath(QStringLiteral("Projects"))).toEncoded() + " Code\n");
    f.write("sftp://example.org/srv\n");
    f.close();

    Places p;
    p.setHomePath(home.path());
    p.setConfigDir(cfg.path());
    QCOMPARE(p.bookmarksFile(), file);
    QTRY_VERIFY(column(p, Places::NameRole).contains(QStringLiteral("Code")));
    QCOMPARE(column(p, Places::KindRole).count(QStringLiteral("bookmark")), 2);
    const int code = int(column(p, Places::NameRole).indexOf(QStringLiteral("Code")));
    QCOMPARE(p.data(p.index(code), Places::EditableRole).toBool(), true);

    p.addBookmark(home.filePath(QStringLiteral("Work")), QStringLiteral("Job"));
    QTRY_VERIFY(column(p, Places::NameRole).contains(QStringLiteral("Job")));
    QTRY_VERIFY(QFile::exists(file) && QFile(file).size() > 60);
    auto text = [&] { QFile r(file); return r.open(QIODevice::ReadOnly) ? QString::fromUtf8(r.readAll()) : QString(); };
    QTRY_VERIFY(text().contains(QStringLiteral(" Job\n")));
    QVERIFY(text().contains(QStringLiteral("sftp://example.org/srv")));

    // Move the last bookmark to the first bookmark row.
    const int job = int(column(p, Places::NameRole).indexOf(QStringLiteral("Job")));
    p.moveBookmark(job, code);
    QCOMPARE(column(p, Places::NameRole).value(code), QStringLiteral("Job"));
    QTRY_VERIFY(text().startsWith(QUrl::fromLocalFile(home.filePath(QStringLiteral("Work"))).toEncoded()));

    p.renameBookmark(QUrl::fromLocalFile(home.filePath(QStringLiteral("Work"))).toString(), QStringLiteral("Office"));
    QTRY_VERIFY(column(p, Places::NameRole).contains(QStringLiteral("Office")));
    p.removeBookmark(QUrl::fromLocalFile(home.filePath(QStringLiteral("Projects"))).toString());
    QTRY_VERIFY(!column(p, Places::NameRole).contains(QStringLiteral("Code")));
    QTRY_VERIFY(!text().contains(QStringLiteral("Code")));

    // A second model reads what the first one wrote.
    Places q;
    q.setHomePath(home.path());
    q.setConfigDir(cfg.path());
    QTRY_VERIFY(column(q, Places::NameRole).contains(QStringLiteral("Office")));
}

void TstPlaces::trashAndNetworkRows()
{
    Places p;
    const QStringList kinds = column(p, Places::KindRole);
    QVERIFY(kinds.contains(QStringLiteral("trash")));
    QVERIFY(kinds.contains(QStringLiteral("network-root")));
    QCOMPARE(kinds.last(), QStringLiteral("trash"));
    QTRY_VERIFY(p.trashCount() >= 0);
}

void TstPlaces::storageInfo()
{
    QTemporaryDir dir;
    Places p;
    QSignalSpy spy(&p, &Places::storageInfoReady);
    p.storageInfo(dir.path());
    QVERIFY(spy.wait(5000));
    QVERIFY(spy.first().at(1).toLongLong() > 0);
    QVERIFY(spy.first().at(2).toLongLong() >= 0);
    QVERIFY(spy.first().at(4).toString().contains(QStringLiteral("free of")));
}

QTEST_MAIN(TstPlaces)
#include "tst_places.moc"
