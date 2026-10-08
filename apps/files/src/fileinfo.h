#pragma once
#include "gobj.h"

#include <QDateTime>
#include <QObject>
#include <QSize>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

// Details of one file for a properties dialog, plus launching files with applications.
//
// QML element "FileInfo". Set `uri` (URI or absolute path); the data loads async and `ready` turns true.
// Properties: uri, ready, error, name, displayName, path, contentType, kindName, iconName, isDir, isSymlink, symlinkTarget,
//   size, sizeText, modified, accessed, created (QDateTime), modifiedText, permissions (int mode, 0..07777),
//   permissionsOctal ("644"), permissionsText ("rw-r--r--"), owner, group, readable, writable, executable,
//   canSetPermissions, imageSize (QSize, images only), dimensionsText ("4032 x 3024"), openWith (list of
//   {id, name, iconName, isDefault}), inSandbox (true in Flatpak).
// Methods: setPermissions(mode), launch(uri, appId), launchDefault(uris), reload().
// Signals: loaded(), permissionsSet(), launchFinished(ok, message), failed(message).
// Inside Flatpak, launching goes through the xdg-desktop-portal OpenURI/OpenFile calls.
class FileInfo : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QString uri READ uri WRITE setUri NOTIFY uriChanged)
    Q_PROPERTY(bool ready READ ready NOTIFY loadedChanged)
    Q_PROPERTY(QString error READ error NOTIFY loadedChanged)
    Q_PROPERTY(QString name READ name NOTIFY loadedChanged)
    Q_PROPERTY(QString displayName READ displayName NOTIFY loadedChanged)
    Q_PROPERTY(QString path READ path NOTIFY loadedChanged)
    Q_PROPERTY(QString contentType READ contentType NOTIFY loadedChanged)
    Q_PROPERTY(QString kindName READ kindName NOTIFY loadedChanged)
    Q_PROPERTY(QString iconName READ iconName NOTIFY loadedChanged)
    Q_PROPERTY(bool isDir READ isDir NOTIFY loadedChanged)
    Q_PROPERTY(bool isSymlink READ isSymlink NOTIFY loadedChanged)
    Q_PROPERTY(QString symlinkTarget READ symlinkTarget NOTIFY loadedChanged)
    Q_PROPERTY(qint64 size READ size NOTIFY loadedChanged)
    Q_PROPERTY(QString sizeText READ sizeText NOTIFY loadedChanged)
    Q_PROPERTY(QDateTime modified READ modified NOTIFY loadedChanged)
    Q_PROPERTY(QDateTime accessed READ accessed NOTIFY loadedChanged)
    Q_PROPERTY(QDateTime created READ created NOTIFY loadedChanged)
    Q_PROPERTY(QString modifiedText READ modifiedText NOTIFY loadedChanged)
    Q_PROPERTY(int permissions READ permissions NOTIFY loadedChanged)
    Q_PROPERTY(QString permissionsOctal READ permissionsOctal NOTIFY loadedChanged)
    Q_PROPERTY(QString permissionsText READ permissionsText NOTIFY loadedChanged)
    Q_PROPERTY(QString owner READ owner NOTIFY loadedChanged)
    Q_PROPERTY(QString group READ group NOTIFY loadedChanged)
    Q_PROPERTY(bool readable READ readable NOTIFY loadedChanged)
    Q_PROPERTY(bool writable READ writable NOTIFY loadedChanged)
    Q_PROPERTY(bool executable READ executable NOTIFY loadedChanged)
    Q_PROPERTY(bool canSetPermissions READ canSetPermissions NOTIFY loadedChanged)
    Q_PROPERTY(QSize imageSize READ imageSize NOTIFY imageSizeChanged)
    Q_PROPERTY(QString dimensionsText READ dimensionsText NOTIFY imageSizeChanged)
    Q_PROPERTY(QVariantList openWith READ openWith NOTIFY loadedChanged)
    Q_PROPERTY(bool inSandbox READ inSandbox CONSTANT)

public:
    explicit FileInfo(QObject *parent = nullptr);
    ~FileInfo() override;

    QString uri() const { return m_uri; }
    void setUri(const QString &uri);
    bool ready() const { return m_ready; }
    QString error() const { return m_error; }
    QString name() const { return m_name; }
    QString displayName() const { return m_displayName; }
    QString path() const { return m_path; }
    QString contentType() const { return m_contentType; }
    QString kindName() const;
    QString iconName() const;
    bool isDir() const { return m_isDir; }
    bool isSymlink() const { return m_isSymlink; }
    QString symlinkTarget() const { return m_symlinkTarget; }
    qint64 size() const { return m_size; }
    QString sizeText() const;
    QDateTime modified() const { return m_modified; }
    QDateTime accessed() const { return m_accessed; }
    QDateTime created() const { return m_created; }
    QString modifiedText() const;
    int permissions() const { return m_mode & 07777; }
    QString permissionsOctal() const;
    QString permissionsText() const;
    QString owner() const { return m_owner; }
    QString group() const { return m_group; }
    bool readable() const { return m_readable; }
    bool writable() const { return m_writable; }
    bool executable() const { return m_executable; }
    bool canSetPermissions() const { return m_canSetMode; }
    QSize imageSize() const { return m_imageSize; }
    QString dimensionsText() const;
    QVariantList openWith() const { return m_openWith; }
    static bool inSandbox();

    Q_INVOKABLE void reload();
    Q_INVOKABLE void setPermissions(int mode);
    Q_INVOKABLE void launch(const QString &uri, const QString &appId);
    Q_INVOKABLE void launchDefault(const QStringList &uris);

    // Returns true when the portal path would be used; exposed for tests.
    static QString portalPathForTests() { return QStringLiteral("/.flatpak-info"); }

signals:
    void uriChanged();
    void loadedChanged();
    void imageSizeChanged();
    void permissionsSet();
    void launchFinished(bool ok, const QString &message);
    void failed(const QString &message);

private:
    void clear();
    void buildOpenWith();
    void readImageSize();
    void launchViaPortal(const QStringList &uris);
    void launchApp(const QString &appId, const QStringList &uris);

    QString m_uri, m_error, m_name, m_displayName, m_path, m_contentType, m_symlinkTarget, m_owner, m_group;
    QDateTime m_modified, m_accessed, m_created;
    qint64 m_size = 0;
    quint32 m_mode = 0;
    QSize m_imageSize;
    QVariantList m_openWith;
    bool m_ready = false, m_isDir = false, m_isSymlink = false, m_readable = false, m_writable = false, m_executable = false;
    bool m_canSetMode = false;
    int m_gen = 0;
    Gio::Cancellable m_cancel;
};
