#include "commandmarksmodel.h"

#include <QVariantMap>
#include <climits>

CommandMarksModel::CommandMarksModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int CommandMarksModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : static_cast<int>(m_marks.size());
}

static QString stateName(CommandMark::State s)
{
    switch (s) {
    case CommandMark::Running: return QStringLiteral("running");
    case CommandMark::Ok: return QStringLiteral("ok");
    case CommandMark::Fail: return QStringLiteral("fail");
    }
    return {};
}

QVariant CommandMarksModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_marks.size())
        return {};
    const CommandMark &m = m_marks.at(index.row());
    switch (role) {
    case LineRole:
    case PromptLineRole: return m.promptLine;
    case StateRole: return stateName(m.state);
    case ExitCodeRole: return m.exitCode;
    case DurationMsRole: return m.durationMs;
    case CommandTextRole: return m.commandText;
    case CommandLineRole: return m.commandLine;
    case OutputLineRole: return m.outputStartLine;
    case EndLineRole: return m.endLine;
    case StartedMsRole: return m.startedMs;
    default: return {};
    }
}

QHash<int, QByteArray> CommandMarksModel::roleNames() const
{
    return {
        {LineRole, "line"}, {StateRole, "state"}, {ExitCodeRole, "exitCode"}, {DurationMsRole, "durationMs"},
        {CommandTextRole, "commandText"}, {PromptLineRole, "promptLine"}, {CommandLineRole, "commandLine"},
        {OutputLineRole, "outputLine"}, {EndLineRole, "endLine"}, {StartedMsRole, "startedMs"},
    };
}

QVariantMap CommandMarksModel::get(int row) const
{
    QVariantMap out;
    if (row < 0 || row >= m_marks.size())
        return out;
    const QHash<int, QByteArray> names = roleNames();
    for (auto it = names.cbegin(); it != names.cend(); ++it)
        out.insert(QString::fromLatin1(it.value()), data(index(row), it.key()));
    return out;
}

int CommandMarksModel::rowForLine(int line) const
{
    for (int i = static_cast<int>(m_marks.size()) - 1; i >= 0; --i) {
        const CommandMark &m = m_marks.at(i);
        const int end = m.state == CommandMark::Running ? INT_MAX : m.endLine;
        if (line >= m.promptLine && line <= end)
            return i;
    }
    return -1;
}

int CommandMarksModel::append(const CommandMark &mark)
{
    const int row = static_cast<int>(m_marks.size());
    beginInsertRows({}, row, row);
    m_marks.append(mark);
    endInsertRows();
    emit countChanged();
    return row;
}

void CommandMarksModel::finish(int row, int endLine, int exitCode, qint64 durationMs)
{
    if (row < 0 || row >= m_marks.size())
        return;
    CommandMark &m = m_marks[row];
    m.endLine = endLine;
    m.exitCode = exitCode;
    m.durationMs = durationMs;
    m.state = exitCode == 0 ? CommandMark::Ok : CommandMark::Fail;
    emit dataChanged(index(row), index(row));
}

void CommandMarksModel::pruneBefore(int firstLine)
{
    int n = 0;
    while (n < m_marks.size() && m_marks.at(n).state != CommandMark::Running && m_marks.at(n).endLine < firstLine)
        ++n;
    if (n == 0)
        return;
    beginRemoveRows({}, 0, n - 1);
    m_marks.remove(0, n);
    endRemoveRows();
    emit countChanged();
}

void CommandMarksModel::clear()
{
    if (m_marks.isEmpty())
        return;
    beginResetModel();
    m_marks.clear();
    endResetModel();
    emit countChanged();
}
