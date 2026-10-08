#include "glibloop.h"
#include "search.h"

#include <QDir>
#include <QFile>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>

static void touch(const QString &path, const QByteArray &data = {})
{
    QDir().mkpath(QFileInfo(path).absolutePath());
    QFile f(path);
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write(data);
}

static QStringList names(Search &s)
{
    QStringList out;
    for (int i = 0; i < s.rowCount(); ++i)
        out << s.data(s.index(i), Search::NameRole).toString();
    out.sort();
    return out;
}

class TstSearch : public QObject
{
    Q_OBJECT
private slots:
    void initTestCase() { GlibLoop::checkDispatcher(); }
    void nestedSubstringAndGlob();
    void hiddenAndCap();
    void cancelStops();
    void contentSearch();
};

void TstSearch::nestedSubstringAndGlob()
{
    QTemporaryDir dir;
    touch(dir.filePath(QStringLiteral("a/b/c/Budget-2026.txt")));
    touch(dir.filePath(QStringLiteral("a/budget_old.md")));
    touch(dir.filePath(QStringLiteral("x/notes.txt")));
    touch(dir.filePath(QStringLiteral(".hidden/budget.txt")));
    Search s;
    s.setRoot(QUrl::fromLocalFile(dir.path()));
    s.setQuery(QStringLiteral("budget"));
    QSignalSpy fin(&s, &Search::finished);
    s.start();
    QVERIFY(s.running());
    QVERIFY(fin.wait(5000));
    QCOMPARE(names(s), (QStringList{"Budget-2026.txt", "budget_old.md"}));
    QVERIFY(!s.running());

    s.setQuery(QStringLiteral("*.txt"));
    s.start();
    QTRY_VERIFY(!s.running());
    QCOMPARE(names(s), (QStringList{"Budget-2026.txt", "notes.txt"}));

    s.setShowHidden(true);
    s.start();
    QTRY_VERIFY(!s.running());
    QCOMPARE(s.count(), 3);
}

void TstSearch::hiddenAndCap()
{
    QTemporaryDir dir;
    for (int i = 0; i < 50; ++i)
        touch(dir.filePath(QStringLiteral("d%1/hit%2.txt").arg(i % 5).arg(i)));
    Search s;
    s.setRoot(QUrl::fromLocalFile(dir.path()));
    s.setQuery(QStringLiteral("hit"));
    s.setMaxResults(20);
    s.start();
    QTRY_VERIFY(!s.running());
    QCOMPARE(s.count(), 20);
    QVERIFY(s.truncated());
}

void TstSearch::cancelStops()
{
    QTemporaryDir dir;
    for (int i = 0; i < 300; ++i)
        touch(dir.filePath(QStringLiteral("d%1/e%2/file%3.txt").arg(i % 10).arg(i % 7).arg(i)));
    Search s;
    s.setRoot(QUrl::fromLocalFile(dir.path()));
    s.setQuery(QStringLiteral("file"));
    QSignalSpy fin(&s, &Search::finished);
    s.start();
    s.cancel();
    QVERIFY(!s.running());
    QCOMPARE(fin.count(), 1);
    const int before = s.count();
    QTest::qWait(300);
    QCOMPARE(s.count(), before);
    QVERIFY(before < 300);
    // A new start after a cancel works.
    s.start();
    QTRY_VERIFY(!s.running());
    QCOMPARE(s.count(), 300);
}

void TstSearch::contentSearch()
{
    if (!Search::contentSupported())
        QSKIP("rg is not installed");
    QTemporaryDir dir;
    touch(dir.filePath(QStringLiteral("deep/dir/log.txt")), "first line\nthe needle is here\nlast\n");
    touch(dir.filePath(QStringLiteral("other.txt")), "nothing\n");
    Search s;
    s.setRoot(QUrl::fromLocalFile(dir.path()));
    s.setMode(QStringLiteral("content"));
    s.setQuery(QStringLiteral("needle"));
    s.start();
    QTRY_VERIFY_WITH_TIMEOUT(!s.running() && s.count() == 1, 5000);
    QCOMPARE(s.data(s.index(0), Search::NameRole).toString(), QStringLiteral("log.txt"));
    QCOMPARE(s.data(s.index(0), Search::SnippetRole).toString(), QStringLiteral("the needle is here"));
    QCOMPARE(s.data(s.index(0), Search::LineRole).toInt(), 2);
}

QTEST_MAIN(TstSearch)
#include "tst_search.moc"
