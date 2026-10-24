#pragma once
#include <QDBusAbstractAdaptor>
#include <QDBusObjectPath>
#include <QObject>
#include <QStringList>
#include <QVariantMap>

class Player;

// MPRIS on the session bus as org.mpris.MediaPlayer2.opal_media, so Opal's media controls, media keys and playerctl drive the player.
class Mpris : public QObject
{
    Q_OBJECT
public:
    explicit Mpris(Player *player, QObject *parent = nullptr);
    bool registered() const { return m_registered; }
    Player *player() const { return m_player; }
    // Sends org.freedesktop.DBus.Properties.PropertiesChanged for the player interface.
    void notify(const QVariantMap &changed);
    QVariantMap metadata() const;
    QString status() const;

signals:
    void raise();

private:
    Player *m_player;
    bool m_registered = false;
};

class MprisRoot : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.mpris.MediaPlayer2")
    Q_PROPERTY(bool CanQuit READ canQuit CONSTANT)
    Q_PROPERTY(bool CanRaise READ canRaise CONSTANT)
    Q_PROPERTY(bool HasTrackList READ hasTrackList CONSTANT)
    Q_PROPERTY(QString Identity READ identity CONSTANT)
    Q_PROPERTY(QString DesktopEntry READ desktopEntry CONSTANT)
    Q_PROPERTY(QStringList SupportedUriSchemes READ schemes CONSTANT)
    Q_PROPERTY(QStringList SupportedMimeTypes READ mimeTypes CONSTANT)
public:
    explicit MprisRoot(Mpris *parent);
    bool canQuit() const { return true; }
    bool canRaise() const { return true; }
    bool hasTrackList() const { return false; }
    QString identity() const { return QStringLiteral("Media"); }
    QString desktopEntry() const { return QStringLiteral("dev.opal.Media"); }
    QStringList schemes() const { return {QStringLiteral("file"), QStringLiteral("http"), QStringLiteral("https")}; }
    QStringList mimeTypes() const;
public slots:
    void Raise();
    void Quit();
private:
    Mpris *m_mpris;
};

class MprisPlayer : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.mpris.MediaPlayer2.Player")
    Q_PROPERTY(QString PlaybackStatus READ playbackStatus)
    Q_PROPERTY(QString LoopStatus READ loopStatus WRITE setLoopStatus)
    Q_PROPERTY(double Rate READ rate WRITE setRate)
    Q_PROPERTY(bool Shuffle READ shuffle WRITE setShuffle)
    Q_PROPERTY(QVariantMap Metadata READ metadata)
    Q_PROPERTY(double Volume READ volume WRITE setVolume)
    Q_PROPERTY(qlonglong Position READ position)
    Q_PROPERTY(double MinimumRate READ minimumRate CONSTANT)
    Q_PROPERTY(double MaximumRate READ maximumRate CONSTANT)
    Q_PROPERTY(bool CanGoNext READ canGoNext)
    Q_PROPERTY(bool CanGoPrevious READ canGoPrevious)
    Q_PROPERTY(bool CanPlay READ canPlay)
    Q_PROPERTY(bool CanPause READ canPause)
    Q_PROPERTY(bool CanSeek READ canSeek)
    Q_PROPERTY(bool CanControl READ canControl CONSTANT)
public:
    explicit MprisPlayer(Mpris *parent);
    QString playbackStatus() const;
    QString loopStatus() const;
    void setLoopStatus(const QString &s);
    double rate() const;
    void setRate(double r);
    bool shuffle() const;
    void setShuffle(bool on);
    QVariantMap metadata() const;
    double volume() const;
    void setVolume(double v);
    qlonglong position() const;
    double minimumRate() const { return .5; }
    double maximumRate() const { return 3; }
    bool canGoNext() const;
    bool canGoPrevious() const;
    bool canPlay() const;
    bool canPause() const;
    bool canSeek() const;
    bool canControl() const { return true; }
public slots:
    void Next();
    void Previous();
    void Pause();
    void PlayPause();
    void Stop();
    void Play();
    void Seek(qlonglong offset);
    void SetPosition(const QDBusObjectPath &track, qlonglong position);
    void OpenUri(const QString &uri);
signals:
    void Seeked(qlonglong position);
private:
    Mpris *m_mpris;
};
