#include "foldermodel.h"
#include "format.h"
#include "glibloop.h"

#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>

static void touchFile(const QString &path, qint64 size = 0)
{
    QFile f(path);
    QVERIFY(f.open(QIODevice::WriteOnly));
    if (size)
        f.write(QByteArray(int(size), 'x'));
}

static QStringList names(FolderModel &m)
{
    QStringList out;
    for (int i = 0; i < m.rowCount(); ++i)
        out << m.data(m.index(i), FolderModel::NameRole).toString();
    return out;
}

class TstFolderModel : public QObject
{
    Q_OBJECT
private slots:
    void initTestCase() { GlibLoop::checkDispatcher(); }
    void dispatcherIsGlib();
    void naturalSortThousandFiles();
    void foldersFirst();
    void sortRoles();
    void liveUpdates();
    void filters();
    void selection();
    void humanFormatting();
    void childCountAndRoles();
    void hundredThousand();
};

void TstFolderModel::dispatcherIsGlib()
{
    QVERIFY(GlibLoop::checkDispatcher());
    QVERIFY(!GlibLoop::usesFallbackPump());
}

void TstFolderModel::naturalSortThousandFiles()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    for (int i = 0; i < 1000; ++i)
        touchFile(dir.filePath(QStringLiteral("file%1.txt").arg(i)));
    touchFile(dir.filePath(QStringLiteral("File0b.txt")));
    FolderModel m;
    m.setLocation(QUrl::fromLocalFile(dir.path()));
    QTRY_VERIFY_WITH_TIMEOUT(!m.loading() && m.count() == 1001, 10000);
    const QStringList n = names(m);
    QCOMPARE(n.value(0), QStringLiteral("file0.txt"));
    // 2 sorts before 10 and 100: natural, not lexical.
    QVERIFY(n.indexOf(QStringLiteral("file2.txt")) < n.indexOf(QStringLiteral("file10.txt")));
    QVERIFY(n.indexOf(QStringLiteral("file10.txt")) < n.indexOf(QStringLiteral("file100.txt")));
    QVERIFY(n.indexOf(QStringLiteral("file99.txt")) < n.indexOf(QStringLiteral("file100.txt")));
    // Case-insensitive: "File0b" lands next to "file0", not at the end.
    QVERIFY(n.indexOf(QStringLiteral("File0b.txt")) < n.indexOf(QStringLiteral("file1.txt")));
    for (int i = 0; i < 1000; ++i)
        QCOMPARE(m.rowOfName(QStringLiteral("file%1.txt").arg(i)) >= 0, true);
    m.setSortOrder(Qt::DescendingOrder);
    QCOMPARE(names(m).value(0), QStringLiteral("file999.txt"));
}

void TstFolderModel::foldersFirst()
{
    QTemporaryDir dir;
    touchFile(dir.filePath(QStringLiteral("aaa.txt")));
    touchFile(dir.filePath(QStringLiteral("zzz.txt")));
    QDir().mkdir(dir.filePath(QStringLiteral("mmm")));
    QDir().mkdir(dir.filePath(QStringLiteral("bbb")));
    FolderModel m;
    m.setLocation(QUrl::fromLocalFile(dir.path()));
    QTRY_VERIFY(!m.loading() && m.count() == 4);
    QCOMPARE(names(m), (QStringList{"bbb", "mmm", "aaa.txt", "zzz.txt"}));
    m.setFoldersFirst(false);
    QCOMPARE(names(m), (QStringList{"aaa.txt", "bbb", "mmm", "zzz.txt"}));
    m.setFoldersFirst(true);
    m.setSortOrder(Qt::DescendingOrder);
    QCOMPARE(names(m), (QStringList{"mmm", "bbb", "zzz.txt", "aaa.txt"}));
}

void TstFolderModel::sortRoles()
{
    QTemporaryDir dir;
    touchFile(dir.filePath(QStringLiteral("big.bin")), 5000);
    touchFile(dir.filePath(QStringLiteral("small.bin")), 10);
    touchFile(dir.filePath(QStringLiteral("mid.txt")), 500);
    QFile(dir.filePath(QStringLiteral("big.bin"))).setFileTime(QDateTime::currentDateTime().addDays(-3), QFileDevice::FileModificationTime);
    FolderModel m;
    m.setLocation(QUrl::fromLocalFile(dir.path()));
    QTRY_VERIFY(!m.loading() && m.count() == 3);
    m.setSortRole(FolderModel::Size);
    QCOMPARE(names(m), (QStringList{"small.bin", "mid.txt", "big.bin"}));
    m.setSortRole(FolderModel::Modified);
    QCOMPARE(names(m).first(), QStringLiteral("big.bin"));
    QCOMPARE(m.totalSize(), 5510);
}

void TstFolderModel::liveUpdates()
{
    QTemporaryDir dir;
    touchFile(dir.filePath(QStringLiteral("a.txt")));
    touchFile(dir.filePath(QStringLiteral("c.txt")));
    FolderModel m;
    m.setLocation(QUrl::fromLocalFile(dir.path()));
    QTRY_VERIFY(!m.loading() && m.count() == 2);
    QTRY_VERIFY_WITH_TIMEOUT(m.live(), 5000);

    int resets = 0;
    connect(&m, &QAbstractItemModel::modelReset, this, [&] { ++resets; });
    QSignalSpy inserted(&m, &QAbstractItemModel::rowsInserted);
    QSignalSpy removed(&m, &QAbstractItemModel::rowsRemoved);

    touchFile(dir.filePath(QStringLiteral("b.txt")));
    QTRY_COMPARE_WITH_TIMEOUT(m.count(), 3, 5000);
    QCOMPARE(names(m), (QStringList{"a.txt", "b.txt", "c.txt"}));
    QCOMPARE(inserted.count(), 1);

    m.select(1);
    QVERIFY(QFile::rename(dir.filePath(QStringLiteral("b.txt")), dir.filePath(QStringLiteral("d.txt"))));
    QTRY_VERIFY_WITH_TIMEOUT(names(m) == (QStringList{"a.txt", "c.txt", "d.txt"}), 5000);
    // The selection follows a rename.
    QTRY_COMPARE(m.selectedCount(), 1);
    QCOMPARE(m.data(m.index(2), FolderModel::SelectedRole).toBool(), true);

    QVERIFY(QFile::remove(dir.filePath(QStringLiteral("a.txt"))));
    QTRY_COMPARE_WITH_TIMEOUT(m.count(), 2, 5000);
    QVERIFY(removed.count() >= 1);
    QCOMPARE(resets, 0);

    // Growing a file updates its size without a reset.
    QSignalSpy changed(&m, &QAbstractItemModel::dataChanged);
    touchFile(dir.filePath(QStringLiteral("c.txt")), 2048);
    QTRY_VERIFY_WITH_TIMEOUT(m.data(m.index(0), FolderModel::SizeRole).toLongLong() == 2048, 5000);
    QCOMPARE(resets, 0);
}

void TstFolderModel::filters()
{
    QTemporaryDir dir;
    touchFile(dir.filePath(QStringLiteral("photo.png")));
    touchFile(dir.filePath(QStringLiteral("photo2.jpg")));
    touchFile(dir.filePath(QStringLiteral("notes.txt")));
    touchFile(dir.filePath(QStringLiteral("movie.mp4")));
    touchFile(dir.filePath(QStringLiteral(".hidden")));
    QDir().mkdir(dir.filePath(QStringLiteral("pictures")));
    FolderModel m;
    m.setLocation(QUrl::fromLocalFile(dir.path()));
    QTRY_VERIFY(!m.loading() && m.count() == 5);
    m.setShowHidden(true);
    QCOMPARE(m.count(), 6);
    m.setShowHidden(false);

    m.setFilterKind({QStringLiteral("image")});
    QCOMPARE(names(m), (QStringList{"photo.png", "photo2.jpg"}));
    m.setFilterKind({QStringLiteral("image"), QStringLiteral("folder")});
    QCOMPARE(names(m), (QStringList{"pictures", "photo.png", "photo2.jpg"}));
    m.setFilterKind({QStringLiteral("video")});
    QCOMPARE(names(m), (QStringList{"movie.mp4"}));
    m.setFilterKind({});
    QCOMPARE(m.count(), 5);

    m.setNameFilter(QStringLiteral("PHOTO"));
    QCOMPARE(names(m), (QStringList{"photo.png", "photo2.jpg"}));
    m.setNameFilter(QStringLiteral("*.txt"));
    QCOMPARE(names(m), (QStringList{"notes.txt"}));
    m.setNameFilter(QStringLiteral("zzz"));
    QCOMPARE(m.count(), 0);
    m.setNameFilter({});
    QCOMPARE(m.count(), 5);
}

void TstFolderModel::selection()
{
    QTemporaryDir dir;
    touchFile(dir.filePath(QStringLiteral("a")), 100);
    touchFile(dir.filePath(QStringLiteral("b")), 200);
    touchFile(dir.filePath(QStringLiteral("c")), 300);
    touchFile(dir.filePath(QStringLiteral("d")), 400);
    QDir().mkdir(dir.filePath(QStringLiteral("dir")));
    FolderModel m;
    m.setLocation(QUrl::fromLocalFile(dir.path()));
    QTRY_VERIFY(!m.loading() && m.count() == 5);
    QCOMPARE(m.rowOfName(QStringLiteral("dir")), 0);
    QSignalSpy spy(&m, &EntryModel::selectionChanged);

    m.select(1);
    QCOMPARE(m.selectedCount(), 1);
    QCOMPARE(m.selectedSize(), 100);
    m.select(3, EntryModel::Toggle);
    QCOMPARE(m.selectedCount(), 2);
    QCOMPARE(m.selectedSize(), 400);
    m.select(4, EntryModel::Range);
    // The anchor is row 3, so the range is rows 3 and 4.
    QCOMPARE(m.selectedCount(), 2);
    QCOMPARE(m.selectedSize(), 700);
    m.select(1, EntryModel::Range);
    QCOMPARE(m.selectedCount(), 3);
    QCOMPARE(m.selectedSize(), 600);
    m.select(3, EntryModel::Toggle);
    QCOMPARE(m.selectedCount(), 2);
    m.selectAll();
    QCOMPARE(m.selectedCount(), 5);
    QCOMPARE(m.selectedSize(), 1000);
    QCOMPARE(m.selectedUris().size(), 5);
    QCOMPARE(m.selectedPaths().size(), 5);
    QVERIFY(m.selectedPaths().contains(dir.filePath(QStringLiteral("a"))));
    m.invertSelection();
    QCOMPARE(m.selectedCount(), 0);
    m.select(2);
    m.clearSelection();
    QCOMPARE(m.selectedCount(), 0);
    QCOMPARE(m.selectedSize(), 0);
    QVERIFY(spy.count() > 3);

    // A filter drops hidden rows from the selection.
    m.selectAll();
    m.setNameFilter(QStringLiteral("a"));
    QCOMPARE(m.selectedCount(), m.count());
}

void TstFolderModel::humanFormatting()
{
    const QLocale c = QLocale::c();
    QCOMPARE(Fmt::humanSize(0, c), QStringLiteral("0 B"));
    QCOMPARE(Fmt::humanSize(999, c), QStringLiteral("999 B"));
    QCOMPARE(Fmt::humanSize(2000, c), QStringLiteral("2 KB"));
    QCOMPARE(Fmt::humanSize(880000, c), QStringLiteral("880 KB"));
    QCOMPARE(Fmt::humanSize(4800000, c), QStringLiteral("4.8 MB"));
    QCOMPARE(Fmt::humanSize(212000000, c), QStringLiteral("212 MB"));
    QCOMPARE(Fmt::humanSize(1200000000LL, c), QStringLiteral("1.2 GB"));
    QCOMPARE(Fmt::humanSize(412000000000LL, c), QStringLiteral("412 GB"));

    const QDateTime now(QDate(2026, 10, 8), QTime(15, 0));
    QCOMPARE(Fmt::humanDate(QDateTime(QDate(2026, 10, 8), QTime(14, 2)), now, c), QStringLiteral("Today 14:02"));
    QCOMPARE(Fmt::humanDate(QDateTime(QDate(2026, 10, 7), QTime(9, 0)), now, c), QStringLiteral("Yesterday"));
    QCOMPARE(Fmt::humanDate(QDateTime(QDate(2026, 10, 5), QTime(22, 41)), now, c), QStringLiteral("Mon 22:41"));
    QCOMPARE(Fmt::humanDate(QDateTime(QDate(2026, 10, 2), QTime(1, 0)), now, c), QStringLiteral("2 Oct"));
    QCOMPARE(Fmt::humanDate(QDateTime(QDate(2024, 9, 27), QTime(1, 0)), now, c), QStringLiteral("27 Sep 2024"));
    QCOMPARE(Fmt::itemCount(1, c), QStringLiteral("1 item"));
    QCOMPARE(Fmt::itemCount(38, c), QStringLiteral("38 items"));
    QCOMPARE(Fmt::humanEta(40), QStringLiteral("40 s left"));
    QCOMPARE(Fmt::humanEta(125), QStringLiteral("2 min left"));
    QCOMPARE(Fmt::humanEta(3900), QStringLiteral("1 h 5 min left"));

    QCOMPARE(Fmt::kindString(Fmt::kindFor(QStringLiteral("image/png"), false)), QStringLiteral("image"));
    QCOMPARE(Fmt::kindString(Fmt::kindFor(QStringLiteral("application/zip"), false)), QStringLiteral("archive"));
    QCOMPARE(Fmt::kindString(Fmt::kindFor(QStringLiteral("text/x-csrc"), false)), QStringLiteral("code"));
    QCOMPARE(Fmt::kindString(Fmt::kindFor(QStringLiteral("text/plain"), false)), QStringLiteral("text"));
    QCOMPARE(Fmt::kindString(Fmt::kindFor(QStringLiteral("application/pdf"), false)), QStringLiteral("pdf"));
    QCOMPARE(Fmt::kindString(Fmt::kindFor(QStringLiteral("inode/directory"), true)), QStringLiteral("folder"));
}

void TstFolderModel::childCountAndRoles()
{
    QTemporaryDir dir;
    QDir().mkdir(dir.filePath(QStringLiteral("sub")));
    for (int i = 0; i < 38; ++i)
        touchFile(dir.filePath(QStringLiteral("sub/f%1").arg(i)));
    touchFile(dir.filePath(QStringLiteral("sub/.dot")));
    touchFile(dir.filePath(QStringLiteral("pic.png")), 10);
    FolderModel m;
    m.setLocation(QUrl::fromLocalFile(dir.path()));
    QTRY_VERIFY(!m.loading() && m.count() == 2);
    // The lazy count starts when the role is read and arrives later.
    QCOMPARE(m.data(m.index(0), FolderModel::ChildCountRole).toInt(), -1);
    QTRY_COMPARE_WITH_TIMEOUT(m.data(m.index(0), FolderModel::ChildCountRole).toInt(), 38, 5000);
    QCOMPARE(m.data(m.index(0), FolderModel::SizeTextRole).toString(), Fmt::itemCount(38));
    QCOMPARE(m.data(m.index(0), FolderModel::KindRole).toString(), QStringLiteral("folder"));
    QCOMPARE(m.data(m.index(1), FolderModel::KindRole).toString(), QStringLiteral("image"));
    QCOMPARE(m.data(m.index(1), FolderModel::ContentTypeRole).toString(), QStringLiteral("image/png"));
    QCOMPARE(m.data(m.index(1), FolderModel::SizeTextRole).toString(), QStringLiteral("10 B"));
    QCOMPARE(m.data(m.index(1), FolderModel::PathRole).toString(), dir.filePath(QStringLiteral("pic.png")));
    QVERIFY(m.data(m.index(1), FolderModel::UriRole).toString().startsWith(QLatin1String("file:///")));
    QVERIFY(!m.data(m.index(1), FolderModel::KindNameRole).toString().isEmpty());
    QVERIFY(!m.data(m.index(1), FolderModel::IconNameRole).toString().isEmpty());
    QVERIFY(m.data(m.index(1), FolderModel::ModifiedTextRole).toString().startsWith(QLatin1String("Today")));
    QCOMPARE(m.data(m.index(1), FolderModel::ThumbnailRole).toUrl(), QUrl());
    QCOMPARE(m.breadcrumbs().last().toMap().value(QStringLiteral("name")).toString(), QFileInfo(dir.path()).fileName());
    QCOMPARE(m.parentLocation(), QUrl::fromLocalFile(QFileInfo(dir.path()).absolutePath()));
    // A missing folder reports an error.
    FolderModel bad;
    bad.setLocation(QUrl::fromLocalFile(dir.filePath(QStringLiteral("nope"))));
    QTRY_VERIFY(!bad.loading());
    QVERIFY(!bad.error().isEmpty());
}

void TstFolderModel::hundredThousand()
{
    if (qEnvironmentVariableIsEmpty("OPAL_FILES_BIG"))
        QSKIP("Set OPAL_FILES_BIG=1 to run the 100000 entry test.");
    QTemporaryDir dir;
    for (int i = 0; i < 100000; ++i)
        touchFile(dir.filePath(QStringLiteral("f%1.dat").arg(i)));
    FolderModel m;
    QElapsedTimer t;
    t.start();
    m.setLocation(QUrl::fromLocalFile(dir.path()));
    // The first rows show up long before the load ends.
    QTRY_VERIFY_WITH_TIMEOUT(m.count() > 0, 5000);
    qInfo() << "first rows after" << t.elapsed() << "ms";
    QTRY_VERIFY_WITH_TIMEOUT(!m.loading(), 60000);
    qInfo() << "100000 entries loaded in" << t.elapsed() << "ms";
    QCOMPARE(m.count(), 100000);
}

QTEST_GUILESS_MAIN(TstFolderModel)
#include "tst_foldermodel.moc"
