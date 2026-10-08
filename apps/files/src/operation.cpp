#include "operation.h"

#include "format.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QUrl>
#include <QtConcurrent/QtConcurrentRun>
#include <cmath>

namespace {

constexpr int kPrio = G_PRIORITY_DEFAULT;

QString tr(const char *s) { return QCoreApplication::translate("Operation", s); }

// Splits "a.tar.gz" into {"a", ".tar.gz"} and "a.txt" into {"a", ".txt"}.
std::pair<QString, QString> splitName(const QString &n)
{
    static const QStringList dbl = {QStringLiteral(".tar.gz"), QStringLiteral(".tar.xz"), QStringLiteral(".tar.bz2"), QStringLiteral(".tar.zst"),
                                    QStringLiteral(".tar.lz"), QStringLiteral(".tar.lzma"), QStringLiteral(".tar.Z")};
    for (const QString &d : dbl)
        if (n.size() > d.size() && n.endsWith(d, Qt::CaseInsensitive))
            return {n.left(n.size() - d.size()), n.right(d.size())};
    const int dot = n.lastIndexOf(QLatin1Char('.'));
    if (dot <= 0)
        return {n, QString()};
    return {n.left(dot), n.mid(dot)};
}

QString baseName(GFile *f)
{
    return Gio::take(g_file_get_basename(f));
}

struct ProgCtx
{
    QPointer<Operation> op;
    std::function<void(qint64, qint64)> fn;
};

void progressCb(goffset cur, goffset total, gpointer data)
{
    auto *c = static_cast<ProgCtx *>(data);
    if (c->op)
        c->fn(qint64(cur), qint64(total));
}

QString archiveStem(const QString &name)
{
    return splitName(name).first;
}

}

void uniqueNameAsync(const Gio::File &target, bool tryOriginal, GCancellable *cancel, std::function<void(Gio::File)> cont)
{
    auto parent = Gio::File::adopt(g_file_get_parent(target.get()));
    if (!parent) {
        cont(target);
        return;
    }
    const QString name = baseName(target.get());
    auto [stem, ext] = splitName(name);
    static const QRegularExpression numbered(QStringLiteral("^(.*) \\((\\d+)\\)$"));
    const auto m = numbered.match(stem);
    if (m.hasMatch())
        stem = m.captured(1);
    auto cancellable = Gio::Cancellable::retain(cancel);
    auto attempt = std::make_shared<std::function<void(int)>>();
    *attempt = [=](int n) {
        const QString candidateName = n <= 1 ? name : stem + QStringLiteral(" (%1)").arg(n) + ext;
        auto candidate = Gio::File::adopt(g_file_get_child(parent.get(), candidateName.toUtf8().constData()));
        auto [cb, ud] = Gio::thunk([=](GObject *src, GAsyncResult *res) {
            Gio::Error err;
            auto info = Gio::Info::adopt(g_file_query_info_finish(G_FILE(src), res, err.out()));
            if (err.cancelled())
                return;
            if (info)
                (*attempt)(n < 2 ? 2 : n + 1);
            else
                cont(candidate);
        });
        g_file_query_info_async(candidate.get(), "standard::name", G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS, kPrio, cancellable.get(), cb, ud);
    };
    (*attempt)(tryOriginal ? 1 : 2);
}

Operation::Operation(int id, Kind kind, QObject *parent)
    : QObject(parent)
    , m_id(id)
    , m_kind(kind)
{
    m_cancel = Gio::Cancellable::adopt(g_cancellable_new());
    m_clock.start();
}

Operation::~Operation()
{
    m_cancelled = true;
    g_cancellable_cancel(m_cancel.get());
    if (m_proc) {
        m_proc->disconnect(this);
        m_proc->kill();
        m_proc->waitForFinished(500);
    }
}

QString Operation::stateName() const
{
    switch (m_state) {
    case Scanning: return QStringLiteral("scanning");
    case Running: return QStringLiteral("running");
    case Paused: return QStringLiteral("paused");
    case Conflict: return QStringLiteral("conflict");
    case Done: return QStringLiteral("done");
    case Failed: return QStringLiteral("failed");
    case Cancelled: return QStringLiteral("cancelled");
    }
    return {};
}

void Operation::setState(State s)
{
    if (m_state == s)
        return;
    m_state = s;
    touch(true);
}

void Operation::touch(bool force)
{
    if (!force && m_lastEmit.isValid() && m_lastEmit.elapsed() < 100)
        return;
    m_lastEmit.restart();
    sample();
    emit changed();
}

double Operation::workDone() const
{
    if (m_useBytes)
        return double(m_bytesDone + m_curBytes);
    return m_itemsDone + (m_curTotal > 0 ? double(m_curBytes) / double(m_curTotal) : 0.0);
}

double Operation::workTotal() const { return m_useBytes ? double(m_bytesTotal) : double(m_itemsTotal); }

// Speed is an exponential moving average, so the estimate does not jump with every chunk.
void Operation::sample()
{
    const double w = workDone();
    if (!m_lastSample.isValid()) {
        m_lastSample.start();
        m_lastWork = w;
        return;
    }
    const qint64 ms = m_lastSample.elapsed();
    if (ms < 250)
        return;
    const double inst = (w - m_lastWork) / (double(ms) / 1000.0);
    m_rate = m_rate <= 0 ? inst : 0.7 * m_rate + 0.3 * inst;
    m_lastWork = w;
    m_lastSample.restart();
}

double Operation::progress() const
{
    if (m_state == Done)
        return 1.0;
    if (m_indeterminate || m_state == Scanning)
        return -1.0;
    const double t = workTotal();
    if (t <= 0)
        return 0.0;
    return std::clamp(workDone() / t, 0.0, 1.0);
}

QString Operation::etaText() const
{
    if (m_state != Running || m_rate <= 0 || m_indeterminate || m_clock.elapsed() < 1000)
        return {};
    const double left = workTotal() - workDone();
    if (left <= 0)
        return {};
    return Fmt::humanEta(qint64(std::ceil(left / m_rate)));
}

QString Operation::subject() const
{
    const size_t n = !pairs.empty() ? pairs.size() : origins.size();
    if (m_kind == Compress)
        return n == 1 ? tr("“%1”").arg(baseName(pairs[0].src.get())) : tr("%1 items").arg(n);
    if (n == 1 && !pairs.empty() && pairs[0].src)
        return tr("“%1”").arg(baseName(pairs[0].src.get()));
    return tr("%1 items").arg(n);
}

QString Operation::title() const
{
    const bool active = m_state < Done;
    const QString what = subject();
    const QString dest = m_destName;
    auto compose = [&](const QString &ing, const QString &ed, const QString &fail, const QString &joiner) {
        QString base = m_state == Done ? ed : m_state == Failed ? fail : ing;
        if (m_state == Cancelled)
            base = tr("Cancelled: %1").arg(ing);
        QString t = base + QLatin1Char(' ') + what;
        if (!joiner.isEmpty() && !dest.isEmpty())
            t += QLatin1Char(' ') + joiner + QLatin1Char(' ') + dest;
        return t;
    };
    Q_UNUSED(active)
    switch (m_kind) {
    case Copy: return compose(tr("Copying"), tr("Copied"), tr("Could not copy"), tr("to"));
    case Move: return compose(tr("Moving"), tr("Moved"), tr("Could not move"), tr("to"));
    case Link: return compose(tr("Linking"), tr("Linked"), tr("Could not link"), tr("in"));
    case Trash: return compose(tr("Moving"), tr("Moved"), tr("Could not trash"), QString()) + (m_state == Failed ? QString() : tr(" to Trash"));
    case Delete: return compose(tr("Deleting"), tr("Deleted"), tr("Could not delete"), QString());
    case Restore: return compose(tr("Restoring"), tr("Restored"), tr("Could not restore"), QString());
    case EmptyTrash:
        return m_state == Done ? tr("Emptied Trash") : m_state == Failed ? tr("Could not empty Trash") : tr("Emptying Trash");
    case Extract: return compose(tr("Extracting"), tr("Extracted"), tr("Could not extract"), tr("to"));
    case Compress: return compose(tr("Compressing"), tr("Compressed"), tr("Could not compress"), tr("to"));
    }
    return {};
}

QString Operation::detail() const
{
    switch (m_state) {
    case Scanning:
        return m_itemsTotal > 0 ? tr("Preparing · %1 items").arg(m_itemsTotal) : tr("Preparing…");
    case Conflict: return tr("Waiting for your choice");
    case Cancelled: return tr("Cancelled");
    case Failed: return error();
    case Done:
        if (m_kind == EmptyTrash || m_kind == Trash)
            return tr("%1 items").arg(m_itemsDone);
        return m_bytesTotal > 0 ? Fmt::humanSize(m_bytesTotal) : (m_itemsTotal ? Fmt::itemCount(m_itemsTotal) : QString());
    case Running:
    case Paused: break;
    }
    QString text;
    if (m_indeterminate) {
        text = tr("Working…");
    } else if (m_useBytes) {
        // Both sizes use the unit of the total: "1.4 of 3.2 GB".
        int k = 0;
        double t = double(m_bytesTotal);
        while (t >= 1000 && k < 4) {
            t /= 1000;
            ++k;
        }
        const double d = double(m_bytesDone + m_curBytes) / std::pow(1000.0, k);
        const QString num = QLocale().toString(d, 'f', t < 10 ? 1 : 0);
        text = tr("%1 of %2").arg(k == 0 ? QString::number(m_bytesDone + m_curBytes) : num, Fmt::humanSize(m_bytesTotal));
    } else {
        text = tr("%1 of %2 items").arg(m_itemsDone).arg(m_itemsTotal);
    }
    const QString eta = etaText();
    if (!eta.isEmpty())
        text += QStringLiteral(" · ") + eta;
    if (m_state == Paused)
        text = tr("Paused · ") + text;
    return text;
}

void Operation::addError(const QString &msg)
{
    m_errors << msg;
}

// Names the destination the way the user knows it: the drive name for a mount root, else the folder name.
void Operation::findDestName()
{
    if (!destFolder)
        return;
    m_destName = baseName(destFolder.get());
    const QString home = QString::fromUtf8(g_get_home_dir());
    if (Gio::pathOf(destFolder.get()) == home)
        m_destName = tr("Home");
    if (g_file_is_native(destFolder.get())) {
        // For a native file the volume monitor answers from memory, without disk or network access.
        GMount *mount = g_file_find_enclosing_mount(destFolder.get(), nullptr, nullptr);
        if (mount) {
            GFile *root = g_mount_get_root(mount);
            if (g_file_equal(root, destFolder.get()))
                m_destName = Gio::take(g_mount_get_name(mount));
            g_object_unref(root);
            g_object_unref(mount);
            return;
        }
    }
    QPointer<Operation> self(this);
    auto [cb, ud] = Gio::thunk([self](GObject *src, GAsyncResult *res) {
        Gio::Error err;
        auto info = Gio::Info::adopt(g_file_query_info_finish(G_FILE(src), res, err.out()));
        if (!self || !info)
            return;
        if (Gio::pathOf(G_FILE(src)) == QString::fromUtf8(g_get_home_dir()) || self->m_kind == Compress)
            return;
        self->m_destName = Gio::str(g_file_info_get_display_name(info.get()));
        self->touch(true);
    });
    g_file_query_info_async(destFolder.get(), "standard::display-name", G_FILE_QUERY_INFO_NONE, G_PRIORITY_LOW, m_cancel.get(), cb, ud);
}

void Operation::step(std::function<void()> fn)
{
    if (finished())
        return;
    if (m_cancelled) {
        finish(Cancelled);
        return;
    }
    if (m_paused) {
        m_held = std::move(fn);
        return;
    }
    fn();
}

void Operation::finish(State s)
{
    if (finished())
        return;
    if (s == Done && !m_errors.isEmpty())
        s = Failed;
    m_curBytes = 0;
    m_held = nullptr;
    m_resolveCb = nullptr;
    if (s == Cancelled) {
        g_cancellable_cancel(m_cancel.get());
        if (m_proc)
            m_proc->kill();
        if (!m_partial.isEmpty())
            QFile::remove(m_partial);
    }
    if (s == Done && m_useBytes)
        m_bytesDone = m_bytesTotal;
    m_state = s;
    touch(true);
    emit done(m_id);
}

void Operation::finishOk() { finish(Done); }

void Operation::cancel()
{
    if (finished())
        return;
    m_cancelled = true;
    finish(Cancelled);
}

void Operation::pause()
{
    if (!canPause() || (m_state != Running && m_state != Scanning))
        return;
    m_paused = true;
    setState(Paused);
}

void Operation::resume()
{
    if (!m_paused)
        return;
    m_paused = false;
    m_state = m_tasks.empty() && !m_useBytes && m_bytesTotal == 0 && m_itemsTotal == 0 ? Scanning : Running;
    touch(true);
    if (m_held) {
        auto f = std::move(m_held);
        m_held = nullptr;
        f();
    }
}

void Operation::resolve(Action a, bool all)
{
    if (m_state != Conflict || !m_resolveCb)
        return;
    if (all) {
        m_allSet = true;
        m_allAction = a;
    }
    auto cb = std::move(m_resolveCb);
    m_resolveCb = nullptr;
    m_state = Running;
    touch(true);
    if (a == CancelAll) {
        cancel();
        return;
    }
    cb(a);
}

void Operation::askConflict(const Gio::File &src, const Gio::File &dst, bool isDir, std::function<void(Action)> cont)
{
    if (m_allSet) {
        cont(m_allAction);
        return;
    }
    m_resolveCb = std::move(cont);
    setState(Conflict);
    emit conflictRequested(m_id, baseName(dst.get()), Gio::uriOf(src.get()), Gio::uriOf(dst.get()), isDir);
}

void Operation::start()
{
    findDestName();
    if (m_kind == Compress)
        m_destName = archiveName;
    m_state = Running;
    switch (m_kind) {
    case Copy: startCopy(); break;
    case Move: startMove(); break;
    case Link: startLink(); break;
    case Trash: startTrash(); break;
    case Delete: startDelete(); break;
    case Restore: startRestore(); break;
    case EmptyTrash: startEmptyTrash(); break;
    case Extract:
    case Compress: startProcess(); break;
    }
    touch(true);
}

// ---------------------------------------------------------------- tree engine

void Operation::startCopy()
{
    std::vector<int> tops;
    for (size_t i = 0; i < pairs.size(); ++i)
        tops.push_back(int(i));
    m_itemsTotal = int(pairs.size());
    scan(tops);
}

void Operation::startDelete()
{
    std::vector<int> tops;
    for (size_t i = 0; i < pairs.size(); ++i)
        tops.push_back(int(i));
    m_itemsTotal = int(pairs.size());
    scan(tops);
}

void Operation::scan(const std::vector<int> &tops)
{
    setState(Scanning);
    m_tasks.clear();
    m_dirQueue.clear();
    m_scanTops = tops;
    scanTop(0);
}

void Operation::scanTop(size_t k)
{
    step([this, k] {
        if (k >= m_scanTops.size()) {
            scanDirs();
            return;
        }
        const int top = m_scanTops[k];
        auto proceed = [this, k, top] {
            QPointer<Operation> self(this);
            auto [cb, ud] = Gio::thunk([self, k, top](GObject *src, GAsyncResult *res) {
                Gio::Error err;
                auto info = Gio::Info::adopt(g_file_query_info_finish(G_FILE(src), res, err.out()));
                if (!self || self->finished())
                    return;
                if (!info) {
                    if (!(self->m_kind == Delete && err.is(G_IO_ERROR_NOT_FOUND)))
                        self->addError(err.message());
                    self->m_keepTops.insert(top);
                    self->scanTop(k + 1);
                    return;
                }
                Task t;
                t.src = Gio::File::retain(G_FILE(src));
                t.name = Gio::take(g_file_get_basename(G_FILE(src))).toStdString();
                t.isDir = g_file_info_get_file_type(info.get()) == G_FILE_TYPE_DIRECTORY;
                t.size = t.isDir ? 0 : qint64(g_file_info_get_size(info.get()));
                t.top = top;
                self->m_tasks.push_back(std::move(t));
                if (self->m_tasks.back().isDir)
                    self->m_dirQueue.push_back(int(self->m_tasks.size()) - 1);
                self->m_itemsTotal = std::max(self->m_itemsTotal, int(self->m_tasks.size()));
                self->touch();
                self->scanTop(k + 1);
            });
            g_file_query_info_async(pairs[size_t(top)].src.get(), "standard::type,standard::size", G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS, kPrio, m_cancel.get(), cb, ud);
        };
        Pair &p = pairs[size_t(top)];
        if ((m_kind == Copy || m_kind == Move) && p.dst) {
            if (g_file_equal(p.src.get(), p.dst.get())) {
                if (m_kind == Move) {
                    m_keepTops.insert(top);
                    scanTop(k + 1);
                    return;
                }
                // Copying a file into its own folder makes "name (2)".
                QPointer<Operation> self(this);
                uniqueNameAsync(p.dst, false, m_cancel.get(), [self, top, proceed](Gio::File f) {
                    if (!self || self->finished())
                        return;
                    self->pairs[size_t(top)].dst = f;
                    proceed();
                });
                return;
            }
            if (g_file_has_prefix(p.dst.get(), p.src.get())) {
                addError(tr("“%1” cannot be copied into itself.").arg(baseName(p.src.get())));
                m_keepTops.insert(top);
                scanTop(k + 1);
                return;
            }
        }
        proceed();
    });
}

void Operation::scanDirs()
{
    step([this] {
        if (m_dirQueue.empty()) {
            scanDone();
            return;
        }
        const int idx = m_dirQueue.front();
        m_dirQueue.pop_front();
        QPointer<Operation> self(this);
        auto [cb, ud] = Gio::thunk([self, idx](GObject *src, GAsyncResult *res) {
            Gio::Error err;
            auto en = Gio::Obj<GFileEnumerator>::adopt(g_file_enumerate_children_finish(G_FILE(src), res, err.out()));
            if (!self || self->finished())
                return;
            if (!en) {
                self->addError(err.message());
                self->m_keepTops.insert(self->m_tasks[size_t(idx)].top);
                self->scanDirs();
                return;
            }
            // The batch loop keeps itself alive through this shared function.
            auto loop = std::make_shared<std::function<void()>>();
            *loop = [self, idx, en, loop] {
                auto [cb2, ud2] = Gio::thunk([self, idx, en, loop](GObject *s2, GAsyncResult *r2) {
                    Gio::Error e2;
                    GList *infos = g_file_enumerator_next_files_finish(G_FILE_ENUMERATOR(s2), r2, e2.out());
                    if (!self || self->finished()) {
                        g_list_free_full(infos, g_object_unref);
                        return;
                    }
                    if (!infos) {
                        if (e2)
                            self->addError(e2.message());
                        self->scanDirs();
                        return;
                    }
                    for (GList *l = infos; l; l = l->next) {
                        GFileInfo *fi = G_FILE_INFO(l->data);
                        const GFileType type = g_file_info_get_file_type(fi);
                        if (type == G_FILE_TYPE_SPECIAL)
                            continue;
                        Task t;
                        t.name = g_file_info_get_name(fi);
                        t.src = Gio::File::adopt(g_file_get_child(self->m_tasks[size_t(idx)].src.get(), t.name.c_str()));
                        t.isDir = type == G_FILE_TYPE_DIRECTORY;
                        t.size = t.isDir ? 0 : qint64(g_file_info_get_size(fi));
                        t.parent = idx;
                        t.top = self->m_tasks[size_t(idx)].top;
                        self->m_tasks.push_back(std::move(t));
                        if (self->m_tasks.back().isDir)
                            self->m_dirQueue.push_back(int(self->m_tasks.size()) - 1);
                    }
                    g_list_free_full(infos, g_object_unref);
                    self->m_itemsTotal = std::max(self->m_itemsTotal, int(self->m_tasks.size()));
                    self->touch();
                    (*loop)();
                });
                g_file_enumerator_next_files_async(en.get(), 200, kPrio, self->m_cancel.get(), cb2, ud2);
            };
            (*loop)();
        });
        g_file_enumerate_children_async(m_tasks[size_t(idx)].src.get(), "standard::name,standard::type,standard::size", G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS, kPrio,
                                        m_cancel.get(), cb, ud);
    });
}

void Operation::scanDone()
{
    m_bytesTotal = 0;
    for (const Task &t : m_tasks)
        if (!t.isDir)
            m_bytesTotal += t.size;
    m_itemsTotal = int(m_tasks.size());
    m_itemsDone = 0;
    m_bytesDone = 0;
    m_useBytes = (m_kind == Copy || m_kind == Move) && m_bytesTotal > 0;
    m_lastSample.invalidate();
    m_rate = 0;
    m_state = Running;
    touch(true);
    if (m_kind == Delete || m_kind == EmptyTrash)
        deleteTasks(int(m_tasks.size()) - 1, false);
    else
        runTasks(0);
}

Gio::File Operation::dstOf(int i)
{
    Task &t = m_tasks[size_t(i)];
    if (t.dst)
        return t.dst;
    if (t.parent < 0)
        t.dst = pairs[size_t(t.top)].dst;
    else
        t.dst = Gio::File::adopt(g_file_get_child(dstOf(t.parent).get(), t.name.c_str()));
    return t.dst;
}

void Operation::markSkipped(int i, bool keepSource)
{
    Task &t = m_tasks[size_t(i)];
    t.skipped = true;
    if (keepSource)
        m_keepTops.insert(t.top);
    if (!t.isDir)
        m_bytesDone += t.size;
    ++m_itemsDone;
    touch();
    runTasks(i + 1);
}

void Operation::runTasks(int i)
{
    step([this, i] {
        if (i >= int(m_tasks.size())) {
            copyDone();
            return;
        }
        Task &t = m_tasks[size_t(i)];
        if (t.parent >= 0 && m_tasks[size_t(t.parent)].skipped)
            t.skipped = true;
        if (t.skipped) {
            if (!t.isDir)
                m_bytesDone += t.size;
            ++m_itemsDone;
            runTasks(i + 1);
            return;
        }
        if (t.isDir)
            makeDir(i);
        else
            copyFile(i, false);
    });
}

void Operation::makeDir(int i)
{
    Gio::File dst = dstOf(i);
    QPointer<Operation> self(this);
    auto [cb, ud] = Gio::thunk([self, i](GObject *src, GAsyncResult *res) {
        Gio::Error err;
        const bool ok = g_file_make_directory_finish(G_FILE(src), res, err.out());
        if (!self || self->finished())
            return;
        Task &t = self->m_tasks[size_t(i)];
        if (ok) {
            ++self->m_itemsDone;
            if (t.parent < 0)
                self->m_ok.push_back({t.src, t.dst});
            self->touch();
            self->runTasks(i + 1);
            return;
        }
        if (err.is(G_IO_ERROR_EXISTS)) {
            // An existing folder is merged; an existing file of that name is a conflict.
            auto [cb2, ud2] = Gio::thunk([self, i](GObject *s2, GAsyncResult *r2) {
                Gio::Error e2;
                auto info = Gio::Info::adopt(g_file_query_info_finish(G_FILE(s2), r2, e2.out()));
                if (!self || self->finished())
                    return;
                if (info && g_file_info_get_file_type(info.get()) == G_FILE_TYPE_DIRECTORY) {
                    ++self->m_itemsDone;
                    self->runTasks(i + 1);
                    return;
                }
                Task &tt = self->m_tasks[size_t(i)];
                self->askConflict(tt.src, tt.dst, true, [self, i](Action a) {
                    if (!self || self->finished())
                        return;
                    if (a == Skip) {
                        self->markSkipped(i, true);
                        return;
                    }
                    uniqueNameAsync(self->m_tasks[size_t(i)].dst, false, self->m_cancel.get(), [self, i](Gio::File f) {
                        if (!self || self->finished())
                            return;
                        self->m_tasks[size_t(i)].dst = f;
                        self->makeDir(i);
                    });
                });
            });
            g_file_query_info_async(G_FILE(src), "standard::type", G_FILE_QUERY_INFO_NONE, kPrio, self->m_cancel.get(), cb2, ud2);
            return;
        }
        self->addError(err.message());
        self->markSkipped(i, true);
    });
    g_file_make_directory_async(dst.get(), kPrio, m_cancel.get(), cb, ud);
}

void Operation::copyFile(int i, bool overwrite)
{
    Gio::File src = m_tasks[size_t(i)].src;
    Gio::File dst = dstOf(i);
    QPointer<Operation> self(this);
    auto ctx = std::make_shared<ProgCtx>();
    ctx->op = this;
    ctx->fn = [self](qint64 cur, qint64 total) {
        if (!self)
            return;
        self->m_curBytes = cur;
        self->m_curTotal = total;
        self->touch();
    };
    auto [cb, ud] = Gio::thunk([self, i, ctx](GObject *s, GAsyncResult *res) {
        Gio::Error err;
        const bool ok = g_file_copy_finish(G_FILE(s), res, err.out());
        if (!self || self->finished())
            return;
        Task &t = self->m_tasks[size_t(i)];
        self->m_curBytes = 0;
        self->m_curTotal = 0;
        if (ok) {
            self->m_bytesDone += t.size;
            ++self->m_itemsDone;
            if (t.parent < 0)
                self->m_ok.push_back({t.src, t.dst});
            self->touch();
            self->runTasks(i + 1);
            return;
        }
        if (err.is(G_IO_ERROR_EXISTS) || err.is(G_IO_ERROR_IS_DIRECTORY) || err.is(G_IO_ERROR_WOULD_MERGE)) {
            const bool dirInTheWay = !err.is(G_IO_ERROR_EXISTS);
            self->askConflict(t.src, t.dst, dirInTheWay, [self, i, dirInTheWay](Action a) {
                if (!self || self->finished())
                    return;
                if (a == Skip) {
                    self->markSkipped(i, true);
                } else if (a == Replace && !dirInTheWay) {
                    self->copyFile(i, true);
                } else {
                    uniqueNameAsync(self->m_tasks[size_t(i)].dst, false, self->m_cancel.get(), [self, i](Gio::File f) {
                        if (!self || self->finished())
                            return;
                        self->m_tasks[size_t(i)].dst = f;
                        self->copyFile(i, false);
                    });
                }
            });
            return;
        }
        self->addError(err.message());
        self->markSkipped(i, true);
    });
    const GFileCopyFlags flags = GFileCopyFlags(G_FILE_COPY_NOFOLLOW_SYMLINKS | G_FILE_COPY_ALL_METADATA | (overwrite ? G_FILE_COPY_OVERWRITE : 0));
    g_file_copy_async(src.get(), dst.get(), flags, kPrio, m_cancel.get(), &progressCb, ctx.get(), cb, ud);
}

void Operation::copyDone()
{
    if (m_kind == Move || m_kind == Restore) {
        // Source is removed only after the whole copy finished and only for items without trouble.
        m_itemsDone = 0;
        deleteTasks(int(m_tasks.size()) - 1, true);
        return;
    }
    finishOk();
}

void Operation::deleteTasks(int i, bool keepFilter)
{
    step([this, i, keepFilter] {
        if (i < 0) {
            finishOk();
            return;
        }
        const Task &t = m_tasks[size_t(i)];
        if (keepFilter && m_keepTops.contains(t.top)) {
            deleteTasks(i - 1, keepFilter);
            return;
        }
        QPointer<Operation> self(this);
        auto [cb, ud] = Gio::thunk([self, i, keepFilter](GObject *src, GAsyncResult *res) {
            Gio::Error err;
            const bool ok = g_file_delete_finish(G_FILE(src), res, err.out());
            if (!self || self->finished())
                return;
            if (!ok && !err.is(G_IO_ERROR_NOT_FOUND) && !(keepFilter && err.is(G_IO_ERROR_NOT_EMPTY)))
                self->addError(err.message());
            if (!keepFilter) {
                ++self->m_itemsDone;
                self->touch();
            }
            self->deleteTasks(i - 1, keepFilter);
        });
        g_file_delete_async(t.src.get(), kPrio, m_cancel.get(), cb, ud);
    });
}

// ---------------------------------------------------------------- move

void Operation::startMove()
{
    m_itemsTotal = int(pairs.size());
    moveNext(0);
}

void Operation::moveNext(size_t k)
{
    step([this, k] {
        if (k >= pairs.size()) {
            if (m_fallback.empty()) {
                finishOk();
                return;
            }
            // Cross-device folders and merges go through copy and delete.
            std::vector<int> tops;
            for (size_t f : m_fallback)
                tops.push_back(int(f));
            m_fallback.clear();
            scan(tops);
            return;
        }
        moveOne(k, false);
    });
}

void Operation::moveOne(size_t k, bool overwrite)
{
    Pair &p = pairs[k];
    if (!p.dst || g_file_equal(p.src.get(), p.dst.get())) {
        ++m_itemsDone;
        moveNext(k + 1);
        return;
    }
    QPointer<Operation> self(this);
    auto ctx = std::make_shared<ProgCtx>();
    ctx->op = this;
    ctx->fn = [self](qint64 cur, qint64 total) {
        if (!self)
            return;
        self->m_curBytes = cur;
        self->m_curTotal = total;
        self->touch();
    };
    auto [cb, ud] = Gio::thunk([self, k, ctx](GObject *src, GAsyncResult *res) {
        Gio::Error err;
        const bool ok = g_file_move_finish(G_FILE(src), res, err.out());
        if (!self || self->finished())
            return;
        self->m_curBytes = 0;
        self->m_curTotal = 0;
        Pair &pp = self->pairs[k];
        if (ok) {
            ++self->m_itemsDone;
            if (self->m_kind == Move)
                self->m_ok.push_back(pp);
            if (k < self->m_infoFor.size() && !self->m_infoFor[k].isEmpty())
                QFile::remove(self->m_infoFor[k]);
            self->touch();
            self->moveNext(k + 1);
            return;
        }
        if (err.is(G_IO_ERROR_EXISTS)) {
            self->askConflict(pp.src, pp.dst, false, [self, k](Action a) {
                if (!self || self->finished())
                    return;
                if (a == Skip) {
                    self->m_keepTops.insert(int(k));
                    ++self->m_itemsDone;
                    self->moveNext(k + 1);
                } else if (a == Replace) {
                    self->moveOne(k, true);
                } else {
                    uniqueNameAsync(self->pairs[k].dst, false, self->m_cancel.get(), [self, k](Gio::File f) {
                        if (!self || self->finished())
                            return;
                        self->pairs[k].dst = f;
                        self->moveOne(k, false);
                    });
                }
            });
            return;
        }
        if (err.is(G_IO_ERROR_WOULD_MERGE) || err.is(G_IO_ERROR_WOULD_RECURSE) || err.is(G_IO_ERROR_NOT_SUPPORTED)) {
            self->m_fallback.push_back(k);
            self->moveNext(k + 1);
            return;
        }
        self->addError(err.message());
        self->moveNext(k + 1);
    });
    const GFileCopyFlags flags = GFileCopyFlags(G_FILE_COPY_NOFOLLOW_SYMLINKS | G_FILE_COPY_ALL_METADATA | (overwrite ? G_FILE_COPY_OVERWRITE : 0));
    g_file_move_async(p.src.get(), p.dst.get(), flags, kPrio, m_cancel.get(), &progressCb, ctx.get(), cb, ud);
}

// ---------------------------------------------------------------- simple operations

void Operation::startLink()
{
    m_itemsTotal = int(pairs.size());
    for (Pair &p : pairs) {
        const QString target = Gio::pathOf(p.src.get());
        if (target.isEmpty() || !destFolder || !g_file_is_native(destFolder.get())) {
            addError(tr("Links work on local files only."));
            continue;
        }
        const QString name = tr("Link to %1").arg(baseName(p.src.get()));
        auto dst = Gio::File::adopt(g_file_get_child(destFolder.get(), name.toUtf8().constData()));
        for (int n = 2; g_file_query_exists(dst.get(), nullptr); ++n) {
            auto [stem, ext] = splitName(name);
            dst = Gio::File::adopt(g_file_get_child(destFolder.get(), (stem + QStringLiteral(" (%1)").arg(n) + ext).toUtf8().constData()));
        }
        Gio::Error err;
        if (g_file_make_symbolic_link(dst.get(), target.toUtf8().constData(), nullptr, err.out())) {
            m_ok.push_back({p.src, dst});
            ++m_itemsDone;
        } else {
            addError(err.message());
        }
    }
    finishOk();
}

void Operation::startTrash()
{
    m_itemsTotal = int(pairs.size());
    trashNext(0);
}

void Operation::trashNext(size_t k)
{
    step([this, k] {
        if (k >= pairs.size()) {
            finishOk();
            return;
        }
        QPointer<Operation> self(this);
        auto [cb, ud] = Gio::thunk([self, k](GObject *src, GAsyncResult *res) {
            Gio::Error err;
            const bool ok = g_file_trash_finish(G_FILE(src), res, err.out());
            if (!self || self->finished())
                return;
            if (ok) {
                self->m_ok.push_back({self->pairs[k].src, {}});
                ++self->m_itemsDone;
            } else if (err.is(G_IO_ERROR_NOT_SUPPORTED)) {
                self->addError(tr("“%1” cannot go to the Trash here. Delete it instead.").arg(baseName(G_FILE(src))));
            } else {
                self->addError(err.message());
            }
            self->touch();
            self->trashNext(k + 1);
        });
        g_file_trash_async(pairs[k].src.get(), kPrio, m_cancel.get(), cb, ud);
    });
}

void Operation::startEmptyTrash()
{
    auto trash = Gio::fileForUri(QStringLiteral("trash:///"));
    QPointer<Operation> self(this);
    auto [cb, ud] = Gio::thunk([self](GObject *src, GAsyncResult *res) {
        Gio::Error err;
        auto en = Gio::Obj<GFileEnumerator>::adopt(g_file_enumerate_children_finish(G_FILE(src), res, err.out()));
        if (!self || self->finished())
            return;
        if (!en) {
            self->addError(err.message());
            self->finishOk();
            return;
        }
        auto loop = std::make_shared<std::function<void()>>();
        *loop = [self, en, loop] {
            auto [cb2, ud2] = Gio::thunk([self, en, loop](GObject *s2, GAsyncResult *r2) {
                Gio::Error e2;
                GList *infos = g_file_enumerator_next_files_finish(G_FILE_ENUMERATOR(s2), r2, e2.out());
                if (!self || self->finished()) {
                    g_list_free_full(infos, g_object_unref);
                    return;
                }
                if (!infos) {
                    if (self->pairs.empty()) {
                        self->finishOk();
                        return;
                    }
                    self->startDelete();
                    return;
                }
                auto trashRoot = Gio::fileForUri(QStringLiteral("trash:///"));
                for (GList *l = infos; l; l = l->next)
                    self->pairs.push_back({Gio::File::adopt(g_file_get_child(trashRoot.get(), g_file_info_get_name(G_FILE_INFO(l->data)))), {}});
                g_list_free_full(infos, g_object_unref);
                (*loop)();
            });
            g_file_enumerator_next_files_async(en.get(), 200, kPrio, self->m_cancel.get(), cb2, ud2);
        };
        (*loop)();
    });
    g_file_enumerate_children_async(trash.get(), "standard::name", G_FILE_QUERY_INFO_NONE, kPrio, m_cancel.get(), cb, ud);
}

// ---------------------------------------------------------------- restore

void Operation::startRestore()
{
    // pairs hold trashed files (src only); origins hold original places for undo. Both end up as move pairs.
    const bool byOrigin = !origins.empty();
    m_itemsTotal = int(byOrigin ? origins.size() : pairs.size());
    std::vector<Pair> input = std::move(pairs);
    pairs.clear();
    auto inputPtr = std::make_shared<std::vector<Pair>>(std::move(input));
    auto next = std::make_shared<std::function<void(size_t)>>();
    QPointer<Operation> self(this);
    const size_t n = byOrigin ? origins.size() : inputPtr->size();
    auto add = [self](Gio::File src, Gio::File dst, QString info) {
        if (!self || self->finished())
            return;
        // The folder that held the file may be gone; make it again.
        auto parent = Gio::File::adopt(g_file_get_parent(dst.get()));
        if (parent)
            g_file_make_directory_with_parents(parent.get(), nullptr, nullptr);
        self->pairs.push_back({src, dst});
        self->m_infoFor.push_back(info);
    };
    *next = [=](size_t k) {
        if (!self || self->finished())
            return;
        if (k >= n) {
            if (self->pairs.empty()) {
                self->finishOk();
                return;
            }
            self->m_itemsTotal = int(self->pairs.size());
            self->moveNext(0);
            return;
        }
        auto fail = [=](const QString &msg) {
            self->addError(msg);
            (*next)(k + 1);
        };
        if (byOrigin) {
            // Undo of trash: look the file up in the local trash index first, then ask gvfs.
            const QString orig = Gio::pathOf(origins[k].get());
            Gio::File origFile = origins[k];
            QtConcurrent::run([orig] {
                const QString data = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QStringLiteral("/Trash");
                const QDir infoDir(data + QStringLiteral("/info"));
                for (const QString &f : infoDir.entryList({QStringLiteral("*.trashinfo")}, QDir::Files)) {
                    QFile file(infoDir.filePath(f));
                    if (!file.open(QIODevice::ReadOnly))
                        continue;
                    for (const QByteArray &line : file.readAll().split('\n'))
                        if (line.startsWith("Path=") && QUrl::fromPercentEncoding(line.mid(5).trimmed()) == orig)
                            return QStringList{data + QStringLiteral("/files/") + f.chopped(10), infoDir.filePath(f)};
                }
                return QStringList();
            }).then(self.data(), [=](const QStringList &found) {
                if (!self || self->finished())
                    return;
                if (found.size() == 2) {
                    add(Gio::fileForPath(found[0]), origFile, found[1]);
                    (*next)(k + 1);
                    return;
                }
                auto trash = Gio::fileForUri(QStringLiteral("trash:///"));
                auto [cb, ud] = Gio::thunk([=](GObject *src, GAsyncResult *res) {
                    Gio::Error err;
                    auto en = Gio::Obj<GFileEnumerator>::adopt(g_file_enumerate_children_finish(G_FILE(src), res, err.out()));
                    if (!self || self->finished())
                        return;
                    if (!en) {
                        fail(tr("“%1” is not in the Trash.").arg(orig));
                        return;
                    }
                    auto loop = std::make_shared<std::function<void()>>();
                    *loop = [=] {
                        auto [cb2, ud2] = Gio::thunk([=](GObject *s2, GAsyncResult *r2) {
                            Gio::Error e2;
                            GList *infos = g_file_enumerator_next_files_finish(G_FILE_ENUMERATOR(s2), r2, e2.out());
                            if (!self || self->finished()) {
                                g_list_free_full(infos, g_object_unref);
                                return;
                            }
                            if (!infos) {
                                fail(tr("“%1” is not in the Trash.").arg(orig));
                                return;
                            }
                            for (GList *l = infos; l; l = l->next) {
                                GFileInfo *fi = G_FILE_INFO(l->data);
                                if (Gio::str(g_file_info_get_attribute_byte_string(fi, "trash::orig-path")) == orig) {
                                    add(Gio::File::adopt(g_file_get_child(trash.get(), g_file_info_get_name(fi))), origFile, QString());
                                    g_list_free_full(infos, g_object_unref);
                                    (*next)(k + 1);
                                    return;
                                }
                            }
                            g_list_free_full(infos, g_object_unref);
                            (*loop)();
                        });
                        g_file_enumerator_next_files_async(en.get(), 200, kPrio, self->m_cancel.get(), cb2, ud2);
                    };
                    (*loop)();
                });
                g_file_enumerate_children_async(trash.get(), "standard::name,trash::orig-path", G_FILE_QUERY_INFO_NONE, kPrio, self->m_cancel.get(), cb, ud);
            });
            return;
        }
        Gio::File trashed = (*inputPtr)[k].src;
        const QString uri = Gio::uriOf(trashed.get());
        if (uri.startsWith(QLatin1String("trash:"))) {
            auto [cb, ud] = Gio::thunk([=](GObject *src, GAsyncResult *res) {
                Gio::Error err;
                auto info = Gio::Info::adopt(g_file_query_info_finish(G_FILE(src), res, err.out()));
                if (!self || self->finished())
                    return;
                const QString orig = info ? Gio::str(g_file_info_get_attribute_byte_string(info.get(), "trash::orig-path")) : QString();
                if (orig.isEmpty()) {
                    fail(tr("The original place of “%1” is unknown.").arg(baseName(trashed.get())));
                    return;
                }
                add(trashed, Gio::fileForPath(orig), QString());
                (*next)(k + 1);
            });
            g_file_query_info_async(trashed.get(), "trash::orig-path", G_FILE_QUERY_INFO_NONE, kPrio, self->m_cancel.get(), cb, ud);
            return;
        }
        // A file inside a Trash/files folder: its .trashinfo names the original place.
        const QString path = Gio::pathOf(trashed.get());
        const QString filesDir = QFileInfo(path).absolutePath();
        const QString infoPath = QFileInfo(filesDir).absolutePath() + QStringLiteral("/info/") + QFileInfo(path).fileName() + QStringLiteral(".trashinfo");
        if (!filesDir.endsWith(QLatin1String("/files")) || path.isEmpty()) {
            fail(tr("“%1” is not in the Trash.").arg(baseName(trashed.get())));
            return;
        }
        auto infoFile = Gio::fileForPath(infoPath);
        auto [cb, ud] = Gio::thunk([=](GObject *src, GAsyncResult *res) {
            Gio::Error err;
            char *contents = nullptr;
            gsize len = 0;
            const bool ok = g_file_load_contents_finish(G_FILE(src), res, &contents, &len, nullptr, err.out());
            const QByteArray text = ok ? QByteArray(contents, int(len)) : QByteArray();
            g_free(contents);
            if (!self || self->finished())
                return;
            QString orig;
            for (const QByteArray &line : text.split('\n'))
                if (line.startsWith("Path="))
                    orig = QUrl::fromPercentEncoding(line.mid(5).trimmed());
            if (orig.isEmpty()) {
                fail(tr("The original place of “%1” is unknown.").arg(baseName(trashed.get())));
                return;
            }
            // A relative Path belongs to a trash folder on another drive and counts from that drive's top folder.
            if (!orig.startsWith(QLatin1Char('/'))) {
                const QString trashRoot = QFileInfo(filesDir).absolutePath();
                orig = QFileInfo(trashRoot).absolutePath() + QLatin1Char('/') + orig;
            }
            add(trashed, Gio::fileForPath(orig), infoPath);
            (*next)(k + 1);
        });
        g_file_load_contents_async(infoFile.get(), self->m_cancel.get(), cb, ud);
    };
    (*next)(0);
}

// ---------------------------------------------------------------- archives

void Operation::startProcess()
{
    const QString tool = QStandardPaths::findExecutable(QStringLiteral("bsdtar"));
    if (tool.isEmpty()) {
        addError(tr("bsdtar is not installed."));
        finishOk();
        return;
    }
    if (m_kind == Extract) {
        m_itemsTotal = int(pairs.size());
        procNext(0);
        return;
    }
    // Compress: one process for all sources.
    m_indeterminate = true;
    const QString destDir = Gio::pathOf(destFolder.get());
    if (destDir.isEmpty()) {
        addError(tr("Archives can be made in local folders only."));
        finishOk();
        return;
    }
    QStringList args = {QStringLiteral("-a"), QStringLiteral("-cf")};
    QString out = destDir + QLatin1Char('/') + archiveName;
    const auto [stem, ext] = splitName(archiveName);
    for (int n = 2; QFileInfo::exists(out); ++n)
        out = destDir + QLatin1Char('/') + stem + QStringLiteral(" (%1)").arg(n) + ext;
    m_partial = out;
    args << out;
    QString lastDir;
    for (const Pair &p : pairs) {
        const QString path = Gio::pathOf(p.src.get());
        if (path.isEmpty()) {
            addError(tr("Only local files can go into an archive."));
            continue;
        }
        const QFileInfo fi(path);
        if (fi.absolutePath() != lastDir) {
            args << QStringLiteral("-C") << fi.absolutePath();
            lastDir = fi.absolutePath();
        }
        args << fi.fileName();
    }
    m_proc = new QProcess(this);
    connect(m_proc, &QProcess::finished, this, [this, out](int code, QProcess::ExitStatus st) {
        if (finished())
            return;
        const QString errText = QString::fromUtf8(m_proc->readAllStandardError()).trimmed();
        if (st != QProcess::NormalExit || code != 0) {
            addError(errText.isEmpty() ? tr("The archive tool failed.") : errText);
            QFile::remove(out);
        } else {
            m_ok.push_back({{}, Gio::fileForPath(out)});
        }
        m_partial.clear();
        finishOk();
    });
    connect(m_proc, &QProcess::errorOccurred, this, [this](QProcess::ProcessError e) {
        if (e == QProcess::FailedToStart && !finished()) {
            addError(tr("The archive tool did not start."));
            finishOk();
        }
    });
    m_proc->start(tool, args);
}

void Operation::procNext(size_t k)
{
    step([this, k] {
        if (k >= pairs.size()) {
            finishOk();
            return;
        }
        const QString tool = QStandardPaths::findExecutable(QStringLiteral("bsdtar"));
        const QString archive = Gio::pathOf(pairs[k].src.get());
        QString parentDir = destFolder ? Gio::pathOf(destFolder.get()) : QFileInfo(archive).absolutePath();
        if (archive.isEmpty() || parentDir.isEmpty()) {
            addError(tr("Archives can be opened from local folders only."));
            procNext(k + 1);
            return;
        }
        QString target = parentDir;
        if (intoFolder) {
            const QString stem = archiveStem(QFileInfo(archive).fileName());
            target = parentDir + QLatin1Char('/') + stem;
            for (int n = 2; QFileInfo::exists(target); ++n)
                target = parentDir + QLatin1Char('/') + stem + QStringLiteral(" (%1)").arg(n);
            QDir().mkpath(target);
        }
        m_proc = new QProcess(this);
        // bsdtar drops leading slashes and refuses ".." paths unless told otherwise, so an archive cannot leave the target.
        connect(m_proc, &QProcess::finished, this, [this, k, target](int code, QProcess::ExitStatus st) {
            if (finished())
                return;
            if (st != QProcess::NormalExit || code != 0) {
                const QString errText = QString::fromUtf8(m_proc->readAllStandardError()).trimmed();
                addError(errText.isEmpty() ? tr("The archive tool failed.") : errText);
            } else if (intoFolder) {
                m_ok.push_back({pairs[k].src, Gio::fileForPath(target)});
            }
            m_proc->deleteLater();
            m_proc = nullptr;
            ++m_itemsDone;
            touch(true);
            procNext(k + 1);
        });
        connect(m_proc, &QProcess::errorOccurred, this, [this, k](QProcess::ProcessError e) {
            if (e == QProcess::FailedToStart && !finished()) {
                addError(tr("The archive tool did not start."));
                m_proc->deleteLater();
                m_proc = nullptr;
                procNext(k + 1);
            }
        });
        m_proc->start(tool, {QStringLiteral("-xf"), archive, QStringLiteral("-C"), target});
    });
}
