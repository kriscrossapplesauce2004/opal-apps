#pragma once
#include "entrymodel.h"

#include <QProcess>
#include <QRegularExpression>
#include <QTimer>
#include <deque>

// Recursive search under a folder. Results stream into the model while the walk runs.
//
// QML element "Search" (roles and selection API come from EntryModel; content hits also fill snippet and line).
// Properties: root (QUrl), query (string), mode ("name" or "content"), showHidden (bool), maxResults (default 5000),
//   running, truncated (the cap was hit), error, contentSupported (true when `rg` is installed), scanned (folders walked).
// A query with * or ? is a glob that must match the whole name; any other query is a case-insensitive substring.
// Methods: start() (also restarts), cancel(), clear().
// Signals: finished(), started().
// The name walk uses async GIO with at most four folder reads in flight. Content mode runs `rg --json` as a process.
class Search : public EntryModel
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QUrl root READ root WRITE setRoot NOTIFY rootChanged)
    Q_PROPERTY(QString query READ query WRITE setQuery NOTIFY queryChanged)
    Q_PROPERTY(QString mode READ mode WRITE setMode NOTIFY modeChanged)
    Q_PROPERTY(bool showHidden READ showHidden WRITE setShowHidden NOTIFY queryChanged)
    Q_PROPERTY(int maxResults READ maxResults WRITE setMaxResults NOTIFY queryChanged)
    Q_PROPERTY(bool running READ running NOTIFY runningChanged)
    Q_PROPERTY(bool truncated READ truncated NOTIFY runningChanged)
    Q_PROPERTY(QString error READ error NOTIFY runningChanged)
    Q_PROPERTY(bool contentSupported READ contentSupported CONSTANT)
    Q_PROPERTY(int scanned READ scanned NOTIFY runningChanged)

public:
    explicit Search(QObject *parent = nullptr);
    ~Search() override;

    QUrl root() const { return m_root; }
    void setRoot(const QUrl &u);
    QString query() const { return m_query; }
    void setQuery(const QString &q);
    QString mode() const { return m_content ? QStringLiteral("content") : QStringLiteral("name"); }
    void setMode(const QString &m);
    bool showHidden() const { return m_showHidden; }
    void setShowHidden(bool on) { m_showHidden = on; emit queryChanged(); }
    int maxResults() const { return m_max; }
    void setMaxResults(int n) { m_max = std::max(1, n); emit queryChanged(); }
    bool running() const { return m_running; }
    bool truncated() const { return m_truncated; }
    QString error() const { return m_error; }
    static bool contentSupported();
    int scanned() const { return m_scanned; }

    Q_INVOKABLE void start();
    Q_INVOKABLE void cancel();
    Q_INVOKABLE void clear();

signals:
    void rootChanged();
    void queryChanged();
    void modeChanged();
    void runningChanged();
    void started();
    void finished();

private:
    struct Dir;
    void stop();
    void pump();
    void readDir(const Gio::File &dir);
    void nextFiles(const std::shared_ptr<Dir> &d);
    bool matches(const QString &name) const;
    void addResult(std::shared_ptr<Entry> e);
    void flush();
    void finishIfIdle();
    void setRunning(bool on);
    void startContent();
    void parseRg();
    void fillInfo(const std::shared_ptr<Entry> &e);

    QUrl m_root;
    QString m_query, m_error;
    bool m_content = false, m_showHidden = false, m_running = false, m_truncated = false;
    int m_max = 5000, m_scanned = 0, m_active = 0;
    quint64 m_gen = 0;
    QRegularExpression m_glob;
    Gio::Cancellable m_cancel;
    std::deque<Gio::File> m_queue;
    std::vector<std::shared_ptr<Entry>> m_store, m_pending;
    QTimer m_flushTimer;
    QProcess *m_proc = nullptr;
    QByteArray m_buf;
    int m_contentFiles = 0;
};
