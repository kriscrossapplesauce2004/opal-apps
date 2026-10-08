#include "fileoperations.h"

#include <QCoreApplication>
#include <QPointer>
#include <QStandardPaths>
#include <QQmlEngine>
#include <algorithm>

namespace {
constexpr int kUndoDepth = 20;

QString kindName(Operation::Kind k)
{
    static const char *const names[] = {"copy", "move", "link", "trash", "delete", "restore", "emptyTrash", "extract", "compress"};
    return QString::fromLatin1(names[int(k)]);
}
}

// ---------------------------------------------------------------- OperationsModel

QHash<int, QByteArray> OperationsModel::roleNames() const
{
    return {
        {IdRole, "id"}, {KindRole, "kind"}, {TitleRole, "title"}, {DetailRole, "detail"}, {StateRole, "state"},
        {ProgressRole, "progress"}, {EtaRole, "eta"}, {ErrorRole, "error"}, {DestinationRole, "destination"},
        {ActiveRole, "active"}, {CanPauseRole, "canPause"}, {PausedRole, "paused"}, {BytesDoneRole, "bytesDone"},
        {BytesTotalRole, "bytesTotal"}, {ItemsDoneRole, "itemsDone"}, {ItemsTotalRole, "itemsTotal"},
    };
}

QVariant OperationsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= int(m_ops.size()))
        return {};
    const Operation *o = m_ops[size_t(index.row())];
    switch (role) {
    case IdRole: return o->id();
    case KindRole: return kindName(o->kind());
    case TitleRole:
    case Qt::DisplayRole: return o->title();
    case DetailRole: return o->detail();
    case StateRole: return o->stateName();
    case ProgressRole: return o->progress();
    case EtaRole: return o->etaText();
    case ErrorRole: return o->error();
    case DestinationRole: return o->destination();
    case ActiveRole: return !o->finished();
    case CanPauseRole: return o->canPause();
    case PausedRole: return o->paused();
    case BytesDoneRole: return o->bytesDone();
    case BytesTotalRole: return o->bytesTotal();
    case ItemsDoneRole: return o->itemsDone();
    case ItemsTotalRole: return o->itemsTotal();
    }
    return {};
}

int OperationsModel::activeCount() const
{
    return int(std::count_if(m_ops.begin(), m_ops.end(), [](const Operation *o) { return !o->finished(); }));
}

double OperationsModel::overallProgress() const
{
    double sum = 0;
    int n = 0;
    for (const Operation *o : m_ops)
        if (!o->finished()) {
            const double p = o->progress();
            if (p < 0)
                return -1;
            sum += p;
            ++n;
        }
    return n ? sum / n : -1;
}

int OperationsModel::indexOfId(int id) const
{
    for (size_t i = 0; i < m_ops.size(); ++i)
        if (m_ops[i]->id() == id)
            return int(i);
    return -1;
}

Operation *OperationsModel::byId(int id) const
{
    const int i = indexOfId(id);
    return i < 0 ? nullptr : m_ops[size_t(i)];
}

void OperationsModel::add(Operation *op)
{
    // Newest first, like a download list.
    beginInsertRows({}, 0, 0);
    m_ops.insert(m_ops.begin(), op);
    op->setParent(this);
    endInsertRows();
    connect(op, &Operation::changed, this, [this, op] {
        const auto it = std::find(m_ops.begin(), m_ops.end(), op);
        if (it != m_ops.end())
            emit dataChanged(index(int(it - m_ops.begin())), index(int(it - m_ops.begin())));
        emit countChanged();
    });
    emit countChanged();
}

void OperationsModel::dismiss(int id)
{
    const int i = indexOfId(id);
    if (i < 0 || !m_ops[size_t(i)]->finished())
        return;
    beginRemoveRows({}, i, i);
    Operation *o = m_ops[size_t(i)];
    m_ops.erase(m_ops.begin() + i);
    endRemoveRows();
    o->deleteLater();
    emit countChanged();
}

void OperationsModel::clearFinished()
{
    for (int i = int(m_ops.size()) - 1; i >= 0; --i)
        if (m_ops[size_t(i)]->finished())
            dismiss(m_ops[size_t(i)]->id());
}

// ---------------------------------------------------------------- FileOperations

FileOperations::FileOperations(QObject *parent)
    : QObject(parent)
{
}

FileOperations *FileOperations::instance()
{
    static FileOperations *f = new FileOperations(QCoreApplication::instance());
    return f;
}

FileOperations *FileOperations::create(QQmlEngine *, QJSEngine *)
{
    auto *f = instance();
    QJSEngine::setObjectOwnership(f, QJSEngine::CppOwnership);
    return f;
}

bool FileOperations::archiveSupported()
{
    static const bool ok = !QStandardPaths::findExecutable(QStringLiteral("bsdtar")).isEmpty();
    return ok;
}

void FileOperations::pushUndo(const QString &text, std::function<void()> run)
{
    m_undo.push_back({text, std::move(run)});
    while (int(m_undo.size()) > kUndoDepth)
        m_undo.pop_front();
    emit undoChanged();
}

int FileOperations::launch(Operation *op)
{
    m_model.add(op);
    connect(op, &Operation::conflictRequested, this, &FileOperations::conflictRequested);
    connect(op, &Operation::done, this, &FileOperations::onDone);
    const int id = op->id();
    // Starting after the row exists lets a model view show the first state.
    op->start();
    return id;
}

Operation *FileOperations::make(Operation::Kind kind, const QStringList &sources, const QString &dest, bool pairWithDest)
{
    auto *op = new Operation(m_nextId++, kind);
    if (!dest.isEmpty())
        op->destFolder = Gio::fileForString(dest);
    for (const QString &s : sources) {
        auto src = Gio::fileForString(s);
        if (!src)
            continue;
        Operation::Pair p{src, {}};
        if (pairWithDest && op->destFolder) {
            const QByteArray base = Gio::take(g_file_get_basename(src.get())).toUtf8();
            p.dst = Gio::File::adopt(g_file_get_child(op->destFolder.get(), base.constData()));
        }
        op->pairs.push_back(std::move(p));
    }
    return op;
}

int FileOperations::copy(const QStringList &sources, const QString &destFolder)
{
    if (sources.isEmpty() || destFolder.isEmpty())
        return -1;
    return launch(make(Operation::Copy, sources, destFolder, true));
}

int FileOperations::move(const QStringList &sources, const QString &destFolder)
{
    if (sources.isEmpty() || destFolder.isEmpty())
        return -1;
    return launch(make(Operation::Move, sources, destFolder, true));
}

int FileOperations::link(const QStringList &sources, const QString &destFolder)
{
    if (sources.isEmpty() || destFolder.isEmpty())
        return -1;
    return launch(make(Operation::Link, sources, destFolder, false));
}

int FileOperations::trash(const QStringList &sources)
{
    if (sources.isEmpty())
        return -1;
    return launch(make(Operation::Trash, sources, QString(), false));
}

int FileOperations::remove(const QStringList &sources)
{
    if (sources.isEmpty())
        return -1;
    return launch(make(Operation::Delete, sources, QString(), false));
}

int FileOperations::restore(const QStringList &trashedUris)
{
    if (trashedUris.isEmpty())
        return -1;
    return launch(make(Operation::Restore, trashedUris, QString(), false));
}

int FileOperations::emptyTrash()
{
    return launch(new Operation(m_nextId++, Operation::EmptyTrash));
}

int FileOperations::extract(const QStringList &archives, const QString &destFolder, bool intoFolder)
{
    if (archives.isEmpty() || !archiveSupported())
        return -1;
    auto *op = make(Operation::Extract, archives, destFolder, false);
    op->intoFolder = intoFolder;
    return launch(op);
}

int FileOperations::compress(const QStringList &sources, const QString &destFolder, const QString &archiveName)
{
    if (sources.isEmpty() || destFolder.isEmpty() || archiveName.isEmpty() || !archiveSupported())
        return -1;
    auto *op = make(Operation::Compress, sources, destFolder, false);
    op->archiveName = archiveName;
    return launch(op);
}

void FileOperations::cancel(int id)
{
    if (auto *o = m_model.byId(id))
        o->cancel();
}

void FileOperations::pause(int id)
{
    if (auto *o = m_model.byId(id))
        o->pause();
}

void FileOperations::resume(int id)
{
    if (auto *o = m_model.byId(id))
        o->resume();
}

void FileOperations::resolveConflict(int id, int action, bool applyToAll)
{
    if (auto *o = m_model.byId(id))
        o->resolve(Operation::Action(action), applyToAll);
}

// Builds the undo step from what the operation really did.
void FileOperations::onDone(int id)
{
    Operation *op = m_model.byId(id);
    if (!op)
        return;
    emit finished(id, op->stateName(), op->error());
    if (!op->record || op->succeeded().empty())
        return;
    const std::vector<Operation::Pair> ok = op->succeeded();
    QPointer<FileOperations> self(this);
    switch (op->kind()) {
    case Operation::Trash: {
        std::vector<Gio::File> origins;
        for (const auto &p : ok)
            origins.push_back(p.src);
        pushUndo(tr("Undo move to Trash"), [self, origins] {
            if (!self)
                return;
            auto *r = new Operation(self->m_nextId++, Operation::Restore);
            r->origins = origins;
            r->record = false;
            self->launch(r);
        });
        break;
    }
    case Operation::Move: {
        pushUndo(tr("Undo move"), [self, ok] {
            if (!self)
                return;
            auto *r = new Operation(self->m_nextId++, Operation::Move);
            for (const auto &p : ok)
                r->pairs.push_back({p.dst, p.src});
            r->record = false;
            self->launch(r);
        });
        break;
    }
    case Operation::Copy:
    case Operation::Link:
    case Operation::Extract:
    case Operation::Compress: {
        // Undo of a copy-like operation puts the new files in the Trash.
        pushUndo(op->kind() == Operation::Copy ? tr("Undo copy") : tr("Undo %1").arg(kindName(op->kind())), [self, ok] {
            if (!self)
                return;
            auto *r = new Operation(self->m_nextId++, Operation::Trash);
            for (const auto &p : ok)
                r->pairs.push_back({p.dst, {}});
            r->record = false;
            self->launch(r);
        });
        break;
    }
    default: break;
    }
}

void FileOperations::undo()
{
    if (m_undo.empty())
        return;
    auto step = std::move(m_undo.back());
    m_undo.pop_back();
    emit undoChanged();
    step.run();
}

void FileOperations::setDisplayName(const Gio::File &file, const QString &name, bool recordUndo)
{
    QPointer<FileOperations> self(this);
    const QString oldUri = Gio::uriOf(file.get());
    const QString oldName = Gio::take(g_file_get_basename(file.get()));
    auto [cb, ud] = Gio::thunk([self, oldUri, oldName, recordUndo](GObject *src, GAsyncResult *res) {
        Gio::Error err;
        auto renamed = Gio::File::adopt(g_file_set_display_name_finish(G_FILE(src), res, err.out()));
        if (!self)
            return;
        if (!renamed) {
            emit self->failed(tr("Could not rename “%1”").arg(oldName), err.message());
            return;
        }
        const QString newUri = Gio::uriOf(renamed.get());
        emit self->renamed(oldUri, newUri);
        if (recordUndo)
            self->pushUndo(tr("Undo rename"), [self, renamed, oldName] {
                if (self)
                    self->setDisplayName(renamed, oldName, false);
            });
    });
    g_file_set_display_name_async(file.get(), name.toUtf8().constData(), G_PRIORITY_DEFAULT, nullptr, cb, ud);
}

void FileOperations::rename(const QString &uri, const QString &newName)
{
    auto f = Gio::fileForString(uri);
    if (!f || newName.trimmed().isEmpty() || newName.contains(QLatin1Char('/')))
        return;
    setDisplayName(f, newName.trimmed(), true);
}

void FileOperations::create(const QString &parent, const QString &nameIn, bool folder)
{
    auto dir = Gio::fileForString(parent);
    if (!dir)
        return;
    const QString name = nameIn.trimmed().isEmpty() ? (folder ? tr("New Folder") : tr("New File")) : nameIn.trimmed();
    if (name.contains(QLatin1Char('/'))) {
        emit failed(tr("Could not create “%1”").arg(name), tr("A name cannot hold a slash."));
        return;
    }
    auto target = Gio::File::adopt(g_file_get_child(dir.get(), name.toUtf8().constData()));
    QPointer<FileOperations> self(this);
    uniqueNameAsync(target, true, nullptr, [self, folder, name](Gio::File free) {
        if (!self)
            return;
        auto done = [self, free, name](bool ok, const QString &msg) {
            if (!self)
                return;
            if (!ok) {
                emit self->failed(tr("Could not create “%1”").arg(name), msg);
                return;
            }
            const QString uri = Gio::uriOf(free.get());
            emit self->created(uri);
            self->pushUndo(tr("Undo create"), [self, uri] {
                if (self)
                    self->trash({uri});
            });
        };
        if (folder) {
            auto [cb, ud] = Gio::thunk([done](GObject *src, GAsyncResult *res) {
                Gio::Error err;
                const bool ok = g_file_make_directory_finish(G_FILE(src), res, err.out());
                done(ok, err.message());
            });
            g_file_make_directory_async(free.get(), G_PRIORITY_DEFAULT, nullptr, cb, ud);
        } else {
            auto [cb, ud] = Gio::thunk([done](GObject *src, GAsyncResult *res) {
                Gio::Error err;
                GFileOutputStream *out = g_file_create_finish(G_FILE(src), res, err.out());
                if (out) {
                    // Closing is async too, so a slow backend cannot stall the GUI.
                    g_output_stream_close_async(G_OUTPUT_STREAM(out), G_PRIORITY_DEFAULT, nullptr, +[](GObject *, GAsyncResult *, gpointer) {}, nullptr);
                    g_object_unref(out);
                }
                done(out != nullptr, err.message());
            });
            g_file_create_async(free.get(), G_FILE_CREATE_NONE, G_PRIORITY_DEFAULT, nullptr, cb, ud);
        }
    });
}

void FileOperations::createFolder(const QString &parent, const QString &name) { create(parent, name, true); }
void FileOperations::createFile(const QString &parent, const QString &name) { create(parent, name, false); }
