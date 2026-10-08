#include "search.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QMimeDatabase>
#include <QStandardPaths>
#include <QDir>

namespace {
constexpr int kParallel = 4;
constexpr int kBatch = 200;
constexpr int kContentCap = 1000;
}

struct Search::Dir
{
    Gio::File file;
    Gio::Obj<GFileEnumerator> en;
};

Search::Search(QObject *parent)
    : EntryModel(parent)
{
    m_flushTimer.setSingleShot(true);
    m_flushTimer.setInterval(100);
    connect(&m_flushTimer, &QTimer::timeout, this, &Search::flush);
}

Search::~Search()
{
    stop();
}

bool Search::contentSupported()
{
    static const bool ok = !QStandardPaths::findExecutable(QStringLiteral("rg")).isEmpty();
    return ok;
}

void Search::setRoot(const QUrl &u)
{
    if (u == m_root)
        return;
    m_root = u;
    emit rootChanged();
}

void Search::setQuery(const QString &q)
{
    if (q == m_query)
        return;
    m_query = q;
    emit queryChanged();
}

void Search::setMode(const QString &m)
{
    const bool c = m == QLatin1String("content");
    if (c == m_content)
        return;
    m_content = c;
    emit modeChanged();
}

void Search::setRunning(bool on)
{
    if (m_running == on)
        return;
    m_running = on;
    emit runningChanged();
}

bool Search::matches(const QString &name) const
{
    if (m_glob.isValid() && !m_glob.pattern().isEmpty())
        return m_glob.match(name).hasMatch();
    return name.contains(m_query, Qt::CaseInsensitive);
}

void Search::stop()
{
    ++m_gen;
    if (m_cancel)
        g_cancellable_cancel(m_cancel.get());
    m_cancel.reset();
    m_queue.clear();
    m_active = 0;
    m_flushTimer.stop();
    if (m_proc) {
        m_proc->disconnect(this);
        m_proc->kill();
        m_proc->deleteLater();
        m_proc = nullptr;
    }
}

void Search::cancel()
{
    if (!m_running)
        return;
    stop();
    flush();
    setRunning(false);
    emit finished();
}

void Search::clear()
{
    stop();
    beginResetModel();
    m_view.clear();
    forgetAllEntries();
    m_store.clear();
    m_pending.clear();
    m_anchor = -1;
    endResetModel();
    recomputeStats();
    m_truncated = false;
    m_scanned = 0;
    m_contentFiles = 0;
    m_buf.clear();
    m_error.clear();
    setRunning(false);
}

void Search::start()
{
    clear();
    const QString q = m_query.trimmed();
    if (q.isEmpty() || m_root.isEmpty())
        return;
    m_glob = (q.contains(QLatin1Char('*')) || q.contains(QLatin1Char('?')))
        ? QRegularExpression(QRegularExpression::wildcardToRegularExpression(q), QRegularExpression::CaseInsensitiveOption)
        : QRegularExpression();
    auto root = Gio::fileForUrl(m_root);
    if (!root) {
        m_error = tr("Invalid location");
        emit runningChanged();
        return;
    }
    m_cancel = Gio::Cancellable::adopt(g_cancellable_new());
    setRunning(true);
    emit started();
    if (m_content) {
        startContent();
        return;
    }
    m_queue.push_back(root);
    pump();
}

void Search::pump()
{
    while (m_active < kParallel && !m_queue.empty()) {
        Gio::File dir = m_queue.front();
        m_queue.pop_front();
        ++m_active;
        readDir(dir);
    }
    finishIfIdle();
}

void Search::finishIfIdle()
{
    if (m_active > 0 || !m_queue.empty() || m_proc || !m_running)
        return;
    flush();
    setRunning(false);
    emit finished();
}

void Search::readDir(const Gio::File &dir)
{
    const quint64 gen = m_gen;
    QPointer<Search> self(this);
    auto [cb, ud] = Gio::thunk([self, gen, dir](GObject *src, GAsyncResult *res) {
        Gio::Error err;
        auto en = Gio::Obj<GFileEnumerator>::adopt(g_file_enumerate_children_finish(G_FILE(src), res, err.out()));
        if (!self || self->m_gen != gen)
            return;
        if (!en) {
            // An unreadable folder is skipped; the rest of the tree still counts.
            --self->m_active;
            self->pump();
            return;
        }
        auto d = std::make_shared<Dir>();
        d->file = dir;
        d->en = en;
        ++self->m_scanned;
        self->nextFiles(d);
    });
    g_file_enumerate_children_async(dir.get(), Gio::kEntryAttributes, G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS, G_PRIORITY_LOW, m_cancel.get(), cb, ud);
}

void Search::nextFiles(const std::shared_ptr<Dir> &d)
{
    const quint64 gen = m_gen;
    QPointer<Search> self(this);
    auto [cb, ud] = Gio::thunk([self, gen, d](GObject *src, GAsyncResult *res) {
        Gio::Error err;
        GList *infos = g_file_enumerator_next_files_finish(G_FILE_ENUMERATOR(src), res, err.out());
        if (!self || self->m_gen != gen) {
            g_list_free_full(infos, g_object_unref);
            return;
        }
        if (!infos) {
            --self->m_active;
            self->pump();
            return;
        }
        for (GList *l = infos; l; l = l->next) {
            GFileInfo *info = G_FILE_INFO(l->data);
            if (!self->m_showHidden && g_file_info_get_is_hidden(info))
                continue;
            const QString name = Gio::str(g_file_info_get_display_name(info));
            const bool isDir = g_file_info_get_file_type(info) == G_FILE_TYPE_DIRECTORY;
            if (self->m_truncated)
                break;
            if (self->matches(name) || self->matches(Gio::str(g_file_info_get_name(info)))) {
                if (int(self->m_store.size()) >= self->m_max) {
                    self->m_truncated = true;
                    break;
                }
                self->addResult(Gio::makeEntry(info, d->file.get()));
            }
            if (isDir)
                self->m_queue.push_back(Gio::File::adopt(g_file_get_child(d->file.get(), g_file_info_get_name(info))));
        }
        g_list_free_full(infos, g_object_unref);
        if (self->m_truncated) {
            // The cap ends the walk at once.
            self->m_queue.clear();
            --self->m_active;
            self->pump();
            return;
        }
        self->pump();
        self->nextFiles(d);
    });
    g_file_enumerator_next_files_async(d->en.get(), kBatch, G_PRIORITY_LOW, m_cancel.get(), cb, ud);
}

void Search::addResult(std::shared_ptr<Entry> e)
{
    m_store.push_back(e);
    m_pending.push_back(std::move(e));
    if (!m_flushTimer.isActive())
        m_flushTimer.start();
}

void Search::flush()
{
    m_flushTimer.stop();
    if (m_pending.empty())
        return;
    const int first = int(m_view.size());
    beginInsertRows({}, first, first + int(m_pending.size()) - 1);
    for (auto &sp : m_pending) {
        sp->inView = true;
        m_view.push_back(sp.get());
    }
    endInsertRows();
    m_pending.clear();
    recomputeStats();
    emit runningChanged();
}

void Search::startContent()
{
    const QString rg = QStandardPaths::findExecutable(QStringLiteral("rg"));
    auto root = Gio::fileForUrl(m_root);
    const QString path = Gio::pathOf(root.get());
    if (rg.isEmpty() || path.isEmpty()) {
        m_error = rg.isEmpty() ? tr("Content search needs ripgrep (rg).") : tr("Content search works on local folders only.");
        setRunning(false);
        emit finished();
        return;
    }
    m_proc = new QProcess(this);
    m_proc->setProcessChannelMode(QProcess::SeparateChannels);
    QStringList args = {QStringLiteral("--no-config"), QStringLiteral("--json"), QStringLiteral("--line-number"), QStringLiteral("--smart-case"),
                        QStringLiteral("--fixed-strings"), QStringLiteral("--max-count"), QStringLiteral("3"), QStringLiteral("--max-filesize"),
                        QStringLiteral("5M"), QStringLiteral("--max-columns"), QStringLiteral("300")};
    if (m_showHidden)
        args << QStringLiteral("--hidden");
    args << QStringLiteral("--") << m_query.trimmed() << path;
    const quint64 gen = m_gen;
    connect(m_proc, &QProcess::readyReadStandardOutput, this, [this, gen] {
        if (gen == m_gen)
            parseRg();
    });
    connect(m_proc, &QProcess::finished, this, [this, gen] {
        if (gen != m_gen)
            return;
        parseRg();
        m_proc->deleteLater();
        m_proc = nullptr;
        finishIfIdle();
    });
    m_proc->start(rg, args);
}

void Search::parseRg()
{
    if (!m_proc)
        return;
    m_buf += m_proc->readAllStandardOutput();
    int nl;
    while ((nl = int(m_buf.indexOf('\n'))) >= 0) {
        const QByteArray line = m_buf.left(nl);
        m_buf.remove(0, nl + 1);
        const QJsonObject o = QJsonDocument::fromJson(line).object();
        if (o.value(QLatin1String("type")).toString() != QLatin1String("match"))
            continue;
        const QJsonObject d = o.value(QLatin1String("data")).toObject();
        const QString file = d.value(QLatin1String("path")).toObject().value(QLatin1String("text")).toString();
        if (file.isEmpty())
            continue;
        if (int(m_store.size()) >= m_max || m_contentFiles >= kContentCap) {
            m_truncated = true;
            m_proc->kill();
            return;
        }
        auto e = std::make_shared<Entry>();
        e->name = QFileInfo(file).fileName();
        e->uri = QUrl::fromLocalFile(file).toString(QUrl::FullyEncoded);
        e->contentType = QMimeDatabase().mimeTypeForFile(file, QMimeDatabase::MatchExtension).name();
        e->kind = Fmt::kindFor(e->contentType, false);
        e->snippet = d.value(QLatin1String("lines")).toObject().value(QLatin1String("text")).toString().trimmed();
        e->line = d.value(QLatin1String("line_number")).toInt();
        ++m_contentFiles;
        addResult(e);
        fillInfo(e);
    }
}

// Content hits arrive without size and date, so a low priority async query adds them later.
void Search::fillInfo(const std::shared_ptr<Entry> &e)
{
    auto f = Gio::fileForUri(e->uri);
    const quint64 gen = m_gen;
    QPointer<Search> self(this);
    std::weak_ptr<Entry> weak = e;
    auto [cb, ud] = Gio::thunk([self, gen, weak](GObject *src, GAsyncResult *res) {
        Gio::Error err;
        auto info = Gio::Info::adopt(g_file_query_info_finish(G_FILE(src), res, err.out()));
        auto ent = weak.lock();
        if (!self || self->m_gen != gen || !info || !ent)
            return;
        ent->size = qint64(g_file_info_get_size(info.get()));
        ent->modifiedMs = qint64(g_file_info_get_attribute_uint64(info.get(), "time::modified")) * 1000;
        self->notifyRow(ent.get(), {SizeRole, SizeTextRole, ModifiedRole, ModifiedTextRole});
    });
    g_file_query_info_async(f.get(), "standard::size,time::modified", G_FILE_QUERY_INFO_NONE, G_PRIORITY_LOW, m_cancel.get(), cb, ud);
}
