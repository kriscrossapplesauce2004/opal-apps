#pragma once
#include <QPointer>
#include <QString>
#include <QUrl>
// Qt defines "signals" as a macro and GIO uses it as a member name.
#pragma push_macro("signals")
#undef signals
#include <gio/gio.h>
#pragma pop_macro("signals")
#include <memory>
#include <utility>

// Small GIO helpers: a ref-counting RAII handle, error wrapper, URI helpers and a lambda-to-callback bridge.
// All async GIO calls in this module run on the default main context, which Qt pumps on the GUI thread (see glibloop.h).
namespace Gio {

// Owns one reference to a GObject. Copies add a reference.
template <class T> class Obj
{
public:
    Obj() = default;
    Obj(const Obj &o) : m_p(o.m_p) { if (m_p) g_object_ref(m_p); }
    Obj(Obj &&o) noexcept : m_p(std::exchange(o.m_p, nullptr)) {}
    Obj &operator=(Obj o) noexcept { std::swap(m_p, o.m_p); return *this; }
    ~Obj() { if (m_p) g_object_unref(m_p); }
    // Takes over a reference that the caller already owns.
    static Obj adopt(T *p) { Obj o; o.m_p = p; return o; }
    // Adds a new reference to a borrowed pointer.
    static Obj retain(T *p) { if (p) g_object_ref(p); return adopt(p); }
    T *get() const { return m_p; }
    T *operator->() const { return m_p; }
    explicit operator bool() const { return m_p != nullptr; }
    void reset() { Obj().swap(*this); }
    void swap(Obj &o) noexcept { std::swap(m_p, o.m_p); }

private:
    T *m_p = nullptr;
};

using File = Obj<GFile>;
using Info = Obj<GFileInfo>;
using Cancellable = Obj<GCancellable>;

// Owns one GError and answers the usual questions about it.
class Error
{
public:
    Error() = default;
    explicit Error(GError *e) : m_e(e) {}
    Error(const Error &) = delete;
    Error &operator=(const Error &) = delete;
    ~Error() { if (m_e) g_error_free(m_e); }
    GError **out() { if (m_e) { g_error_free(m_e); m_e = nullptr; } return &m_e; }
    explicit operator bool() const { return m_e != nullptr; }
    bool is(GIOErrorEnum code) const { return m_e && g_error_matches(m_e, G_IO_ERROR, code); }
    bool cancelled() const { return is(G_IO_ERROR_CANCELLED); }
    int code() const { return m_e ? m_e->code : 0; }
    QString message() const { return m_e ? QString::fromUtf8(m_e->message) : QString(); }

private:
    GError *m_e = nullptr;
};

inline QString str(const char *s) { return s ? QString::fromUtf8(s) : QString(); }

// Takes a g_malloc string and returns it as QString.
inline QString take(char *s)
{
    const QString r = str(s);
    g_free(s);
    return r;
}

inline File fileForPath(const QString &path) { return File::adopt(g_file_new_for_path(path.toUtf8().constData())); }
inline File fileForUri(const QString &uri) { return File::adopt(g_file_new_for_uri(uri.toUtf8().constData())); }

// Accepts an absolute path, a "~/" path or any URI that GIO understands.
inline File fileForString(const QString &s)
{
    if (s.isEmpty())
        return {};
    if (s.startsWith(QLatin1Char('/')))
        return fileForPath(s);
    if (s.startsWith(QLatin1String("~/")) || s == QLatin1String("~"))
        return fileForPath(QString::fromUtf8(g_get_home_dir()) + s.mid(1));
    if (s.contains(QLatin1String(":/")))
        return fileForUri(s);
    return File::adopt(g_file_new_for_commandline_arg(s.toUtf8().constData()));
}

inline File fileForUrl(const QUrl &u)
{
    if (u.isLocalFile())
        return fileForPath(u.toLocalFile());
    if (u.scheme().isEmpty())
        return fileForString(u.toString());
    return fileForUri(u.toString(QUrl::FullyEncoded));
}

inline QString uriOf(GFile *f) { return take(g_file_get_uri(f)); }
inline QString pathOf(GFile *f) { return take(g_file_get_path(f)); }

// Carries a lambda through a GIO async call. GIO always calls back exactly once, so the lambda owns itself.
template <class F> struct Thunk
{
    F fn;
    static void call(GObject *source, GAsyncResult *res, gpointer data)
    {
        std::unique_ptr<Thunk> self(static_cast<Thunk *>(data));
        self->fn(source, res);
    }
};

template <class F> std::pair<GAsyncReadyCallback, gpointer> thunk(F fn)
{
    // The raw pointer is freed in Thunk::call, which GIO always runs once.
    return {&Thunk<F>::call, new Thunk<F>{std::move(fn)}};
}

} // namespace Gio
