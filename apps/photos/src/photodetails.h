#pragma once
#include <QColor>
#include <QDateTime>
#include <QObject>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

// The details panel for one photo. All reads run on a worker thread.
// QML: PhotoDetails { path }. Properties: ready, name, folder, folderName, folderDisplay, taken, dateText, timeText, dateNote
//   (where the date came from when not from EXIF), dimensions, megapixels, sizeText, camera, lens, exposure, hasLocation,
//   latitude, longitude, colours (list of colour), caption, text (OCR), video.
class PhotoDetails : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QString path READ path WRITE setPath NOTIFY pathChanged)
    Q_PROPERTY(bool ready READ ready NOTIFY loaded)
    Q_PROPERTY(QString name READ name NOTIFY loaded)
    Q_PROPERTY(QString folder READ folder NOTIFY loaded)
    Q_PROPERTY(QString folderName READ folderName NOTIFY loaded)
    Q_PROPERTY(QString folderDisplay READ folderDisplay NOTIFY loaded)
    Q_PROPERTY(QDateTime taken READ taken NOTIFY loaded)
    Q_PROPERTY(QString dateText READ dateText NOTIFY loaded)
    Q_PROPERTY(QString timeText READ timeText NOTIFY loaded)
    Q_PROPERTY(QString dateNote READ dateNote NOTIFY loaded)
    Q_PROPERTY(QString dimensions READ dimensions NOTIFY loaded)
    Q_PROPERTY(QString megapixels READ megapixels NOTIFY loaded)
    Q_PROPERTY(QString sizeText READ sizeText NOTIFY loaded)
    Q_PROPERTY(QString camera READ camera NOTIFY loaded)
    Q_PROPERTY(QString lens READ lens NOTIFY loaded)
    Q_PROPERTY(QString exposure READ exposure NOTIFY loaded)
    Q_PROPERTY(bool hasLocation READ hasLocation NOTIFY loaded)
    Q_PROPERTY(double latitude READ latitude NOTIFY loaded)
    Q_PROPERTY(double longitude READ longitude NOTIFY loaded)
    Q_PROPERTY(QString locationText READ locationText NOTIFY loaded)
    Q_PROPERTY(QVariantList colours READ colours NOTIFY coloursChanged)
    Q_PROPERTY(QString caption READ caption NOTIFY loaded)
    Q_PROPERTY(QString text READ text NOTIFY loaded)
    Q_PROPERTY(bool video READ video NOTIFY loaded)

public:
    explicit PhotoDetails(QObject *parent = nullptr);

    QString path() const { return m_path; }
    void setPath(const QString &p);
    bool ready() const { return m_ready; }
    QString name() const { return m_path.section(QLatin1Char('/'), -1); }
    QString folder() const { return m_path.section(QLatin1Char('/'), 0, -2); }
    QString folderName() const { return folder().section(QLatin1Char('/'), -1); }
    QString folderDisplay() const;
    QDateTime taken() const { return m_taken; }
    QString dateText() const;
    QString timeText() const;
    QString dateNote() const { return m_dateNote; }
    QString dimensions() const { return m_dimensions; }
    QString megapixels() const { return m_megapixels; }
    QString sizeText() const { return m_sizeText; }
    QString camera() const { return m_camera; }
    QString lens() const { return m_lens; }
    QString exposure() const { return m_exposure; }
    bool hasLocation() const { return m_hasLocation; }
    double latitude() const { return m_lat; }
    double longitude() const { return m_lon; }
    QString locationText() const;
    QVariantList colours() const { return m_colours; }
    QString caption() const { return m_caption; }
    QString text() const { return m_text; }
    bool video() const { return m_video; }

    // Up to five colours that cover the most of the image, most common first, with near duplicates merged.
    static QList<QColor> dominantColours(const QImage &image, int count = 5);

signals:
    void pathChanged();
    void loaded();
    void coloursChanged();

private:
    void load();

    QString m_path;
    int m_generation = 0;
    bool m_ready = false;
    QDateTime m_taken;
    QString m_dateNote;
    QString m_dimensions;
    QString m_megapixels;
    QString m_sizeText;
    QString m_camera;
    QString m_lens;
    QString m_exposure;
    bool m_hasLocation = false;
    double m_lat = 0;
    double m_lon = 0;
    QVariantList m_colours;
    QString m_caption;
    QString m_text;
    bool m_video = false;
};
