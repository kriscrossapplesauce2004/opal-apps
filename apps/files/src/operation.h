#pragma once
#include "gobj.h"

#include <QElapsedTimer>
#include <QObject>
#include <QPointer>
#include <QProcess>
#include <QSet>
#include <deque>
#include <functional>
#include <vector>

// One running file operation. Private to the FileOperations module; OperationsModel shows it, FileOperations creates it.
// All GIO calls are async and started on the GUI thread, so every callback also arrives there.
// Finds a free name for target in its folder ("name (2).ext"); with tryOriginal the unchanged name is tried first.
// The callback runs on the GUI thread, and not at all when the call is cancelled.
void uniqueNameAsync(const Gio::File &target, bool tryOriginal, GCancellable *cancel, std::function<void(Gio::File)> cont);

class Operation : public QObject
{
    Q_OBJECT
public:
    enum Kind { Copy, Move, Link, Trash, Delete, Restore, EmptyTrash, Extract, Compress };
    enum State { Scanning, Running, Paused, Conflict, Done, Failed, Cancelled };
    enum Action { Replace, Skip, KeepBoth, CancelAll };
    struct Pair
    {
        Gio::File src, dst; // dst is the final target; empty for trash and delete
    };

    Operation(int id, Kind kind, QObject *parent = nullptr);
    ~Operation() override;

    // Input, set before start().
    std::vector<Pair> pairs;
    Gio::File destFolder;
    QString archiveName; // compress: file name of the archive
    bool intoFolder = true; // extract: use a folder named after the archive
    bool record = true; // false for undo operations
    std::vector<Gio::File> origins; // restore by original place (undo of trash)

    int id() const { return m_id; }
    Kind kind() const { return m_kind; }
    State state() const { return m_state; }
    QString stateName() const;
    QString title() const;
    QString detail() const;
    QString error() const { return m_errors.isEmpty() ? QString() : m_errors.first(); }
    QString destination() const { return m_destName; }
    double progress() const;
    QString etaText() const;
    bool finished() const { return m_state >= Done; }
    bool canPause() const { return m_kind != Link && m_kind != Extract && m_kind != Compress && !finished(); }
    bool paused() const { return m_state == Paused; }
    qint64 bytesDone() const { return m_bytesDone + m_curBytes; }
    qint64 bytesTotal() const { return m_bytesTotal; }
    int itemsDone() const { return m_itemsDone; }
    int itemsTotal() const { return m_itemsTotal; }
    const std::vector<Pair> &succeeded() const { return m_ok; }

    void start();
    void cancel();
    void pause();
    void resume();
    void resolve(Action a, bool all);

signals:
    void changed();
    void conflictRequested(int id, const QString &name, const QString &sourceUri, const QString &destUri, bool isDir);
    void done(int id);

private:
    struct Task
    {
        Gio::File src, dst;
        std::string name;
        qint64 size = 0;
        int parent = -1;
        int top = 0;
        bool isDir = false;
        bool skipped = false;
    };

    void setState(State s);
    void touch(bool force = false);
    void finish(State s);
    void step(std::function<void()> fn);
    void addError(const QString &msg);
    void sample();
    double workDone() const;
    double workTotal() const;
    void findDestName();
    QString subject() const;

    void startCopy();
    void startMove();
    void startLink();
    void startTrash();
    void startDelete();
    void startRestore();
    void startEmptyTrash();
    void startProcess();
    void procNext(size_t k);
    void finishOk();

    void scan(const std::vector<int> &tops);
    void scanTop(size_t k);
    void scanDirs();
    void scanDone();
    void runTasks(int i);
    void copyFile(int i, bool overwrite);
    void makeDir(int i);
    void copyDone();
    void deleteTasks(int i, bool keepFilter);
    void moveNext(size_t k);
    void moveOne(size_t k, bool overwrite);
    void trashNext(size_t k);
    void resolveRestore(size_t k);
    Gio::File dstOf(int i);
    void askConflict(const Gio::File &src, const Gio::File &dst, bool isDir, std::function<void(Action)> cont);
    void uniqueName(const Gio::File &target, std::function<void(Gio::File)> cont);
    void markSkipped(int i, bool keepSource);

    const int m_id;
    const Kind m_kind;
    State m_state = Scanning;
    QStringList m_errors;
    QString m_destName;
    Gio::Cancellable m_cancel;
    bool m_cancelled = false, m_paused = false, m_useBytes = false, m_allSet = false;
    Action m_allAction = Skip;
    std::function<void()> m_held;
    std::function<void(Action)> m_resolveCb;
    std::vector<Task> m_tasks;
    std::vector<int> m_scanTops;
    std::vector<size_t> m_fallback;
    std::vector<QString> m_infoFor; // restore: the trashinfo file of each pair, if any
    std::deque<int> m_dirQueue;
    QSet<int> m_keepTops;
    std::vector<Pair> m_ok;
    qint64 m_bytesTotal = 0, m_bytesDone = 0, m_curBytes = 0, m_curTotal = 0;
    int m_itemsTotal = 0, m_itemsDone = 0;
    bool m_indeterminate = false;
    QElapsedTimer m_clock, m_lastEmit, m_lastSample;
    double m_rate = 0, m_lastWork = 0;
    QProcess *m_proc = nullptr;
    QString m_partial;
};
