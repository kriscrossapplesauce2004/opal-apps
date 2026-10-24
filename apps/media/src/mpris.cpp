#include "mpris.h"
#include "player.h"

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QFileInfo>
#include <QUrl>

namespace {
const QString ObjectPath = QStringLiteral("/org/mpris/MediaPlayer2");
const QString PlayerInterface = QStringLiteral("org.mpris.MediaPlayer2.Player");
}

Mpris::Mpris(Player *player, QObject *parent)
    : QObject(parent), m_player(player)
{
    new MprisRoot(this);
    auto *adaptor = new MprisPlayer(this);
    QDBusConnection bus = QDBusConnection::sessionBus();
    if (!bus.isConnected())
        return;
    // A second window of Media gets its own name, so both stay controllable.
    QString name = QStringLiteral("org.mpris.MediaPlayer2.opal_media");
    if (!bus.registerService(name))
        bus.registerService(name += QStringLiteral(".instance%1").arg(QCoreApplication::applicationPid()));
    m_registered = bus.registerObject(ObjectPath, this);

    connect(player, &Player::playingChanged, this, [this] { notify({{QStringLiteral("PlaybackStatus"), status()}}); });
    connect(player, &Player::currentChanged, this, [this] {
        notify({{QStringLiteral("Metadata"), metadata()}, {QStringLiteral("CanGoNext"), true}, {QStringLiteral("CanGoPrevious"), true},
                {QStringLiteral("CanPlay"), m_player->hasMedia()}, {QStringLiteral("CanPause"), m_player->hasMedia()}, {QStringLiteral("CanSeek"), m_player->hasMedia()}});
    });
    connect(player, &Player::durationChanged, this, [this] { notify({{QStringLiteral("Metadata"), metadata()}}); });
    connect(player, &Player::artColorChanged, this, [] {});
    connect(player, &Player::volumeChanged, this, [this] { notify({{QStringLiteral("Volume"), m_player->volume()}}); });
    connect(player, &Player::modeChanged, this, [this, adaptor] {
        notify({{QStringLiteral("LoopStatus"), adaptor->loopStatus()}, {QStringLiteral("Shuffle"), m_player->shuffle()}, {QStringLiteral("Rate"), m_player->rate()}});
    });
}

void Mpris::notify(const QVariantMap &changed)
{
    if (!m_registered)
        return;
    QDBusMessage msg = QDBusMessage::createSignal(ObjectPath, QStringLiteral("org.freedesktop.DBus.Properties"), QStringLiteral("PropertiesChanged"));
    msg << PlayerInterface << changed << QStringList();
    QDBusConnection::sessionBus().send(msg);
}

QString Mpris::status() const
{
    if (m_player->playing())
        return QStringLiteral("Playing");
    return m_player->hasMedia() && m_player->position() > 0 ? QStringLiteral("Paused") : QStringLiteral("Stopped");
}

QVariantMap Mpris::metadata() const
{
    const QVariantMap t = m_player->current();
    if (t.isEmpty())
        return {};
    const QString id = QString::fromLatin1(QCryptographicHash::hash(t.value(QStringLiteral("url")).toString().toUtf8(), QCryptographicHash::Md5).toHex());
    QVariantMap m{{QStringLiteral("mpris:trackid"), QVariant::fromValue(QDBusObjectPath(QStringLiteral("/dev/opal/media/track/") + id))},
                  {QStringLiteral("mpris:length"), qlonglong(m_player->duration() * 1000)},
                  {QStringLiteral("xesam:title"), t.value(QStringLiteral("kind")) == QLatin1String("radio") && !t.value(QStringLiteral("nowPlaying")).toString().isEmpty()
                                                       ? t.value(QStringLiteral("nowPlaying")) : t.value(QStringLiteral("title"))},
                  {QStringLiteral("xesam:album"), t.value(QStringLiteral("kind")) == QLatin1String("radio") ? t.value(QStringLiteral("title")) : t.value(QStringLiteral("album"))},
                  {QStringLiteral("xesam:artist"), QStringList{t.value(QStringLiteral("artist")).toString()}},
                  {QStringLiteral("xesam:url"), t.value(QStringLiteral("url"))}};
    const QString cover = t.value(QStringLiteral("cover")).toString();
    if (!cover.isEmpty())
        m.insert(QStringLiteral("mpris:artUrl"), cover);
    if (t.value(QStringLiteral("number")).toInt() > 0)
        m.insert(QStringLiteral("xesam:trackNumber"), t.value(QStringLiteral("number")).toInt());
    return m;
}

// ---------- org.mpris.MediaPlayer2 ----------

MprisRoot::MprisRoot(Mpris *parent)
    : QDBusAbstractAdaptor(parent), m_mpris(parent)
{
}

QStringList MprisRoot::mimeTypes() const
{
    return {QStringLiteral("audio/mpeg"), QStringLiteral("audio/flac"), QStringLiteral("audio/ogg"), QStringLiteral("audio/opus"), QStringLiteral("audio/mp4"),
            QStringLiteral("audio/x-wav"), QStringLiteral("audio/aac")};
}

void MprisRoot::Raise()
{
    emit m_mpris->raise();
}

void MprisRoot::Quit()
{
    QCoreApplication::quit();
}

// ---------- org.mpris.MediaPlayer2.Player ----------

MprisPlayer::MprisPlayer(Mpris *parent)
    : QDBusAbstractAdaptor(parent), m_mpris(parent)
{
    setAutoRelaySignals(false);
}

QString MprisPlayer::playbackStatus() const { return m_mpris->status(); }

QString MprisPlayer::loopStatus() const
{
    const QString r = m_mpris->player()->repeat();
    return r == QLatin1String("one") ? QStringLiteral("Track") : r == QLatin1String("all") ? QStringLiteral("Playlist") : QStringLiteral("None");
}

void MprisPlayer::setLoopStatus(const QString &s)
{
    m_mpris->player()->setRepeat(s == QLatin1String("Track") ? QStringLiteral("one") : s == QLatin1String("Playlist") ? QStringLiteral("all") : QStringLiteral("off"));
}

double MprisPlayer::rate() const { return m_mpris->player()->rate(); }
void MprisPlayer::setRate(double r) { m_mpris->player()->setRate(r); }
bool MprisPlayer::shuffle() const { return m_mpris->player()->shuffle(); }
void MprisPlayer::setShuffle(bool on) { m_mpris->player()->setShuffle(on); }
QVariantMap MprisPlayer::metadata() const { return m_mpris->metadata(); }
double MprisPlayer::volume() const { return m_mpris->player()->volume(); }
void MprisPlayer::setVolume(double v) { m_mpris->player()->setVolume(v); }
qlonglong MprisPlayer::position() const { return m_mpris->player()->position() * 1000; }
bool MprisPlayer::canGoNext() const { return m_mpris->player()->queue().size() > 1; }
bool MprisPlayer::canGoPrevious() const { return m_mpris->player()->hasMedia(); }
bool MprisPlayer::canPlay() const { return m_mpris->player()->hasMedia(); }
bool MprisPlayer::canPause() const { return m_mpris->player()->hasMedia(); }
bool MprisPlayer::canSeek() const { return m_mpris->player()->hasMedia() && m_mpris->player()->current().value(QStringLiteral("kind")) != QLatin1String("radio"); }

void MprisPlayer::Next() { m_mpris->player()->next(); }
void MprisPlayer::Previous() { m_mpris->player()->previous(); }
void MprisPlayer::Pause() { m_mpris->player()->pause(); }
void MprisPlayer::PlayPause() { m_mpris->player()->toggle(); }
void MprisPlayer::Stop() { m_mpris->player()->stop(); }
void MprisPlayer::Play() { m_mpris->player()->play(); }

void MprisPlayer::Seek(qlonglong offset)
{
    m_mpris->player()->seekBy(offset / 1000);
    emit Seeked(m_mpris->player()->position() * 1000);
}

void MprisPlayer::SetPosition(const QDBusObjectPath &, qlonglong position)
{
    m_mpris->player()->seek(position / 1000);
    emit Seeked(position);
}

void MprisPlayer::OpenUri(const QString &uri)
{
    const QUrl url(uri);
    QVariantMap t{{QStringLiteral("url"), uri}, {QStringLiteral("kind"), url.isLocalFile() ? QStringLiteral("music") : QStringLiteral("radio")},
                  {QStringLiteral("title"), url.isLocalFile() ? QFileInfo(url.toLocalFile()).completeBaseName() : url.host()},
                  {QStringLiteral("path"), url.isLocalFile() ? url.toLocalFile() : QString()}};
    if (url.isLocalFile())
        m_mpris->player()->playTracks({t}, 0);
    else
        m_mpris->player()->playRadio(t);
}
