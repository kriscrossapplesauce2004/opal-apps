#pragma once
#include <QDir>
#include <QObject>
#include <QUrl>
#include <QtQml/qqmlregistration.h>

// Start-up settings for the debug UI and tests.
// QML: singleton "FilesEnv". startLocation is $OPAL_FILES_LOCATION when set, else the home folder.
// query is $OPAL_FILES_QUERY, used to pre-fill the debug search field.
// demoDest is $OPAL_FILES_DEMO, a folder where the debug UI copies the start folder once, to show an operation row.
class FilesEnv : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(QUrl startLocation READ startLocation CONSTANT)
    Q_PROPERTY(QString query READ query CONSTANT)
    Q_PROPERTY(QString demoDest READ demoDest CONSTANT)

public:
    explicit FilesEnv(QObject *parent = nullptr) : QObject(parent) {}
    QUrl startLocation() const
    {
        const QString v = qEnvironmentVariable("OPAL_FILES_LOCATION");
        return QUrl::fromUserInput(v.isEmpty() ? QDir::homePath() : v);
    }
    QString query() const { return qEnvironmentVariable("OPAL_FILES_QUERY"); }
    QString demoDest() const { return qEnvironmentVariable("OPAL_FILES_DEMO"); }
};
