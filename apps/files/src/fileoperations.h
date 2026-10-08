#pragma once
#include "gobj.h"
#include "operation.h"

#include <QAbstractListModel>
#include <QObject>
#include <QStringList>
#include <QtQml/qqmlregistration.h>
#include <deque>
#include <functional>

class QQmlEngine;
class QJSEngine;

// The list of running and finished file operations, for a progress panel.
//
// QML: reached as FileOperations.operations (not creatable).
// Roles: id, kind (copy move link trash delete restore emptyTrash extract compress), title ("Copying 3 items to Samsung T7"),
//   detail ("1.4 of 3.2 GB · 40 s left"), state (scanning running paused conflict done failed cancelled), progress (0..1,
//   -1 when unknown), eta, error, destination, active, canPause, paused, bytesDone, bytesTotal, itemsDone, itemsTotal.
// Properties: count, activeCount, overallProgress (-1 when idle).
// Methods: indexOfId(id), dismiss(id), clearFinished().
class OperationsModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ANONYMOUS
    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(int activeCount READ activeCount NOTIFY countChanged)
    Q_PROPERTY(double overallProgress READ overallProgress NOTIFY countChanged)

public:
    enum Role {
        IdRole = Qt::UserRole + 1,
        KindRole,
        TitleRole,
        DetailRole,
        StateRole,
        ProgressRole,
        EtaRole,
        ErrorRole,
        DestinationRole,
        ActiveRole,
        CanPauseRole,
        PausedRole,
        BytesDoneRole,
        BytesTotalRole,
        ItemsDoneRole,
        ItemsTotalRole,
    };
    Q_ENUM(Role)

    explicit OperationsModel(QObject *parent = nullptr) : QAbstractListModel(parent) {}
    int rowCount(const QModelIndex &parent = {}) const override { return parent.isValid() ? 0 : int(m_ops.size()); }
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    int count() const { return int(m_ops.size()); }
    int activeCount() const;
    double overallProgress() const;

    Q_INVOKABLE int indexOfId(int id) const;
    Q_INVOKABLE void dismiss(int id);
    Q_INVOKABLE void clearFinished();

    void add(Operation *op);
    Operation *byId(int id) const;

signals:
    void countChanged();

private:
    std::vector<Operation *> m_ops;
};

// File operations with progress, conflict handling and undo.
//
// QML: singleton "FileOperations". All sources and destinations are URIs or absolute paths. Every method that starts
// work returns an operation id (or -1) and runs async; watch `operations` for state.
// Properties: operations (OperationsModel), archiveSupported (bsdtar found; hide archive actions otherwise), canUndo, undoText.
// Methods: copy(sources, destFolder), move(sources, destFolder), link(sources, destFolder), trash(sources), remove(sources)
//   (delete for good), restore(trashedUris), emptyTrash(), rename(uri, newName), createFolder(parent, name),
//   createFile(parent, name), extract(archives, destFolder = "", intoFolder = true), compress(sources, destFolder, archiveName),
//   cancel(id), pause(id), resume(id), resolveConflict(id, action, applyToAll), undo().
// Conflict actions: FileOperations.Replace, Skip, KeepBoth ("name (2).ext"), CancelAll.
// Signals: conflictRequested(id, name, sourceUri, destUri, isDir) (answer with resolveConflict), finished(id, state, error),
//   created(uri), renamed(oldUri, newUri), failed(title, message).
// Pause stops between files, not inside one running file copy. Undo covers trash, move, rename, create, copy, link,
// extract and compress, 20 steps deep.
class FileOperations : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(OperationsModel *operations READ operations CONSTANT)
    Q_PROPERTY(bool archiveSupported READ archiveSupported CONSTANT)
    Q_PROPERTY(bool canUndo READ canUndo NOTIFY undoChanged)
    Q_PROPERTY(QString undoText READ undoText NOTIFY undoChanged)

public:
    enum ConflictAction { Replace = Operation::Replace, Skip = Operation::Skip, KeepBoth = Operation::KeepBoth, CancelAll = Operation::CancelAll };
    Q_ENUM(ConflictAction)

    explicit FileOperations(QObject *parent = nullptr);
    static FileOperations *instance();
    static FileOperations *create(QQmlEngine *, QJSEngine *);

    OperationsModel *operations() { return &m_model; }
    static bool archiveSupported();
    bool canUndo() const { return !m_undo.empty(); }
    QString undoText() const { return m_undo.empty() ? QString() : m_undo.back().text; }

    Q_INVOKABLE int copy(const QStringList &sources, const QString &destFolder);
    Q_INVOKABLE int move(const QStringList &sources, const QString &destFolder);
    Q_INVOKABLE int link(const QStringList &sources, const QString &destFolder);
    Q_INVOKABLE int trash(const QStringList &sources);
    Q_INVOKABLE int remove(const QStringList &sources);
    Q_INVOKABLE int restore(const QStringList &trashedUris);
    Q_INVOKABLE int emptyTrash();
    Q_INVOKABLE void rename(const QString &uri, const QString &newName);
    Q_INVOKABLE void createFolder(const QString &parent, const QString &name = QString());
    Q_INVOKABLE void createFile(const QString &parent, const QString &name = QString());
    Q_INVOKABLE int extract(const QStringList &archives, const QString &destFolder = QString(), bool intoFolder = true);
    Q_INVOKABLE int compress(const QStringList &sources, const QString &destFolder, const QString &archiveName);
    Q_INVOKABLE void cancel(int id);
    Q_INVOKABLE void pause(int id);
    Q_INVOKABLE void resume(int id);
    Q_INVOKABLE void resolveConflict(int id, int action, bool applyToAll = false);
    Q_INVOKABLE void undo();

signals:
    void undoChanged();
    void conflictRequested(int id, const QString &name, const QString &sourceUri, const QString &destUri, bool isDir);
    void finished(int id, const QString &state, const QString &error);
    void created(const QString &uri);
    void renamed(const QString &oldUri, const QString &newUri);
    void failed(const QString &title, const QString &message);

private:
    struct UndoStep
    {
        QString text;
        std::function<void()> run;
    };
    Operation *make(Operation::Kind kind, const QStringList &sources, const QString &dest, bool pairWithDest);
    int launch(Operation *op);
    void pushUndo(const QString &text, std::function<void()> run);
    void onDone(int id);
    void setDisplayName(const Gio::File &file, const QString &name, bool recordUndo);
    void create(const QString &parent, const QString &name, bool folder);

    OperationsModel m_model;
    std::deque<UndoStep> m_undo;
    int m_nextId = 1;
};
