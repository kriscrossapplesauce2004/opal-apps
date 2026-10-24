#pragma once
#include <QAudioOutput>
#include <QColor>
#include <QMediaPlayer>
#include <QObject>
#include <QTimer>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

class QQmlEngine;
class QJSEngine;

// Audio playback for songs, audiobooks and radio, with the queue, lyrics, outputs and a sleep timer. Videos play in the video page.
// QML: singleton "Player".
// Properties: queue, index, current (track map; kind is music, book or radio), hasMedia, playing, buffering, position, duration,
//   volume (0..1), muted, shuffle, repeat ("off", "all", "one"), rate, sleepRemaining (ms, 0 = off, -1 = at the end of this track),
//   lyrics ([{time, text}], time -1 when unsynced), lyricIndex, outputs ([{id, name}]), output, artColor, error, bookKey.
// Methods: playTracks(tracks, start), playBook(key, chapter = -1), playRadio(station), enqueue(tracks), playNext(tracks),
//   removeAt(i), move(from, to), jump(i), clearQueue(), toggle(), play(), pause(), stop(), next(), previous(), seek(ms),
//   seekBy(ms), setSleep(minutes) (0 off, -1 end of track), setOutput(id).
// The queue and position come back at the next start, paused.
class Player : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(QVariantList queue READ queue NOTIFY queueChanged)
    Q_PROPERTY(int index READ index NOTIFY currentChanged)
    Q_PROPERTY(QVariantMap current READ current NOTIFY currentChanged)
    Q_PROPERTY(bool hasMedia READ hasMedia NOTIFY currentChanged)
    Q_PROPERTY(bool playing READ playing NOTIFY playingChanged)
    Q_PROPERTY(bool buffering READ buffering NOTIFY playingChanged)
    Q_PROPERTY(qint64 position READ position NOTIFY positionChanged)
    Q_PROPERTY(qint64 duration READ duration NOTIFY durationChanged)
    Q_PROPERTY(qreal volume READ volume WRITE setVolume NOTIFY volumeChanged)
    Q_PROPERTY(bool muted READ muted WRITE setMuted NOTIFY volumeChanged)
    Q_PROPERTY(bool shuffle READ shuffle WRITE setShuffle NOTIFY modeChanged)
    Q_PROPERTY(QString repeat READ repeat WRITE setRepeat NOTIFY modeChanged)
    Q_PROPERTY(qreal rate READ rate WRITE setRate NOTIFY modeChanged)
    Q_PROPERTY(qint64 sleepRemaining READ sleepRemaining NOTIFY sleepChanged)
    Q_PROPERTY(QVariantList lyrics READ lyrics NOTIFY lyricsChanged)
    Q_PROPERTY(int lyricIndex READ lyricIndex NOTIFY lyricIndexChanged)
    Q_PROPERTY(QVariantList outputs READ outputs NOTIFY outputsChanged)
    Q_PROPERTY(QString output READ output NOTIFY outputsChanged)
    Q_PROPERTY(QColor artColor READ artColor NOTIFY artColorChanged)
    Q_PROPERTY(QString error READ error NOTIFY errorChanged)
    Q_PROPERTY(QString bookKey READ bookKey NOTIFY currentChanged)

public:
    explicit Player(QObject *parent = nullptr);
    ~Player() override;
    static Player *instance();
    static Player *create(QQmlEngine *, QJSEngine *);

    QVariantList queue() const { return m_queue; }
    int index() const { return m_index; }
    QVariantMap current() const { return m_index >= 0 && m_index < m_queue.size() ? m_queue.at(m_index).toMap() : QVariantMap(); }
    bool hasMedia() const { return m_index >= 0 && m_index < m_queue.size(); }
    bool playing() const { return m_player.playbackState() == QMediaPlayer::PlayingState; }
    bool buffering() const;
    qint64 position() const { return m_player.position(); }
    qint64 duration() const;
    qreal volume() const { return m_out.volume(); }
    void setVolume(qreal v);
    bool muted() const { return m_out.isMuted(); }
    void setMuted(bool m);
    bool shuffle() const { return m_shuffle; }
    void setShuffle(bool on);
    QString repeat() const { return m_repeat; }
    void setRepeat(const QString &r);
    qreal rate() const { return m_player.playbackRate(); }
    void setRate(qreal r);
    qint64 sleepRemaining() const;
    QVariantList lyrics() const { return m_lyrics; }
    int lyricIndex() const { return m_lyricIndex; }
    QVariantList outputs() const;
    QString output() const;
    QColor artColor() const { return m_artColor; }
    QString error() const { return m_error; }
    QString bookKey() const { return m_bookKey; }

    Q_INVOKABLE void playTracks(const QVariantList &tracks, int start = 0);
    Q_INVOKABLE void playBook(const QString &key, int chapter = -1);
    Q_INVOKABLE void playRadio(const QVariantMap &station);
    Q_INVOKABLE void enqueue(const QVariantList &tracks);
    Q_INVOKABLE void playNext(const QVariantList &tracks);
    Q_INVOKABLE void removeAt(int i);
    Q_INVOKABLE void move(int from, int to);
    Q_INVOKABLE void jump(int i);
    Q_INVOKABLE void clearQueue();
    Q_INVOKABLE void toggle();
    Q_INVOKABLE void play();
    Q_INVOKABLE void pause();
    Q_INVOKABLE void stop();
    Q_INVOKABLE void next();
    Q_INVOKABLE void previous();
    Q_INVOKABLE void seek(qint64 ms);
    Q_INVOKABLE void seekBy(qint64 ms);
    Q_INVOKABLE void setSleep(int minutes);
    Q_INVOKABLE void setOutput(const QString &id);

    // Parses an LRC file. Public for tests.
    static QVariantList parseLrc(const QString &text);
    // The colour that best stands for a cover: the most common lively colour, else the average.
    static QColor coverColour(const QImage &image);

signals:
    void queueChanged();
    void currentChanged();
    void playingChanged();
    void positionChanged();
    void durationChanged();
    void volumeChanged();
    void modeChanged();
    void sleepChanged();
    void lyricsChanged();
    void lyricIndexChanged();
    void outputsChanged();
    void artColorChanged();
    void errorChanged();
    void raiseRequested();

private:
    void load(bool autoplay, qint64 startAt = 0);
    void finished();
    int nextIndex(int from, bool wrap) const;
    void reshuffle();
    void loadExtras();
    void saveState();
    void restoreState();
    void updateLyricIndex();
    void setError(const QString &e);

    QMediaPlayer m_player;
    QAudioOutput m_out;
    QVariantList m_queue;
    QList<int> m_order;
    int m_index = -1;
    bool m_shuffle = false;
    QString m_repeat = QStringLiteral("off");
    QVariantList m_lyrics;
    int m_lyricIndex = -1;
    QColor m_artColor;
    QString m_error;
    QString m_bookKey;
    QTimer m_saveTimer;
    QTimer m_sleep;
    bool m_sleepAtEnd = false;
    qint64 m_pendingSeek = -1;
    int m_extrasGeneration = 0;
};
