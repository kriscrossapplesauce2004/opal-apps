#include "fileoperations.h"
#include "glibloop.h"

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QtTest>

static void writeFile(const QString &path, const QByteArray &data)
{
    QDir().mkpath(QFileInfo(path).absolutePath());
    QFile f(path);
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write(data);
}

static QByteArray readFile(const QString &path)
{
    QFile f(path);
    return f.open(QIODevice::ReadOnly) ? f.readAll() : QByteArray();
}

class TstFileOps : public QObject
{
    Q_OBJECT
private:
    static QString stateOf(FileOperations &ops, int id)
    {
        const int row = ops.operations()->indexOfId(id);
        return row < 0 ? QString() : ops.operations()->data(ops.operations()->index(row), OperationsModel::StateRole).toString();
    }
    static double progressOf(FileOperations &ops, int id)
    {
        const int row = ops.operations()->indexOfId(id);
        return ops.operations()->data(ops.operations()->index(row), OperationsModel::ProgressRole).toDouble();
    }
    static bool finished(FileOperations &ops, int id)
    {
        const QString s = stateOf(ops, id);
        return s == QLatin1String("done") || s == QLatin1String("failed") || s == QLatin1String("cancelled");
    }

private slots:
    void initTestCase() { GlibLoop::checkDispatcher(); }
    void copyNestedTree();
    void moveFiles();
    void trashAndRestore();
    void conflictKeepBoth();
    void conflictReplaceAndSkipAll();
    void undoRename();
    void undoMoveAndCreate();
    void deleteTree();
    void cancelAndPause();
    void archives();
};

void TstFileOps::copyNestedTree()
{
    QTemporaryDir tmp;
    const QString src = tmp.filePath(QStringLiteral("src"));
    QByteArray big(3 * 1024 * 1024, 'q');
    writeFile(src + QStringLiteral("/a.txt"), "alpha");
    writeFile(src + QStringLiteral("/sub/b.bin"), big);
    writeFile(src + QStringLiteral("/sub/deep/c.txt"), "gamma");
    QDir().mkpath(src + QStringLiteral("/empty"));
    QDir().mkpath(tmp.filePath(QStringLiteral("dst")));

    FileOperations ops;
    QSignalSpy fin(&ops, &FileOperations::finished);
    const int id = ops.copy({src}, tmp.filePath(QStringLiteral("dst")));
    QVERIFY(id > 0);
    QVERIFY(fin.wait(10000) || finished(ops, id));
    QTRY_VERIFY(finished(ops, id));
    QCOMPARE(stateOf(ops, id), QStringLiteral("done"));
    QCOMPARE(progressOf(ops, id), 1.0);
    const QString out = tmp.filePath(QStringLiteral("dst/src"));
    QCOMPARE(readFile(out + QStringLiteral("/a.txt")), QByteArray("alpha"));
    QCOMPARE(readFile(out + QStringLiteral("/sub/b.bin")), big);
    QCOMPARE(readFile(out + QStringLiteral("/sub/deep/c.txt")), QByteArray("gamma"));
    QVERIFY(QFileInfo(out + QStringLiteral("/empty")).isDir());
    const QModelIndex row = ops.operations()->index(ops.operations()->indexOfId(id));
    QCOMPARE(ops.operations()->data(row, OperationsModel::TitleRole).toString(), QStringLiteral("Copied “src” to dst"));
    QCOMPARE(ops.operations()->data(row, OperationsModel::BytesTotalRole).toLongLong(), qint64(big.size() + 5 + 5));
}

void TstFileOps::moveFiles()
{
    QTemporaryDir tmp;
    writeFile(tmp.filePath(QStringLiteral("one.txt")), "1");
    writeFile(tmp.filePath(QStringLiteral("dir/two.txt")), "2");
    QDir().mkpath(tmp.filePath(QStringLiteral("target")));
    FileOperations ops;
    const int id = ops.move({tmp.filePath(QStringLiteral("one.txt")), tmp.filePath(QStringLiteral("dir"))}, tmp.filePath(QStringLiteral("target")));
    QTRY_VERIFY(finished(ops, id));
    QCOMPARE(stateOf(ops, id), QStringLiteral("done"));
    QVERIFY(!QFile::exists(tmp.filePath(QStringLiteral("one.txt"))));
    QVERIFY(!QFile::exists(tmp.filePath(QStringLiteral("dir"))));
    QCOMPARE(readFile(tmp.filePath(QStringLiteral("target/one.txt"))), QByteArray("1"));
    QCOMPARE(readFile(tmp.filePath(QStringLiteral("target/dir/two.txt"))), QByteArray("2"));
}

void TstFileOps::trashAndRestore()
{
    // The folder lives on the drive of XDG_DATA_HOME, so GLib uses the home trash there.
    const QString dataHome = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
    QDir().mkpath(dataHome);
    QTemporaryDir tmp(dataHome + QStringLiteral("/trash-test-XXXXXX"));
    QVERIFY(tmp.isValid());
    const QString file = tmp.filePath(QStringLiteral("gone.txt"));
    writeFile(file, "bye");
    FileOperations ops;
    const int id = ops.trash({file});
    QTRY_VERIFY(finished(ops, id));
    if (stateOf(ops, id) != QLatin1String("done"))
        QSKIP(qPrintable(QStringLiteral("Trash is not available: ") + ops.operations()->data(ops.operations()->index(0), OperationsModel::ErrorRole).toString()));
    QVERIFY(!QFile::exists(file));
    QVERIFY(ops.canUndo());
    // The file sits in the trash folder of XDG_DATA_HOME or of the temp drive; look in the first one for the direct restore.
    const QString trashed = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QStringLiteral("/Trash/files/gone.txt");
    if (QFile::exists(trashed)) {
        const int rid = ops.restore({QUrl::fromLocalFile(trashed).toString()});
        QTRY_VERIFY(finished(ops, rid));
        QCOMPARE(stateOf(ops, rid), QStringLiteral("done"));
        QCOMPARE(readFile(file), QByteArray("bye"));
        QVERIFY(!QFile::exists(trashed));
        // Trash again, then undo by original place.
        const int id2 = ops.trash({file});
        QTRY_VERIFY(finished(ops, id2));
        QVERIFY(!QFile::exists(file));
        ops.undo();
        QTRY_VERIFY(QFile::exists(file));
        QCOMPARE(readFile(file), QByteArray("bye"));
    } else {
        ops.undo();
        QTRY_VERIFY_WITH_TIMEOUT(QFile::exists(file), 5000);
    }
}

void TstFileOps::conflictKeepBoth()
{
    QTemporaryDir tmp;
    writeFile(tmp.filePath(QStringLiteral("in/report.txt")), "new");
    writeFile(tmp.filePath(QStringLiteral("out/report.txt")), "old");
    FileOperations ops;
    QSignalSpy conflict(&ops, &FileOperations::conflictRequested);
    const int id = ops.copy({tmp.filePath(QStringLiteral("in/report.txt"))}, tmp.filePath(QStringLiteral("out")));
    QTRY_COMPARE(conflict.count(), 1);
    QCOMPARE(stateOf(ops, id), QStringLiteral("conflict"));
    QCOMPARE(conflict.first().at(1).toString(), QStringLiteral("report.txt"));
    ops.resolveConflict(id, FileOperations::KeepBoth);
    QTRY_VERIFY(finished(ops, id));
    QCOMPARE(readFile(tmp.filePath(QStringLiteral("out/report.txt"))), QByteArray("old"));
    QCOMPARE(readFile(tmp.filePath(QStringLiteral("out/report (2).txt"))), QByteArray("new"));
    // A second copy asks again and gets the next number.
    const int id2 = ops.copy({tmp.filePath(QStringLiteral("in/report.txt"))}, tmp.filePath(QStringLiteral("out")));
    QTRY_COMPARE(conflict.count(), 2);
    ops.resolveConflict(id2, FileOperations::KeepBoth);
    QTRY_VERIFY(finished(ops, id2));
    QCOMPARE(readFile(tmp.filePath(QStringLiteral("out/report (3).txt"))), QByteArray("new"));
    // Copy into the same folder makes a numbered copy without asking.
    const int id3 = ops.copy({tmp.filePath(QStringLiteral("in/report.txt"))}, tmp.filePath(QStringLiteral("in")));
    QTRY_VERIFY(finished(ops, id3));
    QCOMPARE(readFile(tmp.filePath(QStringLiteral("in/report (2).txt"))), QByteArray("new"));
}

void TstFileOps::conflictReplaceAndSkipAll()
{
    QTemporaryDir tmp;
    for (const char *n : {"a", "b", "c"}) {
        writeFile(tmp.filePath(QStringLiteral("in/%1.txt").arg(QLatin1String(n))), "new");
        writeFile(tmp.filePath(QStringLiteral("out/%1.txt").arg(QLatin1String(n))), "old");
    }
    FileOperations ops;
    QSignalSpy conflict(&ops, &FileOperations::conflictRequested);
    QStringList srcs;
    for (const char *n : {"a", "b", "c"})
        srcs << tmp.filePath(QStringLiteral("in/%1.txt").arg(QLatin1String(n)));
    int id = ops.copy(srcs, tmp.filePath(QStringLiteral("out")));
    QTRY_COMPARE(conflict.count(), 1);
    ops.resolveConflict(id, FileOperations::Replace, true);
    QTRY_VERIFY(finished(ops, id));
    QCOMPARE(conflict.count(), 1); // apply to all: asked once
    for (const char *n : {"a", "b", "c"})
        QCOMPARE(readFile(tmp.filePath(QStringLiteral("out/%1.txt").arg(QLatin1String(n)))), QByteArray("new"));
    writeFile(tmp.filePath(QStringLiteral("out/a.txt")), "old");
    id = ops.copy(srcs, tmp.filePath(QStringLiteral("out")));
    QTRY_COMPARE(conflict.count(), 2);
    ops.resolveConflict(id, FileOperations::Skip, true);
    QTRY_VERIFY(finished(ops, id));
    QCOMPARE(readFile(tmp.filePath(QStringLiteral("out/a.txt"))), QByteArray("old"));
}

void TstFileOps::undoRename()
{
    QTemporaryDir tmp;
    const QString a = tmp.filePath(QStringLiteral("a.txt"));
    writeFile(a, "x");
    FileOperations ops;
    QSignalSpy renamed(&ops, &FileOperations::renamed);
    ops.rename(QUrl::fromLocalFile(a).toString(), QStringLiteral("b.txt"));
    QTRY_COMPARE(renamed.count(), 1);
    QVERIFY(QFile::exists(tmp.filePath(QStringLiteral("b.txt"))));
    QVERIFY(ops.canUndo());
    QCOMPARE(ops.undoText(), QStringLiteral("Undo rename"));
    ops.undo();
    QTRY_VERIFY(QFile::exists(a));
    QVERIFY(!QFile::exists(tmp.filePath(QStringLiteral("b.txt"))));
    QVERIFY(!ops.canUndo());
}

void TstFileOps::undoMoveAndCreate()
{
    QTemporaryDir tmp;
    writeFile(tmp.filePath(QStringLiteral("m.txt")), "m");
    QDir().mkpath(tmp.filePath(QStringLiteral("to")));
    FileOperations ops;
    const int id = ops.move({tmp.filePath(QStringLiteral("m.txt"))}, tmp.filePath(QStringLiteral("to")));
    QTRY_VERIFY(finished(ops, id));
    QVERIFY(ops.canUndo());
    ops.undo();
    QTRY_VERIFY(QFile::exists(tmp.filePath(QStringLiteral("m.txt"))));
    QVERIFY(!QFile::exists(tmp.filePath(QStringLiteral("to/m.txt"))));

    QSignalSpy created(&ops, &FileOperations::created);
    ops.createFolder(tmp.path(), QStringLiteral("New Folder"));
    QTRY_COMPARE(created.count(), 1);
    ops.createFolder(tmp.path(), QStringLiteral("New Folder"));
    QTRY_COMPARE(created.count(), 2);
    QVERIFY(QFileInfo(tmp.filePath(QStringLiteral("New Folder"))).isDir());
    QVERIFY(QFileInfo(tmp.filePath(QStringLiteral("New Folder (2)"))).isDir());
    ops.createFile(tmp.path(), QStringLiteral("note.txt"));
    QTRY_COMPARE(created.count(), 3);
    QVERIFY(QFile::exists(tmp.filePath(QStringLiteral("note.txt"))));
}

void TstFileOps::deleteTree()
{
    QTemporaryDir tmp;
    writeFile(tmp.filePath(QStringLiteral("d/x/y.txt")), "y");
    writeFile(tmp.filePath(QStringLiteral("d/z.txt")), "z");
    FileOperations ops;
    const int id = ops.remove({tmp.filePath(QStringLiteral("d"))});
    QTRY_VERIFY(finished(ops, id));
    QCOMPARE(stateOf(ops, id), QStringLiteral("done"));
    QVERIFY(!QFile::exists(tmp.filePath(QStringLiteral("d"))));
}

void TstFileOps::cancelAndPause()
{
    QTemporaryDir tmp;
    for (int i = 0; i < 40; ++i)
        writeFile(tmp.filePath(QStringLiteral("src/f%1.bin").arg(i)), QByteArray(512 * 1024, 'z'));
    QDir().mkpath(tmp.filePath(QStringLiteral("dst")));
    FileOperations ops;
    int id = ops.copy({tmp.filePath(QStringLiteral("src"))}, tmp.filePath(QStringLiteral("dst")));
    ops.pause(id);
    QCOMPARE(stateOf(ops, id), QStringLiteral("paused"));
    QTest::qWait(300);
    // Paused work stops between files, so the count stays put.
    const int doneA = ops.operations()->data(ops.operations()->index(ops.operations()->indexOfId(id)), OperationsModel::ItemsDoneRole).toInt();
    QTest::qWait(300);
    const int doneB = ops.operations()->data(ops.operations()->index(ops.operations()->indexOfId(id)), OperationsModel::ItemsDoneRole).toInt();
    QCOMPARE(doneA, doneB);
    ops.resume(id);
    QTRY_VERIFY(finished(ops, id));
    QCOMPARE(stateOf(ops, id), QStringLiteral("done"));

    id = ops.copy({tmp.filePath(QStringLiteral("src"))}, tmp.filePath(QStringLiteral("dst2")));
    ops.cancel(id);
    QCOMPARE(stateOf(ops, id), QStringLiteral("cancelled"));
}

void TstFileOps::archives()
{
    if (!FileOperations::archiveSupported())
        QSKIP("bsdtar is not installed");
    QTemporaryDir tmp;
    writeFile(tmp.filePath(QStringLiteral("p/one.txt")), "1");
    writeFile(tmp.filePath(QStringLiteral("p/two.txt")), "2");
    FileOperations ops;
    int id = ops.compress({tmp.filePath(QStringLiteral("p"))}, tmp.path(), QStringLiteral("pack.tar.gz"));
    QTRY_VERIFY(finished(ops, id));
    QCOMPARE(stateOf(ops, id), QStringLiteral("done"));
    QVERIFY(QFile::exists(tmp.filePath(QStringLiteral("pack.tar.gz"))));
    QDir().mkpath(tmp.filePath(QStringLiteral("x")));
    id = ops.extract({tmp.filePath(QStringLiteral("pack.tar.gz"))}, tmp.filePath(QStringLiteral("x")));
    QTRY_VERIFY(finished(ops, id));
    QCOMPARE(stateOf(ops, id), QStringLiteral("done"));
    QCOMPARE(readFile(tmp.filePath(QStringLiteral("x/pack/p/two.txt"))), QByteArray("2"));
}

QTEST_GUILESS_MAIN(TstFileOps)
#include "tst_fileops.moc"
