#include "medialibrary.h"
#include "player.h"
#include "radio.h"

#include <QDir>
#include <QFile>
#include <QImage>
#include <QProcess>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTest>

class TestMedia : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir m_dir;
    bool m_haveFfmpeg = false;

    // A short silent MP3 with tags, made by ffmpeg.
    bool makeSong(const QString &path, const QString &title, const QString &artist, const QString &album, int track)
    {
        QDir().mkpath(QFileInfo(path).absolutePath());
        QProcess p;
        p.start(QStringLiteral("ffmpeg"), {QStringLiteral("-v"), QStringLiteral("quiet"), QStringLiteral("-f"), QStringLiteral("lavfi"), QStringLiteral("-i"),
                                           QStringLiteral("anullsrc=r=22050:cl=mono"), QStringLiteral("-t"), QStringLiteral("2"), QStringLiteral("-metadata"),
                                           QStringLiteral("title=") + title, QStringLiteral("-metadata"), QStringLiteral("artist=") + artist,
                                           QStringLiteral("-metadata"), QStringLiteral("album=") + album, QStringLiteral("-metadata"),
                                           QStringLiteral("track=%1").arg(track), QStringLiteral("-y"), path});
        return p.waitForFinished(20000) && p.exitCode() == 0;
    }

private slots:
    void initTestCase()
    {
        QVERIFY(m_dir.isValid());
        m_haveFfmpeg = !QStandardPaths::findExecutable(QStringLiteral("ffmpeg")).isEmpty();
        if (!m_haveFfmpeg)
            QSKIP("ffmpeg is needed to make test songs");
        const QString music = m_dir.path() + QStringLiteral("/Music");
        QVERIFY(makeSong(music + QStringLiteral("/Album A/02 Second.mp3"), QStringLiteral("Second"), QStringLiteral("Band"), QStringLiteral("Album A"), 2));
        QVERIFY(makeSong(music + QStringLiteral("/Album A/01 First.mp3"), QStringLiteral("First"), QStringLiteral("Band"), QStringLiteral("Album A"), 1));
        // Untagged files take their album from the folder and a title from the name.
        QVERIFY(makeSong(music + QStringLiteral("/Soundtrack/03 - Field of Hopes [ChyKnVw5klA].mp3"), QString(), QString(), QString(), 0));
        QVERIFY(makeSong(music + QStringLiteral("/DELTARUNE Chapter 1 Soundtrack (Official)/1. ANOTHER HIM (DELTARUNE Chapter 1 Soundtrack) - Toby Fox.mp3"), QString(), QString(), QString(), 0));
        QVERIFY(makeSong(music + QStringLiteral("/Loose Song.mp3"), QString(), QStringLiteral("Someone"), QString(), 0));
        // A game folder: its sounds are not music.
        QVERIFY(makeSong(music + QStringLiteral("/Game/sfx/hit.mp3"), QStringLiteral("hit"), QString(), QString(), 0));
        QFile exe(music + QStringLiteral("/Game/game.x86_64"));
        QVERIFY(exe.open(QIODevice::WriteOnly));
        exe.close();
        QVERIFY(makeSong(m_dir.path() + QStringLiteral("/Books/Long Story/Chapter 1.mp3"), QStringLiteral("One"), QStringLiteral("Writer"), QStringLiteral("Long Story"), 1));
        QVERIFY(makeSong(m_dir.path() + QStringLiteral("/Books/Long Story/Chapter 2.mp3"), QStringLiteral("Two"), QStringLiteral("Writer"), QStringLiteral("Long Story"), 2));
        qputenv("OPAL_MEDIA_MUSIC", music.toUtf8());
        qputenv("OPAL_MEDIA_BOOKS", (m_dir.path() + QStringLiteral("/Books")).toUtf8());
        qputenv("OPAL_MEDIA_VIDEOS", (m_dir.path() + QStringLiteral("/Videos")).toUtf8());
    }

    void names()
    {
        QVERIFY(MediaLibrary::isAudioName(QStringLiteral("a.FLAC")));
        QVERIFY(MediaLibrary::isVideoName(QStringLiteral("clip.mkv")));
        QVERIFY(!MediaLibrary::isAudioName(QStringLiteral("cover.jpg")));
        QCOMPARE(MediaLibrary::formatDuration(65000), QStringLiteral("1:05"));
        QCOMPARE(MediaLibrary::formatDuration(3723000), QStringLiteral("1:02:03"));
    }

    void lrc()
    {
        const QVariantList l = Player::parseLrc(QStringLiteral("[ar:Someone]\n[00:12.50]Hello\n[00:05.00][00:30.00]Again\n"));
        QCOMPARE(l.size(), 3);
        QCOMPARE(l.at(0).toMap().value(QStringLiteral("time")).toLongLong(), 5000);
        QCOMPARE(l.at(1).toMap().value(QStringLiteral("text")).toString(), QStringLiteral("Hello"));
        QCOMPARE(l.at(2).toMap().value(QStringLiteral("time")).toLongLong(), 30000);
    }

    void coverColour()
    {
        QImage img(40, 40, QImage::Format_RGB32);
        img.fill(QColor(220, 40, 60));
        const QColor c = Player::coverColour(img);
        QVERIFY(c.red() > 180 && c.green() < 80);
    }

    void radioStation()
    {
        const QVariantMap s = Radio::fromDirectory({{QStringLiteral("name"), QStringLiteral(" Lofi ")}, {QStringLiteral("url"), QStringLiteral("http://a/b")},
                                                    {QStringLiteral("url_resolved"), QStringLiteral("http://a/c.mp3")}, {QStringLiteral("tags"), QStringLiteral("lofi, chill,beats,study")}});
        QCOMPARE(s.value(QStringLiteral("name")).toString(), QStringLiteral("Lofi"));
        QCOMPARE(s.value(QStringLiteral("url")).toString(), QStringLiteral("http://a/c.mp3"));
        QCOMPARE(s.value(QStringLiteral("artist")).toString(), QStringLiteral("lofi, chill, beats"));
    }

    void library()
    {
        auto *lib = MediaLibrary::instance();
        QVERIFY(lib->waitForScan(30000));
        QCOMPARE(lib->songCount(), 5);
        QCOMPARE(lib->albumCount(), 4);
        QCOMPARE(lib->bookCount(), 1);
        QVERIFY(!lib->skipped().isEmpty());
        // Tracks sort by number inside the album.
        QVariantMap a;
        for (const QVariant &v : lib->albums())
            if (v.toMap().value(QStringLiteral("title")) == QLatin1String("Album A"))
                a = lib->album(v.toMap().value(QStringLiteral("key")).toString());
        QCOMPARE(a.value(QStringLiteral("tracks")).toList().first().toMap().value(QStringLiteral("title")).toString(), QStringLiteral("First"));
        // The untagged song: folder album, cleaned title.
        const QVariantList found = lib->songs(QStringLiteral("hopes"));
        QCOMPARE(found.size(), 1);
        QCOMPARE(found.first().toMap().value(QStringLiteral("title")).toString(), QStringLiteral("Field of Hopes"));
        QCOMPARE(found.first().toMap().value(QStringLiteral("album")).toString(), QStringLiteral("Soundtrack"));
        // A download's title loses the artist suffix and the repeated album, and the artist comes from it.
        const QVariantList him = lib->songs(QStringLiteral("another him"));
        QCOMPARE(him.size(), 1);
        QCOMPARE(him.first().toMap().value(QStringLiteral("title")).toString(), QStringLiteral("ANOTHER HIM"));
        QCOMPARE(him.first().toMap().value(QStringLiteral("artist")).toString(), QStringLiteral("Toby Fox"));
        // A loose file at the top of the music folder is a single.
        const QVariantList loose = lib->songs(QStringLiteral("loose"));
        QCOMPARE(loose.first().toMap().value(QStringLiteral("album")).toString(), QStringLiteral("Loose Song"));
    }

    void playlistsAndLikes()
    {
        auto *lib = MediaLibrary::instance();
        const QVariantList songs = lib->songs();
        const QString first = songs.first().toMap().value(QStringLiteral("path")).toString();
        lib->toggleLiked(first);
        QVERIFY(lib->liked(first));
        QCOMPARE(lib->likedSongs().size(), 1);
        const QString list = lib->createPlaylist(QStringLiteral("Road trip"), {first});
        QVERIFY(list.endsWith(QStringLiteral("Road trip.m3u8")));
        QVERIFY(lib->addToPlaylist(list, {songs.at(1).toMap().value(QStringLiteral("path")).toString()}));
        QCOMPARE(lib->playlist(list).size(), 2);
        QVERIFY(lib->removeFromPlaylist(list, 0));
        QCOMPARE(lib->playlist(list).size(), 1);
    }

    void bookProgress()
    {
        auto *lib = MediaLibrary::instance();
        const QString key = lib->books().first().toMap().value(QStringLiteral("key")).toString();
        lib->setBookProgress(key, 1, 500);
        const QVariantMap b = lib->book(key);
        QCOMPARE(b.value(QStringLiteral("chapter")).toInt(), 1);
        QVERIFY(b.value(QStringLiteral("progress")).toDouble() > .4);
        QVERIFY(b.value(QStringLiteral("started")).toBool());
    }

    void queue()
    {
        auto *lib = MediaLibrary::instance();
        Player p;
        p.playTracks(lib->songs().mid(0, 3), 1);
        QCOMPARE(p.index(), 1);
        QCOMPARE(p.queue().size(), 3);
        p.playNext({lib->songs().at(0)});
        QCOMPARE(p.queue().size(), 4);
        QCOMPARE(p.queue().at(2).toMap().value(QStringLiteral("path")), lib->songs().at(0).toMap().value(QStringLiteral("path")));
        p.move(3, 0);
        QCOMPARE(p.index(), 2);
        p.removeAt(0);
        QCOMPARE(p.index(), 1);
        p.setRepeat(QStringLiteral("bogus"));
        QCOMPARE(p.repeat(), QStringLiteral("off"));
        p.clearQueue();
        QVERIFY(!p.hasMedia());
    }
};

QTEST_MAIN(TestMedia)
#include "tst_media.moc"
