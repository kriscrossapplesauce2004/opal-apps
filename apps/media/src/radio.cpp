#include "radio.h"

#include <QCoreApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QSettings>
#include <QStandardPaths>
#include <QUrl>
#include <QUrlQuery>

namespace {

QSettings *settings()
{
    static QSettings *s = new QSettings(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + QStringLiteral("/opal/media.ini"),
                                        QSettings::IniFormat, QCoreApplication::instance());
    return s;
}

// radio-browser asks clients to use a mirror; this one answers for the whole pool.
const QString Api = QStringLiteral("https://all.api.radio-browser.info");

}

Radio::Radio(QObject *parent)
    : QObject(parent)
{
    m_favourites = settings()->value(QStringLiteral("radio/favourites")).toList();
    m_net.setTransferTimeout(12000);
}

QVariantMap Radio::fromDirectory(const QVariantMap &raw)
{
    const QString url = raw.value(QStringLiteral("url_resolved")).toString().isEmpty() ? raw.value(QStringLiteral("url")).toString() : raw.value(QStringLiteral("url_resolved")).toString();
    QStringList tags = raw.value(QStringLiteral("tags")).toString().split(QLatin1Char(','), Qt::SkipEmptyParts);
    for (QString &t : tags)
        t = t.trimmed();
    return {{QStringLiteral("id"), raw.value(QStringLiteral("stationuuid"))},
            {QStringLiteral("name"), raw.value(QStringLiteral("name")).toString().trimmed()},
            {QStringLiteral("title"), raw.value(QStringLiteral("name")).toString().trimmed()},
            {QStringLiteral("url"), url},
            {QStringLiteral("cover"), raw.value(QStringLiteral("favicon"))},
            {QStringLiteral("artist"), tags.mid(0, 3).join(QStringLiteral(", "))},
            {QStringLiteral("country"), raw.value(QStringLiteral("country"))},
            {QStringLiteral("codec"), raw.value(QStringLiteral("codec"))},
            {QStringLiteral("bitrate"), raw.value(QStringLiteral("bitrate"))},
            {QStringLiteral("kind"), QStringLiteral("radio")}};
}

void Radio::fetch(const QString &path, const QList<QPair<QString, QString>> &query)
{
    QUrl url(Api + path);
    QUrlQuery q;
    q.setQueryItems(query);
    url.setQuery(q);
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("OpalMedia/1.0"));
    const int id = ++m_request;
    m_busy = true;
    m_error.clear();
    emit resultsChanged();
    QNetworkReply *reply = m_net.get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply, id] {
        reply->deleteLater();
        if (id != m_request)
            return;
        m_busy = false;
        if (reply->error() != QNetworkReply::NoError) {
            m_error = tr("Can't reach the radio directory");
            emit resultsChanged();
            return;
        }
        QVariantList out;
        for (const QJsonValue &v : QJsonDocument::fromJson(reply->readAll()).array()) {
            const QVariantMap s = fromDirectory(v.toObject().toVariantMap());
            if (!s.value(QStringLiteral("url")).toString().isEmpty())
                out << s;
        }
        m_results = out;
        emit resultsChanged();
    });
}

void Radio::search(const QString &query)
{
    if (query.trimmed().isEmpty()) {
        popular();
        return;
    }
    fetch(QStringLiteral("/json/stations/search"), {{QStringLiteral("name"), query.trimmed()}, {QStringLiteral("limit"), QStringLiteral("60")},
                                                    {QStringLiteral("hidebroken"), QStringLiteral("true")}, {QStringLiteral("order"), QStringLiteral("votes")},
                                                    {QStringLiteral("reverse"), QStringLiteral("true")}});
}

void Radio::popular()
{
    fetch(QStringLiteral("/json/stations/topvote/40"), {{QStringLiteral("hidebroken"), QStringLiteral("true")}});
}

bool Radio::isFavourite(const QString &url) const
{
    for (const QVariant &v : m_favourites)
        if (v.toMap().value(QStringLiteral("url")).toString() == url)
            return true;
    return false;
}

void Radio::toggleFavourite(const QVariantMap &station)
{
    const QString url = station.value(QStringLiteral("url")).toString();
    for (int i = 0; i < m_favourites.size(); ++i)
        if (m_favourites.at(i).toMap().value(QStringLiteral("url")).toString() == url) {
            m_favourites.removeAt(i);
            saveFavourites();
            return;
        }
    QVariantMap s = station;
    s.remove(QStringLiteral("nowPlaying"));
    s.insert(QStringLiteral("kind"), QStringLiteral("radio"));
    m_favourites << s;
    saveFavourites();
}

bool Radio::addCustom(const QString &name, const QString &urlText)
{
    const QUrl url = QUrl::fromUserInput(urlText.trimmed());
    if (!url.isValid() || !(url.scheme() == QLatin1String("http") || url.scheme() == QLatin1String("https")))
        return false;
    toggleFavourite({{QStringLiteral("name"), name.trimmed().isEmpty() ? url.host() : name.trimmed()},
                     {QStringLiteral("title"), name.trimmed().isEmpty() ? url.host() : name.trimmed()},
                     {QStringLiteral("url"), url.toString()}, {QStringLiteral("artist"), url.host()}});
    return true;
}

void Radio::saveFavourites()
{
    settings()->setValue(QStringLiteral("radio/favourites"), m_favourites);
    emit favouritesChanged();
}
