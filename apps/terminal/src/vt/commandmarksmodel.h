#pragma once

#include <QAbstractListModel>
#include <QString>
#include <QtQml/qqmlregistration.h>

// One shell command seen through OSC 133. Lines are absolute and survive scrollback trimming.
struct CommandMark
{
    enum State { Running, Ok, Fail };
    int promptLine = -1;
    int commandLine = -1;
    int outputStartLine = -1;
    int endLine = -1;
    int exitCode = -1;
    qint64 startedMs = 0;
    qint64 durationMs = -1;
    State state = Running;
    QString commandText;
};

// List of finished and running commands of a terminal session, oldest first.
class CommandMarksModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Owned by TerminalSession")
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)
public:
    enum Roles {
        LineRole = Qt::UserRole + 1,
        StateRole,
        ExitCodeRole,
        DurationMsRole,
        CommandTextRole,
        PromptLineRole,
        CommandLineRole,
        OutputLineRole,
        EndLineRole,
        StartedMsRole,
    };

    explicit CommandMarksModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    const QList<CommandMark> &marks() const { return m_marks; }
    Q_INVOKABLE QVariantMap get(int row) const;
    // Row of the mark that owns an absolute line (prompt through end), or -1.
    Q_INVOKABLE int rowForLine(int line) const;

    int append(const CommandMark &mark);
    void finish(int row, int endLine, int exitCode, qint64 durationMs);
    // Drops marks that ended above the first line still kept in scrollback.
    void pruneBefore(int firstLine);
    void clear();

signals:
    void countChanged();

private:
    QList<CommandMark> m_marks;
};
