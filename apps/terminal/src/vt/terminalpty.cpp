#include "terminalpty.h"

#include <QDir>
#include <QFileInfo>
#include <QProcessEnvironment>
#include <QStandardPaths>

#include <cerrno>
#include <csignal>
#include <cstring>
#include <fcntl.h>
#include <pty.h>
#include <pwd.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <unistd.h>

Pty::Pty(QObject *parent)
    : QObject(parent)
{
}

Pty::~Pty()
{
    terminate();
}

QString Pty::defaultShell()
{
    const QByteArray env = qgetenv("SHELL");
    if (!env.isEmpty() && access(env.constData(), X_OK) == 0)
        return QString::fromLocal8Bit(env);
    if (const passwd *pw = getpwuid(getuid()); pw && pw->pw_shell && *pw->pw_shell && access(pw->pw_shell, X_OK) == 0)
        return QString::fromLocal8Bit(pw->pw_shell);
    return QStringLiteral("/bin/sh");
}

bool Pty::start(const QString &program, const QStringList &arguments, const QString &workingDirectory, int cols, int rows)
{
    if (m_pid > 0)
        return false;

    const bool isShell = program.isEmpty();
    QString prog = isShell ? defaultShell() : program;
    if (!prog.contains(QLatin1Char('/'))) {
        const QString found = QStandardPaths::findExecutable(prog);
        if (!found.isEmpty())
            prog = found;
    }

    // Everything the child needs is built before the fork, because only async-signal-safe calls are allowed after it.
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    for (const char *name : {"COLUMNS", "LINES", "TERMCAP", "VTE_VERSION", "KITTY_WINDOW_ID", "GHOSTTY_RESOURCES_DIR", "WEZTERM_PANE", "TERM_PROGRAM_VERSION", "TMUX", "STY"})
        env.remove(QString::fromLatin1(name));
    env.insert(QStringLiteral("TERM"), QStringLiteral("xterm-256color"));
    env.insert(QStringLiteral("COLORTERM"), QStringLiteral("truecolor"));
    env.insert(QStringLiteral("TERM_PROGRAM"), QStringLiteral("OpalTerminal"));

    QList<QByteArray> envBytes;
    const QStringList keys = env.keys();
    for (const QString &key : keys)
        envBytes.append((key + QLatin1Char('=') + env.value(key)).toLocal8Bit());
    QList<char *> envp;
    for (QByteArray &b : envBytes)
        envp.append(b.data());
    envp.append(nullptr);

    // A shell gets its basename as argv[0] without a leading dash, so it is interactive but not a login shell.
    const QByteArray progBytes = prog.toLocal8Bit();
    QList<QByteArray> argBytes;
    argBytes.append(isShell ? QFileInfo(prog).fileName().toLocal8Bit() : progBytes);
    for (const QString &a : arguments)
        argBytes.append(a.toLocal8Bit());
    QList<char *> argv;
    for (QByteArray &b : argBytes)
        argv.append(b.data());
    argv.append(nullptr);

    QByteArray cwd = workingDirectory.isEmpty() ? QDir::homePath().toLocal8Bit() : workingDirectory.toLocal8Bit();
    if (access(cwd.constData(), X_OK) != 0)
        cwd = QDir::homePath().toLocal8Bit();

    struct winsize ws = {};
    ws.ws_col = static_cast<unsigned short>(qMax(1, cols));
    ws.ws_row = static_cast<unsigned short>(qMax(1, rows));

    int master = -1;
    const pid_t pid = forkpty(&master, nullptr, nullptr, &ws);
    if (pid < 0)
        return false;
    if (pid == 0) {
        sigset_t none;
        sigemptyset(&none);
        sigprocmask(SIG_SETMASK, &none, nullptr);
        signal(SIGPIPE, SIG_DFL);
        if (chdir(cwd.constData()) != 0) {
            // The child then keeps the inherited directory.
        }
        execve(progBytes.constData(), argv.data(), envp.data());
        _exit(127);
    }

    m_pid = pid;
    m_fd = master;
    m_exited = false;
    fcntl(m_fd, F_SETFL, fcntl(m_fd, F_GETFL) | O_NONBLOCK);
    fcntl(m_fd, F_SETFD, FD_CLOEXEC);

    m_readNotifier = std::make_unique<QSocketNotifier>(m_fd, QSocketNotifier::Read);
    connect(m_readNotifier.get(), &QSocketNotifier::activated, this, &Pty::onReadable);

    // A pidfd tells when the child dies; old kernels fall back to a slow poll.
    m_pidfd = static_cast<int>(syscall(SYS_pidfd_open, pid, 0));
    if (m_pidfd >= 0) {
        m_pidNotifier = std::make_unique<QSocketNotifier>(m_pidfd, QSocketNotifier::Read);
        connect(m_pidNotifier.get(), &QSocketNotifier::activated, this, &Pty::onChildGone);
    } else {
        m_pollTimer = new QTimer(this);
        m_pollTimer->setInterval(250);
        connect(m_pollTimer, &QTimer::timeout, this, &Pty::onChildGone);
        m_pollTimer->start();
    }
    return true;
}

int Pty::foregroundProcessGroup() const
{
    return m_fd >= 0 ? tcgetpgrp(m_fd) : -1;
}

void Pty::write(const QByteArray &data)
{
    if (m_fd < 0 || data.isEmpty())
        return;
    if (!m_pending.isEmpty()) {
        m_pending.append(data);
        return;
    }
    qsizetype done = 0;
    while (done < data.size()) {
        const ssize_t n = ::write(m_fd, data.constData() + done, static_cast<size_t>(data.size() - done));
        if (n > 0) {
            done += n;
        } else if (n < 0 && errno == EINTR) {
            continue;
        } else {
            break;
        }
    }
    if (done < data.size()) {
        // The kernel buffer is full, so the rest waits for a writable signal.
        m_pending = data.mid(done);
        if (!m_writeNotifier) {
            m_writeNotifier = std::make_unique<QSocketNotifier>(m_fd, QSocketNotifier::Write);
            connect(m_writeNotifier.get(), &QSocketNotifier::activated, this, &Pty::onWritable);
        }
        m_writeNotifier->setEnabled(true);
    }
}

void Pty::onWritable()
{
    const QByteArray data = std::exchange(m_pending, QByteArray());
    m_writeNotifier->setEnabled(false);
    write(data);
}

void Pty::resize(int cols, int rows, int pixelWidth, int pixelHeight)
{
    if (m_fd < 0)
        return;
    struct winsize ws = {};
    ws.ws_col = static_cast<unsigned short>(qMax(1, cols));
    ws.ws_row = static_cast<unsigned short>(qMax(1, rows));
    ws.ws_xpixel = static_cast<unsigned short>(qMax(0, pixelWidth));
    ws.ws_ypixel = static_cast<unsigned short>(qMax(0, pixelHeight));
    ioctl(m_fd, TIOCSWINSZ, &ws);
}

void Pty::onReadable()
{
    // One bounded read per wake-up keeps the GUI responsive while a program floods output.
    char buf[65536];
    const ssize_t n = ::read(m_fd, buf, sizeof buf);
    if (n > 0) {
        emit dataReceived(QByteArray(buf, n));
    } else if (n == 0 || (n < 0 && errno != EAGAIN && errno != EINTR)) {
        // EIO means every slave end is closed.
        m_readNotifier->setEnabled(false);
        onChildGone();
    }
}

void Pty::drain()
{
    char buf[65536];
    for (;;) {
        const ssize_t n = ::read(m_fd, buf, sizeof buf);
        if (n > 0)
            emit dataReceived(QByteArray(buf, n));
        else
            break;
    }
}

void Pty::onChildGone()
{
    if (m_exited || m_pid <= 0)
        return;
    int status = 0;
    const pid_t r = waitpid(static_cast<pid_t>(m_pid), &status, WNOHANG);
    if (r == 0) {
        // The slave side closed but the child still lives; poll until it ends.
        if (!m_pollTimer) {
            m_pollTimer = new QTimer(this);
            m_pollTimer->setInterval(100);
            connect(m_pollTimer, &QTimer::timeout, this, &Pty::onChildGone);
            m_pollTimer->start();
        }
        return;
    }
    m_exited = true;
    if (m_pollTimer)
        m_pollTimer->stop();
    if (m_pidNotifier)
        m_pidNotifier->setEnabled(false);
    drain();
    int code = 0;
    if (r > 0)
        code = WIFEXITED(status) ? WEXITSTATUS(status) : (WIFSIGNALED(status) ? 128 + WTERMSIG(status) : 0);
    m_readNotifier->setEnabled(false);
    emit childExited(code);
}

void Pty::reap()
{
    if (m_pid <= 0 || m_exited)
        return;
    int status = 0;
    for (int i = 0; i < 20; ++i) {
        if (waitpid(static_cast<pid_t>(m_pid), &status, WNOHANG) != 0) {
            m_exited = true;
            return;
        }
        usleep(5000);
    }
    kill(static_cast<pid_t>(m_pid), SIGKILL);
    waitpid(static_cast<pid_t>(m_pid), &status, 0);
    m_exited = true;
}

void Pty::terminate()
{
    if (m_pid > 0 && !m_exited) {
        kill(static_cast<pid_t>(m_pid), SIGHUP);
        kill(static_cast<pid_t>(m_pid), SIGCONT);
        reap();
    }
    m_readNotifier.reset();
    m_writeNotifier.reset();
    m_pidNotifier.reset();
    if (m_fd >= 0) {
        close(m_fd);
        m_fd = -1;
    }
    if (m_pidfd >= 0) {
        close(m_pidfd);
        m_pidfd = -1;
    }
    m_pid = -1;
}
