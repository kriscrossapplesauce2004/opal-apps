#include "albummodel.h"
#include "editor.h"
#include "exif.h"
#include "imageio.h"
#include "library.h"
#include "photoactions.h"
#include "photoview.h"
#include "scanner.h"

#include <QDir>
#include <QFile>
#include <QImage>
#include <QPainter>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

class TestPhotos : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir m_dir;

    static void writeImage(const QString &path, int w, int h, QColor colour)
    {
        QImage img(w, h, QImage::Format_RGB32);
        img.fill(colour);
        QDir().mkpath(QFileInfo(path).absolutePath());
        QVERIFY(img.save(path));
    }

private slots:
    void initTestCase()
    {
        QVERIFY(m_dir.isValid());
        const QString root = m_dir.path() + QStringLiteral("/Pictures");
        writeImage(root + QStringLiteral("/Trips/IMG_20260704_101500.jpg"), 400, 300, Qt::red);
        writeImage(root + QStringLiteral("/Trips/IMG_20260704_121500.jpg"), 300, 400, Qt::green);
        writeImage(root + QStringLiteral("/Trips/IMG_20250101_090000.jpg"), 640, 360, Qt::blue);
        writeImage(root + QStringLiteral("/Screenshots/Screenshot_2026-10-08_18-29-41.png"), 1920, 1080, Qt::gray);
        // A game folder: an executable marker keeps its textures out of the timeline.
        writeImage(root + QStringLiteral("/SomeGame/textures/grass.png"), 64, 64, Qt::darkGreen);
        // The root itself holds a program folder, like a Downloads-style Pictures folder; it must still be walked.
        QDir().mkpath(root + QStringLiteral("/tool.x86_64"));
        QFile exe(root + QStringLiteral("/SomeGame/game.exe"));
        QVERIFY(exe.open(QIODevice::WriteOnly));
        exe.write("MZ");
        exe.close();
        qputenv("OPAL_PHOTOS_ROOTS", root.toUtf8());
    }

    void datesFromNames()
    {
        QCOMPARE(Exif::dateFromName(QStringLiteral("IMG_20261008_182941.jpg")), QDateTime(QDate(2026, 10, 8), QTime(18, 29, 41)));
        QCOMPARE(Exif::dateFromName(QStringLiteral("Screenshot_2026-10-08_18-29-41.png")), QDateTime(QDate(2026, 10, 8), QTime(18, 29, 41)));
        QCOMPARE(Exif::dateFromName(QStringLiteral("shot-20261008-182941.png")), QDateTime(QDate(2026, 10, 8), QTime(18, 29, 41)));
        QVERIFY(!Exif::dateFromName(QStringLiteral("holiday.jpg")).isValid());
    }

    void fileKinds()
    {
        QVERIFY(Scanner::isImageName(QStringLiteral("a.JPG")));
        QVERIFY(Scanner::isVideoName(QStringLiteral("clip.mp4")));
        QVERIFY(!Scanner::isImageName(QStringLiteral("notes.txt")));
        QVERIFY(Scanner::isScreenshot(QStringLiteral("/home/a/Pictures/Screenshots/x.png")));
        QVERIFY(Scanner::isScreenshot(QStringLiteral("/home/a/Pictures/Screenshot_2026-10-08.png")));
        QVERIFY(!Scanner::isScreenshot(QStringLiteral("/home/a/Pictures/Trips/beach.jpg")));
    }

    void orientation()
    {
        QCOMPARE(ImageIO::composeOrientation(1, 1), 6);
        QCOMPARE(ImageIO::composeOrientation(6, 1), 3);
        QCOMPARE(ImageIO::composeOrientation(1, 4), 1);
        const QString p = m_dir.path() + QStringLiteral("/copyname/a.jpg");
        writeImage(p, 10, 10, Qt::white);
        QCOMPARE(QFileInfo(ImageIO::copyName(p)).fileName(), QStringLiteral("a-edited.jpg"));
        writeImage(m_dir.path() + QStringLiteral("/copyname/a-edited.jpg"), 10, 10, Qt::white);
        QCOMPARE(QFileInfo(ImageIO::copyName(p)).fileName(), QStringLiteral("a-edited-2.jpg"));
    }

    void editorPipeline()
    {
        QCOMPARE(PhotoEditor::coverScale(100, 100, 0), 1.0);
        QVERIFY(PhotoEditor::coverScale(400, 300, 10) > 1.0);
        QImage img(200, 100, QImage::Format_ARGB32_Premultiplied);
        img.fill(QColor(120, 120, 120));
        // No adjustments leave the pixels as they are.
        QImage same = img;
        PhotoEditor::adjustPixels(same, PhotoEditor::Params{});
        QCOMPARE(same.pixelColor(50, 50), img.pixelColor(50, 50));
        PhotoEditor::Params bright;
        bright.brightness = .5;
        QImage lit = img;
        PhotoEditor::adjustPixels(lit, bright);
        QVERIFY(lit.pixelColor(100, 50).lightness() > img.pixelColor(100, 50).lightness());
        // A quarter turn swaps the sides, and the crop is in frame units.
        const QImage turned = PhotoEditor::render(img, {}, 1, false, 0, QRectF(0, 0, 1, 1), {});
        QCOMPARE(turned.size(), QSize(100, 200));
        const QImage cropped = PhotoEditor::render(img, {}, 0, false, 0, QRectF(.25, 0, .5, .5), {});
        QCOMPARE(cropped.size(), QSize(100, 50));
    }

    void libraryScansAndSkipsSoftware()
    {
        auto *lib = PhotoLibrary::instance();
        QVERIFY(lib->waitForScan(20000));
        QCOMPARE(lib->count(), 4);
        QVERIFY(lib->skipped().size() >= 1);
        QCOMPARE(lib->screenshotCount(), 1);
        // Newest first: the screenshot from October, then the July pair.
        QVERIFY(lib->photos().first().path.endsWith(QStringLiteral("Screenshot_2026-10-08_18-29-41.png")));
        const QVariantMap p = lib->photo(lib->photos().at(1).path);
        QCOMPARE(p.value(QStringLiteral("folderName")).toString(), QStringLiteral("Trips"));
    }

    void viewGroupsAndFilters()
    {
        PhotoView view;
        view.setWidth(1000);
        view.setGroup(QStringLiteral("day"));
        QSignalSpy laid(&view, &PhotoView::laidOut);
        QTRY_VERIFY(view.count() == 4);
        // Three days: a header row plus a photo row each.
        QCOMPARE(view.rowCount(), 6);
        QCOMPARE(view.data(view.index(0), PhotoView::KindRole).toString(), QStringLiteral("header"));
        view.setFilter(QStringLiteral("screenshots"));
        QTRY_COMPARE(view.count(), 1);
        view.setFilter(QStringLiteral("all"));
        view.setQuery(QStringLiteral("trips"));
        QTRY_COMPARE(view.count(), 3);
        view.setQuery(QString());
        QTRY_COMPARE(view.count(), 4);
        view.selectAll();
        QCOMPARE(view.selectedCount(), 4);
        view.clearSelection();
        QCOMPARE(view.selectedCount(), 0);
        // At the edge the focus stays where it is.
        QCOMPARE(view.neighbour(0, QStringLiteral("left")), 0);
        QCOMPARE(view.neighbour(0, QStringLiteral("right")), 1);
    }

    void albums()
    {
        AlbumModel model;
        QTRY_VERIFY(model.count() >= 2);
        QStringList names;
        for (int i = 0; i < model.count(); ++i)
            names << model.get(i).value(QStringLiteral("name")).toString();
        QVERIFY(names.contains(QStringLiteral("Trips")));
        QVERIFY(names.contains(QStringLiteral("Screenshots")));
        QVERIFY(!names.contains(QStringLiteral("textures")));
    }

    void favourites()
    {
        auto *lib = PhotoLibrary::instance();
        const QString p = lib->photos().at(1).path;
        QVERIFY(!lib->isFavourite(p));
        lib->toggleFavourite(p);
        QVERIFY(lib->isFavourite(p));
        PhotoView view;
        view.setWidth(800);
        view.setFilter(QStringLiteral("favourites"));
        QTRY_COMPARE(view.count(), 1);
        lib->toggleFavourite(p);
        QTRY_COMPARE(view.count(), 0);
    }

    void importCopiesOnce()
    {
        QVERIFY(PhotoActions::importFolder(QStringLiteral("/p")).startsWith(QStringLiteral("/p/Imported/")));
        PhotoActions actions;
        QSignalSpy imported(&actions, &PhotoActions::imported);
        const QString src = m_dir.path() + QStringLiteral("/phone/DCIM/IMG_1.jpg");
        writeImage(src, 20, 20, Qt::yellow);
        actions.importPhotos({src});
        QTRY_COMPARE(imported.count(), 1);
        QCOMPARE(imported.first().at(1).toInt(), 1);
        actions.importPhotos({src});
        QTRY_COMPARE(imported.count(), 2);
        QCOMPARE(imported.at(1).at(1).toInt(), 0);
    }

    void saveCopyKeepsOriginal()
    {
        const QString p = m_dir.path() + QStringLiteral("/edit/photo.png");
        writeImage(p, 120, 80, QColor(90, 90, 90));
        PhotoEditor editor;
        editor.setPath(p);
        editor.setValue(QStringLiteral("brightness"), .4);
        editor.setQuarterTurns(1);
        QVERIFY(editor.dirty());
        QSignalSpy saved(&editor, &PhotoEditor::saved);
        editor.save(true);
        QTRY_COMPARE_WITH_TIMEOUT(saved.count(), 1, 10000);
        const QString copy = saved.first().at(0).toString();
        QVERIFY(copy.endsWith(QStringLiteral("photo-edited.png")));
        QCOMPARE(QImage(copy).size(), QSize(80, 120));
        QCOMPARE(QImage(p).size(), QSize(120, 80));
    }
};

QTEST_MAIN(TestPhotos)
#include "tst_photos.moc"
