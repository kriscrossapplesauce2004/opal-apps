#include "player.h"
#include "medialibrary.h"

#include <QAudioDevice>
#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QImage>
#include <QJSEngine>
#include <QMediaDevices>
#include <QMediaMetaData>
#include <QPointer>
#include <QRandomGenerator>
#include <QRegularExpression>
#include <QSettings>
#include <QStandardPaths>
#include <QUrl>
#include <QtConcurrent>
#include <algorithm>

#include <taglib/fileref.h>
#include <taglib/tpropertymap.h>

namespace {

QSettings *state()
{
    static QSettings *s = new QSettings(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + QStringLiteral("/opal/media.ini"),
                                        QSettings::IniFormat, QCoreApplication::instance());
    return s;
}

QString kindOf(const QVariantMap &t)
{
    return t.value(QStringLiteral("kind")).toString();
}

}

Player::Player(QObject *parent)
    : QObject(parent)
{
    m_player.setAudioOutput(&m_out);
    m_out.setVolume(state()->value(QStringLiteral("player/volume"), .8).toDouble());
    m_shuffle = state()->value(QStringLiteral("player/shuffle"), false).toBool();
    m_repeat = state()->value(QStringLiteral("player/repeat"), QStringLiteral("off")).toString();
    connect(&m_player, &QMediaPlayer::playbackStateChanged, this, [this] {
        emit playingChanged();
        if (playing())
            m_saveTimer.start();
        else
            saveState();
    });
    connect(&m_player, &QMediaPlayer::mediaStatusChanged, this, [this](QMediaPlayer::MediaStatus s) {
        emit playingChanged();
        if (s == QMediaPlayer::LoadedMedia && m_pendingSeek > 0) {
            m_player.setPosition(m_pendingSeek);
            m_pendingSeek = -1;
        }
        if (s == QMediaPlayer::EndOfMedia)
            finished();
        if (s == QMediaPlayer::InvalidMedia)
            setError(tr("This file can't be played"));
    });
    connect(&m_player, &QMediaPlayer::positionChanged, this, [this] {
        emit positionChanged();
        updateLyricIndex();
    });
    connect(&m_player, &QMediaPlayer::durationChanged, this, &Player::durationChanged);
    connect(&m_player, &QMediaPlayer::playbackRateChanged, this, &Player::modeChanged);
    connect(&m_player, &QMediaPlayer::errorOccurred, this, [this](QMediaPlayer::Error, const QString &msg) { setError(msg); });
    // Radio streams name the song that plays now in their metadata.
    connect(&m_player, &QMediaPlayer::metaDataChanged, this, [this] {
        if (!hasMedia() || kindOf(current()) != QLatin1String("radio"))
            return;
        const QString title = m_player.metaData().stringValue(QMediaMetaData::Title).trimmed();
        if (title.isEmpty())
            return;
        QVariantMap t = current();
        if (t.value(QStringLiteral("nowPlaying")).toString() == title)
            return;
        t.insert(QStringLiteral("nowPlaying"), title);
        m_queue[m_index] = t;
        emit currentChanged();
    });
    connect(&m_out, &QAudioOutput::volumeChanged, this, &Player::volumeChanged);
    connect(&m_out, &QAudioOutput::mutedChanged, this, &Player::volumeChanged);
    auto *devices = new QMediaDevices(this);
    connect(devices, &QMediaDevices::audioOutputsChanged, this, &Player::outputsChanged);
    m_saveTimer.setInterval(5000);
    connect(&m_saveTimer, &QTimer::timeout, this, &Player::saveState);
    m_sleep.setSingleShot(true);
    connect(&m_sleep, &QTimer::timeout, this, [this] {
        pause();
        emit sleepChanged();
    });
    restoreState();
}

Player::~Player()
{
    saveState();
}

Player *Player::instance()
{
    static Player *p = new Player(QCoreApplication::instance());
    return p;
}

Player *Player::create(QQmlEngine *, QJSEngine *)
{
    auto *p = instance();
    QJSEngine::setObjectOwnership(p, QJSEngine::CppOwnership);
    return p;
}

bool Player::buffering() const
{
    const auto s = m_player.mediaStatus();
    return s == QMediaPlayer::LoadingMedia || s == QMediaPlayer::BufferingMedia || s == QMediaPlayer::StalledMedia;
}

qint64 Player::duration() const
{
    const qint64 d = m_player.duration();
    return d > 0 ? d : current().value(QStringLiteral("duration")).toLongLong();
}

void Player::setVolume(qreal v)
{
    m_out.setVolume(std::clamp(v, 0.0, 1.0));
    state()->setValue(QStringLiteral("player/volume"), m_out.volume());
}

void Player::setMuted(bool m)
{
    m_out.setMuted(m);
}

void Player::setShuffle(bool on)
{
    if (m_shuffle == on)
        return;
    m_shuffle = on;
    reshuffle();
    state()->setValue(QStringLiteral("player/shuffle"), on);
    emit modeChanged();
}

void Player::setRepeat(const QString &r)
{
    const QString v = r == QLatin1String("all") || r == QLatin1String("one") ? r : QStringLiteral("off");
    if (m_repeat == v)
        return;
    m_repeat = v;
    state()->setValue(QStringLiteral("player/repeat"), v);
    emit modeChanged();
}

void Player::setRate(qreal r)
{
    m_player.setPlaybackRate(std::clamp(r, 0.5, 3.0));
    if (!m_bookKey.isEmpty())
        state()->setValue(QStringLiteral("player/bookRate"), m_player.playbackRate());
}

qint64 Player::sleepRemaining() const
{
    if (m_sleepAtEnd)
        return -1;
    return m_sleep.isActive() ? m_sleep.remainingTime() : 0;
}

void Player::setSleep(int minutes)
{
    m_sleep.stop();
    m_sleepAtEnd = minutes < 0;
    if (minutes > 0)
        m_sleep.start(minutes * 60 * 1000);
    emit sleepChanged();
}

QVariantList Player::outputs() const
{
    QVariantList out;
    for (const QAudioDevice &d : QMediaDevices::audioOutputs())
        out << QVariantMap{{QStringLiteral("id"), QString::fromUtf8(d.id())}, {QStringLiteral("name"), d.description()}, {QStringLiteral("default"), d.isDefault()}};
    return out;
}

QString Player::output() const
{
    return QString::fromUtf8(m_out.device().id());
}

void Player::setOutput(const QString &id)
{
    for (const QAudioDevice &d : QMediaDevices::audioOutputs())
        if (QString::fromUtf8(d.id()) == id) {
            m_out.setDevice(d);
            emit outputsChanged();
            return;
        }
}

void Player::setError(const QString &e)
{
    if (m_error == e)
        return;
    m_error = e;
    emit errorChanged();
}

// ---------- queue ----------

void Player::reshuffle()
{
    m_order.clear();
    for (int i = 0; i < m_queue.size(); ++i)
        m_order << i;
    if (m_shuffle) {
        // The playing track stays first, the rest come in a random order.
        std::shuffle(m_order.begin(), m_order.end(), *QRandomGenerator::global());
        const int at = m_order.indexOf(m_index);
        if (at > 0)
            m_order.move(at, 0);
    }
}

int Player::nextIndex(int from, bool wrap) const
{
    if (m_queue.isEmpty())
        return -1;
    const int pos = m_order.indexOf(from);
    if (pos < 0)
        return m_order.value(0, -1);
    if (pos + 1 < m_order.size())
        return m_order.at(pos + 1);
    return wrap ? m_order.first() : -1;
}

void Player::playTracks(const QVariantList &tracks, int start)
{
    if (tracks.isEmpty())
        return;
    m_queue = tracks;
    m_index = std::clamp(start, 0, int(tracks.size()) - 1);
    m_bookKey.clear();
    reshuffle();
    emit queueChanged();
    load(true);
    MediaLibrary::instance()->noteAlbumPlayed(current().value(QStringLiteral("albumKey")).toString());
}

void Player::playBook(const QString &key, int chapter)
{
    const QVariantMap book = MediaLibrary::instance()->book(key);
    const QVariantList tracks = book.value(QStringLiteral("tracks")).toList();
    if (tracks.isEmpty())
        return;
    const bool resume = chapter < 0;
    const int ch = resume ? book.value(QStringLiteral("chapter")).toInt() : chapter;
    m_queue = tracks;
    m_index = std::clamp(ch, 0, int(tracks.size()) - 1);
    m_bookKey = key;
    m_shuffle = false;
    reshuffle();
    emit queueChanged();
    emit modeChanged();
    load(true, resume ? book.value(QStringLiteral("position")).toLongLong() : 0);
    m_player.setPlaybackRate(state()->value(QStringLiteral("player/bookRate"), 1.0).toDouble());
}

void Player::playRadio(const QVariantMap &station)
{
    QVariantMap t = station;
    t.insert(QStringLiteral("kind"), QStringLiteral("radio"));
    if (!t.contains(QStringLiteral("title")))
        t.insert(QStringLiteral("title"), station.value(QStringLiteral("name")));
    m_queue = {t};
    m_index = 0;
    m_bookKey.clear();
    reshuffle();
    emit queueChanged();
    load(true);
}

void Player::enqueue(const QVariantList &tracks)
{
    if (!m_bookKey.isEmpty() || (hasMedia() && kindOf(current()) == QLatin1String("radio"))) {
        playTracks(tracks, 0);
        return;
    }
    const bool empty = m_queue.isEmpty();
    for (const QVariant &t : tracks) {
        m_queue << t;
        m_order << int(m_queue.size()) - 1;
    }
    emit queueChanged();
    if (empty) {
        m_index = 0;
        load(false);
    }
}

void Player::playNext(const QVariantList &tracks)
{
    if (!hasMedia() || !m_bookKey.isEmpty()) {
        playTracks(tracks, 0);
        return;
    }
    int at = m_index + 1;
    for (const QVariant &t : tracks)
        m_queue.insert(at++, t);
    reshuffle();
    // Played next means next, also when shuffle is on.
    if (m_shuffle)
        for (int i = 0; i < tracks.size(); ++i)
            m_order.move(m_order.indexOf(m_index + 1 + i), 1 + i);
    emit queueChanged();
}

void Player::removeAt(int i)
{
    if (i < 0 || i >= m_queue.size())
        return;
    if (i == m_index) {
        if (m_queue.size() == 1) {
            clearQueue();
            return;
        }
        m_queue.removeAt(i);
        if (m_index >= m_queue.size())
            m_index = 0;
        reshuffle();
        emit queueChanged();
        load(playing());
        return;
    }
    m_queue.removeAt(i);
    if (i < m_index)
        --m_index;
    reshuffle();
    emit queueChanged();
    emit currentChanged();
}

void Player::move(int from, int to)
{
    if (from < 0 || to < 0 || from >= m_queue.size() || to >= m_queue.size() || from == to)
        return;
    m_queue.move(from, to);
    if (m_index == from)
        m_index = to;
    else if (from < m_index && to >= m_index)
        --m_index;
    else if (from > m_index && to <= m_index)
        ++m_index;
    reshuffle();
    emit queueChanged();
    emit currentChanged();
}

void Player::jump(int i)
{
    if (i < 0 || i >= m_queue.size())
        return;
    m_index = i;
    load(true);
}

void Player::clearQueue()
{
    m_player.stop();
    m_player.setSource({});
    m_queue.clear();
    m_order.clear();
    m_index = -1;
    m_bookKey.clear();
    m_lyrics.clear();
    emit queueChanged();
    emit currentChanged();
    emit lyricsChanged();
    saveState();
}

// ---------- transport ----------

void Player::load(bool autoplay, qint64 startAt)
{
    setError({});
    const QVariantMap t = current();
    const QString url = t.value(QStringLiteral("url")).toString();
    m_pendingSeek = startAt > 0 ? startAt : -1;
    m_player.setSource(QUrl(url));
    if (startAt > 0)
        m_player.setPosition(startAt);
    if (autoplay)
        m_player.play();
    emit currentChanged();
    emit durationChanged();
    loadExtras();
    saveState();
}

void Player::finished()
{
    if (m_sleepAtEnd) {
        m_sleepAtEnd = false;
        emit sleepChanged();
        if (!m_bookKey.isEmpty())
            MediaLibrary::instance()->setBookProgress(m_bookKey, std::min(m_index + 1, int(m_queue.size()) - 1), 0);
        return;
    }
    if (m_repeat == QLatin1String("one")) {
        m_player.setPosition(0);
        m_player.play();
        return;
    }
    const int n = nextIndex(m_index, m_repeat == QLatin1String("all"));
    if (n < 0) {
        if (!m_bookKey.isEmpty())
            MediaLibrary::instance()->setBookProgress(m_bookKey, m_index, duration());
        m_player.stop();
        m_player.setPosition(0);
        return;
    }
    m_index = n;
    load(true);
}

void Player::toggle()
{
    if (playing())
        pause();
    else
        play();
}

void Player::play()
{
    if (!hasMedia())
        return;
    if (m_player.source().isEmpty())
        load(true);
    else
        m_player.play();
}

void Player::pause()
{
    // Radio has nothing to resume, so pause stops the stream.
    if (hasMedia() && kindOf(current()) == QLatin1String("radio"))
        m_player.stop();
    else
        m_player.pause();
}

void Player::stop()
{
    m_player.stop();
}

void Player::next()
{
    const int n = nextIndex(m_index, true);
    if (n >= 0 && n != m_index) {
        m_index = n;
        load(playing() || m_player.playbackState() == QMediaPlayer::StoppedState);
    }
}

void Player::previous()
{
    // Past three seconds, Previous goes back to the start of the track first.
    if (m_player.position() > 3000 || m_order.indexOf(m_index) <= 0) {
        m_player.setPosition(0);
        return;
    }
    m_index = m_order.at(m_order.indexOf(m_index) - 1);
    load(true);
}

void Player::seek(qint64 ms)
{
    m_player.setPosition(std::max<qint64>(0, ms));
}

void Player::seekBy(qint64 ms)
{
    seek(m_player.position() + ms);
}

// ---------- lyrics and colour ----------

QVariantList Player::parseLrc(const QString &text)
{
    static const QRegularExpression stamp(QStringLiteral("\\[(\\d+):(\\d+(?:[.:]\\d+)?)\\]"));
    QList<QPair<qint64, QString>> lines;
    for (const QString &raw : text.split(QLatin1Char('\n'))) {
        QList<qint64> times;
        auto it = stamp.globalMatch(raw);
        int end = 0;
        while (it.hasNext()) {
            const auto m = it.next();
            if (m.capturedStart() != end)
                break;
            end = m.capturedEnd();
            times << qint64((m.captured(1).toInt() * 60 + m.captured(2).replace(QLatin1Char(':'), QLatin1Char('.')).toDouble()) * 1000);
        }
        const QString words = raw.mid(end).trimmed();
        for (qint64 t : times)
            lines << qMakePair(t, words);
    }
    std::stable_sort(lines.begin(), lines.end(), [](const auto &a, const auto &b) { return a.first < b.first; });
    QVariantList out;
    for (const auto &l : lines)
        out << QVariantMap{{QStringLiteral("time"), l.first}, {QStringLiteral("text"), l.second}};
    return out;
}

QColor Player::coverColour(const QImage &image)
{
    if (image.isNull())
        return {};
    const QImage small = image.scaled(24, 24, Qt::IgnoreAspectRatio, Qt::SmoothTransformation).convertToFormat(QImage::Format_RGB32);
    QHash<int, int> buckets;
    QHash<int, QColor> sample;
    qint64 r = 0, g = 0, b = 0;
    for (int y = 0; y < small.height(); ++y)
        for (int x = 0; x < small.width(); ++x) {
            const QColor c = small.pixelColor(x, y);
            r += c.red();
            g += c.green();
            b += c.blue();
            if (c.hsvSaturationF() < .25 || c.valueF() < .2)
                continue;
            const int key = c.hsvHue() / 20;
            buckets[key]++;
            sample.insert(key, c);
        }
    int best = -1, count = 0;
    for (auto it = buckets.cbegin(); it != buckets.cend(); ++it)
        if (it.value() > count) {
            best = it.key();
            count = it.value();
        }
    if (best >= 0 && count >= 12)
        return sample.value(best);
    const int n = small.width() * small.height();
    return QColor(int(r / n), int(g / n), int(b / n));
}

void Player::loadExtras()
{
    const QVariantMap t = current();
    const QString path = t.value(QStringLiteral("path")).toString();
    const QString cover = QUrl(t.value(QStringLiteral("cover")).toString()).toLocalFile();
    const int gen = ++m_extrasGeneration;
    m_lyrics.clear();
    m_lyricIndex = -1;
    emit lyricsChanged();
    emit lyricIndexChanged();
    struct Extras { QVariantList lyrics; QColor colour; };
    QtConcurrent::run([path, cover] {
        Extras e;
        if (!cover.isEmpty())
            e.colour = coverColour(QImage(cover));
        if (path.isEmpty())
            return e;
        // Synced lyrics from an .lrc next to the file, else the lyrics tag.
        const QFileInfo fi(path);
        QFile lrc(fi.absolutePath() + QLatin1Char('/') + fi.completeBaseName() + QStringLiteral(".lrc"));
        if (lrc.open(QIODevice::ReadOnly | QIODevice::Text)) {
            e.lyrics = parseLrc(QString::fromUtf8(lrc.readAll()));
            return e;
        }
        TagLib::FileRef f(QFile::encodeName(path).constData(), false);
        if (!f.isNull()) {
            const TagLib::PropertyMap props = f.properties();
            const auto it = props.find("LYRICS");
            if (it != props.end() && !it->second.isEmpty()) {
                const QString text = QString::fromStdString(it->second.front().to8Bit(true));
                e.lyrics = parseLrc(text);
                if (e.lyrics.isEmpty())
                    for (const QString &line : text.split(QLatin1Char('\n')))
                        e.lyrics << QVariantMap{{QStringLiteral("time"), -1}, {QStringLiteral("text"), line.trimmed()}};
            }
        }
        return e;
    }).then(this, [this, gen](const Extras &e) {
        if (gen != m_extrasGeneration)
            return;
        m_lyrics = e.lyrics;
        emit lyricsChanged();
        updateLyricIndex();
        if (e.colour.isValid() && e.colour != m_artColor) {
            m_artColor = e.colour;
            emit artColorChanged();
        }
    });
}

void Player::updateLyricIndex()
{
    int i = -1;
    const qint64 pos = m_player.position() + 200;
    for (int k = 0; k < m_lyrics.size(); ++k) {
        const qint64 t = m_lyrics.at(k).toMap().value(QStringLiteral("time")).toLongLong();
        if (t < 0)
            break;
        if (t <= pos)
            i = k;
        else
            break;
    }
    if (i != m_lyricIndex) {
        m_lyricIndex = i;
        emit lyricIndexChanged();
    }
}

// ---------- state across restarts ----------

void Player::saveState()
{
    QSettings *s = state();
    if (!m_bookKey.isEmpty() && hasMedia())
        MediaLibrary::instance()->setBookProgress(m_bookKey, m_index, m_player.position());
    QStringList paths;
    for (const QVariant &v : m_queue)
        paths << v.toMap().value(QStringLiteral("path")).toString();
    const bool radio = hasMedia() && kindOf(current()) == QLatin1String("radio");
    s->setValue(QStringLiteral("player/queue"), radio ? QStringList() : paths);
    s->setValue(QStringLiteral("player/radio"), radio ? current() : QVariantMap());
    s->setValue(QStringLiteral("player/index"), m_index);
    s->setValue(QStringLiteral("player/position"), m_player.position());
    s->setValue(QStringLiteral("player/book"), m_bookKey);
}

void Player::restoreState()
{
    QSettings *s = state();
    const QStringList paths = s->value(QStringLiteral("player/queue")).toStringList();
    const QVariantMap radio = s->value(QStringLiteral("player/radio")).toMap();
    const int index = s->value(QStringLiteral("player/index"), -1).toInt();
    const qint64 position = s->value(QStringLiteral("player/position"), 0).toLongLong();
    const QString book = s->value(QStringLiteral("player/book")).toString();
    if (!radio.isEmpty()) {
        m_queue = {radio};
        m_index = 0;
        reshuffle();
        emit queueChanged();
        emit currentChanged();
        return;
    }
    if (paths.isEmpty())
        return;
    // The library loads its cache on a worker; the queue comes back once the tracks are known.
    auto apply = [this, paths, index, position, book] {
        auto *lib = MediaLibrary::instance();
        QVariantList q;
        for (const QString &p : paths) {
            const QVariantMap t = lib->track(p);
            if (!t.isEmpty())
                q << t;
        }
        if (q.isEmpty() || hasMedia())
            return;
        m_queue = q;
        m_index = std::clamp(index, 0, int(q.size()) - 1);
        m_bookKey = book;
        reshuffle();
        emit queueChanged();
        load(false, position);
    };
    auto *lib = MediaLibrary::instance();
    if (lib->loaded()) {
        apply();
    } else {
        auto conn = std::make_shared<QMetaObject::Connection>();
        *conn = connect(lib, &MediaLibrary::changed, this, [conn, apply, lib] {
            if (!lib->loaded())
                return;
            QObject::disconnect(*conn);
            apply();
        });
    }
}
