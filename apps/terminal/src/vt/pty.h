#pragma once

#include <QByteArray>
#include <QObject>
#include <QSocketNotifier>
#include <QStringList>
#include <QTimer>

#include <memory>

// Child process on a pseudo terminal. All I/O is event driven on the GUI thread.
class Pty : public QObject
{
    Q_OBJECT
public:
    explicit Pty(QObject *parent = nullptr);
    ~Pty() override;

    // Empty program means the user's shell. Returns false when the fork fails.
    bool start(const QString &program, const QStringList &arguments, const QString &workingDirectory, int cols, int rows);
    static QString defaultShell();

    bool isRunning() const { return m_pid > 0 && !m_exited; }
    qint64 pid() const { return m_pid; }
    int masterFd() const { return m_fd; }
    // Process group in the foreground of the terminal, or -1.
    int foregroundProcessGroup() const;

    void write(const QByteArray &data);
    void resize(int cols, int rows, int pixelWidth = 0, int pixelHeight = 0);
    // Sends SIGHUP and reaps the child so no zombie stays.
    void terminate();

signals:
    void dataReceived(const QByteArray &data);
    // Exit code, or 128 plus the signal number.
    void childExited(int status);

private:
    void onReadable();
    void onWritable();
    void onChildGone();
    void drain();
    void reap();

    int m_fd = -1;
    int m_pidfd = -1;
    qint64 m_pid = -1;
    bool m_exited = false;
    QByteArray m_pending;
    std::unique_ptr<QSocketNotifier> m_readNotifier;
    std::unique_ptr<QSocketNotifier> m_writeNotifier;
    std::unique_ptr<QSocketNotifier> m_pidNotifier;
    QTimer *m_pollTimer = nullptr;
};
