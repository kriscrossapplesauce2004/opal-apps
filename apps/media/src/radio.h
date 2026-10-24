#pragma once
#include <QNetworkAccessManager>
#include <QObject>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

// Internet radio: favourite stations and the radio-browser.info directory. The network is used only when the user searches
// or opens the Radio page. QML: singleton "Radio".
// Properties: favourites ([{id, name, url, favicon, tags, country, codec, bitrate}]), results, busy, error.
// Methods: search(query), popular(), isFavourite(url), toggleFavourite(station), addCustom(name, url).
class Radio : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(QVariantList favourites READ favourites NOTIFY favouritesChanged)
    Q_PROPERTY(QVariantList results READ results NOTIFY resultsChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY resultsChanged)
    Q_PROPERTY(QString error READ error NOTIFY resultsChanged)

public:
    explicit Radio(QObject *parent = nullptr);

    QVariantList favourites() const { return m_favourites; }
    QVariantList results() const { return m_results; }
    bool busy() const { return m_busy; }
    QString error() const { return m_error; }

    Q_INVOKABLE void search(const QString &query);
    Q_INVOKABLE void popular();
    Q_INVOKABLE bool isFavourite(const QString &url) const;
    Q_INVOKABLE void toggleFavourite(const QVariantMap &station);
    Q_INVOKABLE bool addCustom(const QString &name, const QString &url);

    // Turns one radio-browser station into the map the interface uses. Public for tests.
    static QVariantMap fromDirectory(const QVariantMap &raw);

signals:
    void favouritesChanged();
    void resultsChanged();

private:
    void fetch(const QString &path, const QList<QPair<QString, QString>> &query);
    void saveFavourites();

    QNetworkAccessManager m_net;
    QVariantList m_favourites;
    QVariantList m_results;
    bool m_busy = false;
    QString m_error;
    int m_request = 0;
};
