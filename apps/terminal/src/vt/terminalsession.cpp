#include "terminalsession.h"

#include "keytranslation.h"
#include "terminalpty.h"

#include <QDateTime>
#include <QFile>
#include <QUrl>

#include <algorithm>
#include <vterm.h>

namespace {

quint32 packColor(const VTermColor &c)
{
    if (VTERM_COLOR_IS_DEFAULT_FG(&c) || VTERM_COLOR_IS_DEFAULT_BG(&c))
        return 0;
    if (VTERM_COLOR_IS_INDEXED(&c))
        return TermCell::ColorIndexed | c.indexed.idx;
    return TermCell::ColorRgb | (quint32(c.rgb.red) << 16) | (quint32(c.rgb.green) << 8) | c.rgb.blue;
}

} // namespace

// Static trampolines from the C callbacks of libvterm into the session.
struct SessionCallbacks
{
    static TerminalSession *self(void *user) { return static_cast<TerminalSession *>(user); }

    static int damage(VTermRect rect, void *user)
    {
        self(user)->markScreenRows(rect.start_row, rect.end_row);
        return 1;
    }
    static int moverect(VTermRect dest, VTermRect src, void *user)
    {
        self(user)->markScreenRows(dest.start_row, dest.end_row);
        self(user)->markScreenRows(src.start_row, src.end_row);
        return 1;
    }
    static int movecursor(VTermPos pos, VTermPos oldpos, int, void *user)
    {
        TerminalSession *s = self(user);
        s->markScreenRows(oldpos.row, oldpos.row + 1);
        s->markScreenRows(pos.row, pos.row + 1);
        if (s->m_cursorRow != pos.row || s->m_cursorCol != pos.col) {
            s->m_cursorRow = pos.row;
            s->m_cursorCol = pos.col;
            emit s->cursorChanged();
        }
        return 1;
    }
    static int settermprop(VTermProp prop, VTermValue *val, void *user)
    {
        self(user)->handleTermProp(prop, val);
        return 1;
    }
    static int bell(void *user)
    {
        emit self(user)->bell();
        return 1;
    }
    static int pushline(int cols, const VTermScreenCell *cells, void *user)
    {
        self(user)->pushScrollbackLine(cols, cells);
        return 1;
    }
    static int popline(int cols, VTermScreenCell *cells, void *user)
    {
        return self(user)->popScrollbackLine(cols, cells);
    }
    static int sbClear(void *user)
    {
        TerminalSession *s = self(user);
        s->m_firstLine += static_cast<int>(s->m_scrollback.size());
        s->m_scrollback.clear();
        s->m_viewOffset = 0;
        s->markAllDirty();
        emit s->scrollbackChanged();
        return 1;
    }
    static int osc(int command, VTermStringFragment frag, void *user)
    {
        TerminalSession *s = self(user);
        if (frag.initial) {
            s->m_oscBuffer.clear();
            s->m_oscCommand = command;
        }
        s->m_oscBuffer.append(frag.str, static_cast<qsizetype>(frag.len));
        if (frag.final) {
            const QByteArray payload = std::exchange(s->m_oscBuffer, QByteArray());
            s->handleOsc(command, payload);
        }
        return 1;
    }
    static void output(const char *bytes, size_t len, void *user)
    {
        self(user)->writeOutput(QByteArray(bytes, static_cast<qsizetype>(len)));
    }
};

TerminalSession::TerminalSession(QObject *parent)
    : QObject(parent)
    , m_palette(new TerminalPalette(this))
    , m_marks(new CommandMarksModel(this))
{
    m_vt = vterm_new(m_rows, m_cols);
    vterm_set_utf8(m_vt, 1);
    m_screen = vterm_obtain_screen(m_vt);

    static const VTermScreenCallbacks screenCallbacks = {
        SessionCallbacks::damage, SessionCallbacks::moverect, SessionCallbacks::movecursor, SessionCallbacks::settermprop,
        SessionCallbacks::bell, nullptr, SessionCallbacks::pushline, SessionCallbacks::popline, SessionCallbacks::sbClear,
    };
    static const VTermStateFallbacks fallbacks = {nullptr, nullptr, SessionCallbacks::osc, nullptr, nullptr, nullptr, nullptr};

    vterm_screen_enable_altscreen(m_screen, 1);
    vterm_screen_set_callbacks(m_screen, &screenCallbacks, this);
    vterm_screen_set_unrecognised_fallbacks(m_screen, &fallbacks, this);
    vterm_screen_set_damage_merge(m_screen, VTERM_DAMAGE_ROW);
    vterm_output_set_callback(m_vt, SessionCallbacks::output, this);
    vterm_screen_reset(m_screen, 1);

    m_dirty = QBitArray(m_rows, true);
    m_foregroundTimer.setSingleShot(true);
    m_foregroundTimer.setInterval(300);
    connect(&m_foregroundTimer, &QTimer::timeout, this, &TerminalSession::refreshForeground);
    connect(m_palette, &TerminalPalette::changed, this, [this] { markAllDirty(); emit damaged(); });
}

TerminalSession::~TerminalSession()
{
    // The child must go before libvterm so late output cannot reach freed memory.
    delete m_pty;
    m_pty = nullptr;
    vterm_free(m_vt);
}

bool TerminalSession::running() const
{
    return m_pty && m_pty->isRunning();
}

void TerminalSession::setPalette(TerminalPalette *palette)
{
    if (!palette || palette == m_palette)
        return;
    if (m_palette)
        disconnect(m_palette, nullptr, this, nullptr);
    m_palette = palette;
    connect(m_palette, &TerminalPalette::changed, this, [this] { markAllDirty(); emit damaged(); });
    markAllDirty();
    emit paletteChanged();
    emit damaged();
}

void TerminalSession::setScrollbackLimit(int lines)
{
    lines = std::max(0, lines);
    if (lines == m_scrollbackLimit)
        return;
    m_scrollbackLimit = lines;
    trimScrollback();
    emit scrollbackChanged();
}

bool TerminalSession::start()
{
    if (m_pty)
        return false;
    m_pty = new Pty(this);
    connect(m_pty, &Pty::dataReceived, this, &TerminalSession::onPtyData);
    connect(m_pty, &Pty::childExited, this, [this](int status) {
        m_exited = true;
        m_foreground.clear();
        emit foregroundProcessChanged();
        emit runningChanged();
        emit childExited(status);
    });
    if (!m_pty->start(m_program, m_arguments, m_workingDirectory, m_cols, m_rows)) {
        delete m_pty;
        m_pty = nullptr;
        return false;
    }
    emit runningChanged();
    return true;
}

void TerminalSession::writeOutput(const QByteArray &bytes)
{
    if (m_pty && m_pty->isRunning())
        m_pty->write(bytes);
    else
        m_output.append(bytes);
}

QByteArray TerminalSession::takeOutput()
{
    return std::exchange(m_output, QByteArray());
}

void TerminalSession::onPtyData(const QByteArray &data)
{
    feed(data);
}

void TerminalSession::feed(const QByteArray &bytes)
{
    vterm_input_write(m_vt, bytes.constData(), static_cast<size_t>(bytes.size()));
    vterm_screen_flush_damage(m_screen);
    emit damaged();
    scheduleForegroundCheck();
}

void TerminalSession::markScreenRows(int top, int bottomExclusive)
{
    for (int r = std::max(0, top); r < bottomExclusive && r < m_rows; ++r) {
        const int viewRow = r + m_viewOffset;
        if (viewRow >= 0 && viewRow < m_dirty.size())
            m_dirty.setBit(viewRow);
    }
}

void TerminalSession::markAllDirty()
{
    m_allDirty = true;
}

void TerminalSession::clearDirty()
{
    m_dirty.fill(false);
    m_allDirty = false;
}

quint32 TerminalSession::internCluster(const QString &text)
{
    auto it = m_clusterIndex.constFind(text);
    if (it != m_clusterIndex.constEnd())
        return it.value();
    const quint32 id = TermCell::ClusterFlag | static_cast<quint32>(m_clusters.size());
    m_clusters.append(text);
    m_clusterIndex.insert(text, id);
    return id;
}

QString TerminalSession::clusterText(quint32 ch) const
{
    if (ch & TermCell::ClusterFlag) {
        const qsizetype i = ch & ~TermCell::ClusterFlag;
        return i < m_clusters.size() ? m_clusters.at(i) : QString();
    }
    return ch ? QString::fromUcs4(reinterpret_cast<const char32_t *>(&ch), 1) : QString();
}

TermCell TerminalSession::convertCell(const void *screenCell) const
{
    const auto &c = *static_cast<const VTermScreenCell *>(screenCell);
    TermCell out;
    if (c.chars[0] == static_cast<uint32_t>(-1)) {
        out.width = 0;
    } else {
        out.width = static_cast<quint8>(c.width);
        if (c.chars[1] != 0 && c.chars[0] != 0) {
            QString s;
            for (int i = 0; i < VTERM_MAX_CHARS_PER_CELL && c.chars[i]; ++i)
                s += QString::fromUcs4(reinterpret_cast<const char32_t *>(&c.chars[i]), 1);
            out.ch = const_cast<TerminalSession *>(this)->internCluster(s);
        } else {
            out.ch = c.chars[0];
        }
    }
    out.fg = packColor(c.fg);
    out.bg = packColor(c.bg);
    quint16 a = 0;
    if (c.attrs.bold) a |= TermCell::Bold;
    if (c.attrs.italic) a |= TermCell::Italic;
    a |= static_cast<quint16>((c.attrs.underline & 3) << 2);
    if (c.attrs.reverse) a |= TermCell::Reverse;
    if (c.attrs.strike) a |= TermCell::Strike;
    if (c.attrs.blink) a |= TermCell::Blink;
    if (c.attrs.conceal) a |= TermCell::Conceal;
    out.attrs = a;
    return out;
}

void TerminalSession::pushScrollbackLine(int cols, const void *cells)
{
    const auto *src = static_cast<const VTermScreenCell *>(cells);
    QVector<TermCell> line;
    line.reserve(cols);
    for (int i = 0; i < cols; ++i)
        line.append(convertCell(&src[i]));
    // Blank cells at the end carry no information, so they are not kept.
    while (!line.isEmpty() && line.last().isBlank() && line.last().width == 1)
        line.removeLast();
    line.squeeze();
    m_scrollback.push_back(std::move(line));
    if (m_viewOffset > 0)
        m_viewOffset = std::min(m_viewOffset + 1, scrollbackCount());
    markAllDirty();
    trimScrollback();
    emit scrollbackChanged();
}

void TerminalSession::trimScrollback()
{
    bool trimmed = false;
    while (static_cast<int>(m_scrollback.size()) > m_scrollbackLimit) {
        m_scrollback.pop_front();
        ++m_firstLine;
        trimmed = true;
    }
    if (trimmed) {
        m_viewOffset = std::min(m_viewOffset, scrollbackCount());
        m_marks->pruneBefore(m_firstLine);
        markAllDirty();
    }
}

int TerminalSession::popScrollbackLine(int cols, void *cells)
{
    if (m_scrollback.empty())
        return 0;
    auto *dst = static_cast<VTermScreenCell *>(cells);
    const QVector<TermCell> line = std::move(m_scrollback.back());
    m_scrollback.pop_back();
    VTermState *state = vterm_obtain_state(m_vt);
    VTermColor dfg, dbg;
    vterm_state_get_default_colors(state, &dfg, &dbg);
    for (int i = 0; i < cols; ++i) {
        VTermScreenCell &c = dst[i];
        c = VTermScreenCell();
        c.width = 1;
        c.fg = dfg;
        c.bg = dbg;
        if (i >= line.size())
            continue;
        const TermCell &t = line.at(i);
        if (t.width == 0) {
            c.chars[0] = static_cast<uint32_t>(-1);
            continue;
        }
        c.width = static_cast<char>(t.width);
        const QString text = clusterText(t.ch);
        const QList<uint> cps = text.toUcs4();
        for (int k = 0; k < cps.size() && k < VTERM_MAX_CHARS_PER_CELL; ++k)
            c.chars[k] = cps.at(k);
        c.attrs.bold = t.attrs & TermCell::Bold;
        c.attrs.italic = (t.attrs & TermCell::Italic) != 0;
        c.attrs.underline = (t.attrs & TermCell::UnderlineMask) >> 2;
        c.attrs.reverse = (t.attrs & TermCell::Reverse) != 0;
        c.attrs.strike = (t.attrs & TermCell::Strike) != 0;
        c.attrs.blink = (t.attrs & TermCell::Blink) != 0;
        c.attrs.conceal = (t.attrs & TermCell::Conceal) != 0;
        auto unpack = [](quint32 word, VTermColor &color) {
            if ((word & 0xff000000u) == TermCell::ColorIndexed)
                vterm_color_indexed(&color, static_cast<uint8_t>(word & 0xff));
            else if ((word & 0xff000000u) == TermCell::ColorRgb)
                vterm_color_rgb(&color, (word >> 16) & 0xff, (word >> 8) & 0xff, word & 0xff);
        };
        unpack(t.fg, c.fg);
        unpack(t.bg, c.bg);
    }
    m_viewOffset = std::min(m_viewOffset, scrollbackCount());
    markAllDirty();
    emit scrollbackChanged();
    return 1;
}

void TerminalSession::handleTermProp(int prop, const void *value)
{
    const auto *val = static_cast<const VTermValue *>(value);
    switch (prop) {
    case VTERM_PROP_CURSORVISIBLE:
        m_cursorVisible = val->boolean;
        markScreenRows(m_cursorRow, m_cursorRow + 1);
        emit cursorChanged();
        break;
    case VTERM_PROP_CURSORBLINK:
        m_cursorBlink = val->boolean;
        emit cursorChanged();
        break;
    case VTERM_PROP_CURSORSHAPE:
        m_cursorShape = val->number == VTERM_PROP_CURSORSHAPE_UNDERLINE ? Underline : val->number == VTERM_PROP_CURSORSHAPE_BAR_LEFT ? Bar : Block;
        markScreenRows(m_cursorRow, m_cursorRow + 1);
        emit cursorChanged();
        break;
    case VTERM_PROP_ALTSCREEN:
        m_altScreen = val->boolean;
        m_viewOffset = 0;
        markAllDirty();
        emit altScreenChanged();
        emit viewportChanged();
        break;
    case VTERM_PROP_MOUSE:
        m_mouseMode = static_cast<MouseMode>(std::clamp(val->number, 0, 3));
        emit mouseModeChanged();
        break;
    case VTERM_PROP_TITLE: {
        if (val->string.initial)
            m_titleBuffer.clear();
        m_titleBuffer.append(val->string.str, static_cast<qsizetype>(val->string.len));
        if (val->string.final) {
            const QString t = QString::fromUtf8(m_titleBuffer);
            if (t != m_title) {
                m_title = t;
                emit titleChanged();
            }
        }
        break;
    }
    default:
        break;
    }
}

void TerminalSession::handleOsc(int command, const QByteArray &payload)
{
    if (command == 7) {
        // The shell reports its directory as file://host/path.
        const QUrl url = QUrl::fromEncoded(payload);
        const QString path = url.isLocalFile() || url.scheme() == QLatin1String("file") ? url.path() : QString();
        if (!path.isEmpty() && path != m_cwd) {
            m_cwd = path;
            emit cwdChanged();
        }
    } else if (command == 133) {
        handleShellMark(payload);
    }
}

void TerminalSession::handleShellMark(const QByteArray &payload)
{
    if (payload.isEmpty())
        return;
    VTermPos pos;
    vterm_state_get_cursorpos(vterm_obtain_state(m_vt), &pos);
    const int line = screenTopLine() + pos.row;
    const char kind = payload.at(0);

    if (kind == 'A') {
        // A new prompt replaces a prompt that never ran a command.
        m_pendingMark = CommandMark();
        m_pendingMark.promptLine = line;
        m_pendingStage = 1;
    } else if (kind == 'B') {
        if (m_pendingStage == 0) {
            m_pendingMark = CommandMark();
            m_pendingMark.promptLine = line;
        }
        m_pendingMark.commandLine = line;
        m_commandCol = pos.col;
        m_pendingStage = 2;
    } else if (kind == 'C') {
        if (m_pendingStage == 0) {
            m_pendingMark = CommandMark();
            m_pendingMark.promptLine = line;
            m_pendingMark.commandLine = line;
        }
        CommandMark mark = m_pendingMark;
        mark.outputStartLine = line;
        mark.startedMs = QDateTime::currentMSecsSinceEpoch();
        mark.state = CommandMark::Running;
        mark.commandText = textBetween(mark.commandLine, m_pendingStage == 2 ? m_commandCol : 0, line, pos.col).trimmed();
        m_runningRow = m_marks->append(mark);
        m_pendingStage = 0;
        emit commandStarted(m_runningRow);
        scheduleForegroundCheck();
    } else if (kind == 'D') {
        if (m_runningRow >= 0 && m_runningRow < m_marks->rowCount()) {
            int exitCode = 0;
            if (payload.size() > 2 && payload.at(1) == ';')
                exitCode = payload.mid(2).split(';').first().toInt();
            const CommandMark &m = m_marks->marks().at(m_runningRow);
            const int end = std::max(m.outputStartLine, pos.col == 0 ? line - 1 : line);
            const qint64 duration = QDateTime::currentMSecsSinceEpoch() - m.startedMs;
            const int row = m_runningRow;
            m_runningRow = -1;
            m_marks->finish(row, end, exitCode, duration);
            emit commandFinished(row);
            scheduleForegroundCheck();
        }
        m_pendingStage = 0;
    }
}

void TerminalSession::scheduleForegroundCheck()
{
    if (!m_pty || !m_pty->isRunning() || m_foregroundTimer.isActive())
        return;
    // Reading /proc is cheap, but a burst of output must not cause one read per chunk.
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    if (now - m_lastForegroundCheck >= 300)
        refreshForeground();
    else
        m_foregroundTimer.start();
}

void TerminalSession::refreshForeground()
{
    m_lastForegroundCheck = QDateTime::currentMSecsSinceEpoch();
    if (!m_pty || !m_pty->isRunning())
        return;
    const int pgid = m_pty->foregroundProcessGroup();
    if (pgid <= 0)
        return;
    QFile f(QStringLiteral("/proc/%1/comm").arg(pgid));
    if (!f.open(QIODevice::ReadOnly))
        return;
    const QString name = QString::fromLocal8Bit(f.readLine(256)).trimmed();
    if (!name.isEmpty() && name != m_foreground) {
        m_foreground = name;
        emit foregroundProcessChanged();
    }
}

void TerminalSession::resize(int cols, int rows, int pixelWidth, int pixelHeight)
{
    cols = std::clamp(cols, 1, 1000);
    rows = std::clamp(rows, 1, 1000);
    if (m_pty)
        m_pty->resize(cols, rows, pixelWidth, pixelHeight);
    if (cols == m_cols && rows == m_rows)
        return;
    m_cols = cols;
    m_rows = rows;
    m_dirty = QBitArray(rows, true);
    vterm_set_size(m_vt, rows, cols);
    vterm_screen_flush_damage(m_screen);
    VTermPos pos;
    vterm_state_get_cursorpos(vterm_obtain_state(m_vt), &pos);
    m_cursorRow = pos.row;
    m_cursorCol = pos.col;
    m_viewOffset = std::min(m_viewOffset, scrollbackCount());
    markAllDirty();
    emit sizeChanged();
    emit cursorChanged();
    emit scrollbackChanged();
    emit damaged();
}

bool TerminalSession::fetchLine(int absLine, QVector<TermCell> &out) const
{
    const int idx = absLine - m_firstLine;
    out.resize(m_cols);
    if (idx < 0)
        return false;
    if (idx < scrollbackCount()) {
        const QVector<TermCell> &src = m_scrollback[static_cast<size_t>(idx)];
        const int n = static_cast<int>(std::min<qsizetype>(src.size(), m_cols));
        std::copy_n(src.constBegin(), n, out.begin());
        std::fill(out.begin() + n, out.end(), TermCell());
        return true;
    }
    const int row = idx - scrollbackCount();
    if (row >= m_rows)
        return false;
    VTermScreenCell cell;
    for (int c = 0; c < m_cols; ++c) {
        if (vterm_screen_get_cell(m_screen, VTermPos{row, c}, &cell))
            out[c] = convertCell(&cell);
        else
            out[c] = TermCell();
    }
    return true;
}

void TerminalSession::fetchViewRow(int viewRow, QVector<TermCell> &out) const
{
    if (!fetchLine(viewTopLine() + viewRow, out)) {
        out.resize(m_cols);
        std::fill(out.begin(), out.end(), TermCell());
    }
}

QString TerminalSession::lineText(int absLine) const
{
    return textBetween(absLine, 0, absLine, m_cols);
}

QString TerminalSession::textBetween(int startLine, int startCol, int endLine, int endCol) const
{
    QString out;
    QVector<TermCell> cells;
    for (int l = startLine; l <= endLine; ++l) {
        if (!fetchLine(l, cells))
            continue;
        const int c0 = l == startLine ? std::clamp(startCol, 0, m_cols) : 0;
        const int c1 = l == endLine ? std::clamp(endCol, 0, m_cols) : m_cols;
        QString text;
        for (int c = c0; c < c1; ++c) {
            const TermCell &cell = cells.at(c);
            if (cell.width == 0)
                continue;
            text += cell.ch ? clusterText(cell.ch) : QStringLiteral(" ");
        }
        // A full row that runs on in the next row is one wrapped line.
        const bool wrapped = !cells.isEmpty() && !cells.last().isBlank();
        if (c1 == m_cols && !wrapped) {
            while (text.endsWith(QLatin1Char(' ')))
                text.chop(1);
        }
        out += text;
        if (l != endLine && !wrapped)
            out += QLatin1Char('\n');
    }
    return out;
}

void TerminalSession::sendKey(int qtKey, int modifiers, const QString &text)
{
    const KeyTranslation t = KeyTranslator::translate(qtKey, modifiers, text);
    if (t.kind == KeyTranslation::None)
        return;
    scrollToBottom();
    switch (t.kind) {
    case KeyTranslation::Key:
        vterm_keyboard_key(m_vt, static_cast<VTermKey>(t.vtermKey), static_cast<VTermModifier>(t.vtermMods));
        break;
    case KeyTranslation::Char:
        vterm_keyboard_unichar(m_vt, t.unichar, static_cast<VTermModifier>(t.vtermMods));
        break;
    case KeyTranslation::Text:
        sendText(t.text);
        break;
    default:
        break;
    }
}

void TerminalSession::sendText(const QString &text, bool paste)
{
    if (text.isEmpty())
        return;
    scrollToBottom();
    QString t = text;
    t.replace(QStringLiteral("\r\n"), QStringLiteral("\r"));
    t.replace(QLatin1Char('\n'), QLatin1Char('\r'));
    if (paste) {
        // An escape inside pasted text could end the bracket early, so it is dropped.
        t.remove(QLatin1Char('\x1b'));
        vterm_keyboard_start_paste(m_vt);
    }
    writeOutput(t.toUtf8());
    if (paste)
        vterm_keyboard_end_paste(m_vt);
}

void TerminalSession::scrollViewport(int lines)
{
    const int target = std::clamp(m_viewOffset + lines, 0, scrollbackCount());
    if (target == m_viewOffset)
        return;
    m_viewOffset = target;
    markAllDirty();
    emit viewportChanged();
    emit damaged();
}

void TerminalSession::scrollToBottom()
{
    if (m_viewOffset != 0)
        scrollViewport(-m_viewOffset);
}

void TerminalSession::focusChanged(bool focused)
{
    VTermState *state = vterm_obtain_state(m_vt);
    if (focused)
        vterm_state_focus_in(state);
    else
        vterm_state_focus_out(state);
}

void TerminalSession::mouseMove(int row, int col, int modifiers)
{
    vterm_mouse_move(m_vt, row, col, static_cast<VTermModifier>(KeyTranslator::translate(0, modifiers, QString()).vtermMods));
}

void TerminalSession::mouseButton(int button, bool pressed, int modifiers)
{
    vterm_mouse_button(m_vt, button, pressed, static_cast<VTermModifier>(KeyTranslator::translate(0, modifiers, QString()).vtermMods));
}
