#pragma once

#include "commandmarksmodel.h"
#include "terminalpalette.h"

#include <QBitArray>
#include <QByteArray>
#include <QHash>
#include <QObject>
#include <QStringList>
#include <QTimer>
#include <QVector>
#include <QtQml/qqmlregistration.h>

#include <deque>

struct VTerm;
struct VTermScreen;
class Pty;

// One cell of the grid in a compact form that is cheap to keep in scrollback.
struct TermCell
{
    // Colour words: 0 is the default colour, otherwise the type in the top byte and the value below.
    static constexpr quint32 ColorIndexed = 1u << 24;
    static constexpr quint32 ColorRgb = 2u << 24;
    static constexpr quint32 ClusterFlag = 0x80000000u;
    enum Attr : quint16 {
        Bold = 1,
        Italic = 2,
        UnderlineMask = 0x0c, // 1 single, 2 double, 3 curly in bits 2 and 3
        Reverse = 0x10,
        Strike = 0x20,
        Blink = 0x40,
        Conceal = 0x80,
        Faint = 0x100,
    };

    quint32 ch = 0; // code point, or ClusterFlag plus a cluster index; 0 is empty
    quint32 fg = 0;
    quint32 bg = 0;
    quint16 attrs = 0;
    quint8 width = 1; // 0 marks the right half of a wide character

    bool isBlank() const { return (ch == 0 || ch == ' ') && bg == 0 && !(attrs & (Reverse | UnderlineMask | Strike)); }
    bool operator==(const TermCell &o) const = default;
};

// Terminal emulation for one shell: child process, VT parser, scrollback and shell integration.
class TerminalSession : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(int cols READ cols NOTIFY sizeChanged)
    Q_PROPERTY(int rows READ rows NOTIFY sizeChanged)
    Q_PROPERTY(QString title READ title NOTIFY titleChanged)
    Q_PROPERTY(QString cwd READ cwd NOTIFY cwdChanged)
    Q_PROPERTY(QString foregroundProcess READ foregroundProcess NOTIFY foregroundProcessChanged)
    Q_PROPERTY(int scrollbackLimit READ scrollbackLimit WRITE setScrollbackLimit NOTIFY scrollbackChanged)
    Q_PROPERTY(int scrollbackCount READ scrollbackCount NOTIFY scrollbackChanged)
    Q_PROPERTY(int firstLine READ firstLine NOTIFY scrollbackChanged)
    Q_PROPERTY(int viewOffset READ viewOffset NOTIFY viewportChanged)
    Q_PROPERTY(int cursorRow READ cursorRow NOTIFY cursorChanged)
    Q_PROPERTY(int cursorColumn READ cursorColumn NOTIFY cursorChanged)
    Q_PROPERTY(bool cursorVisible READ cursorVisible NOTIFY cursorChanged)
    Q_PROPERTY(CursorShape cursorShape READ cursorShape NOTIFY cursorChanged)
    Q_PROPERTY(bool cursorBlink READ cursorBlink NOTIFY cursorChanged)
    Q_PROPERTY(bool altScreen READ altScreen NOTIFY altScreenChanged)
    Q_PROPERTY(MouseMode mouseMode READ mouseMode NOTIFY mouseModeChanged)
    Q_PROPERTY(bool running READ running NOTIFY runningChanged)
    Q_PROPERTY(TerminalPalette *palette READ palette WRITE setPalette NOTIFY paletteChanged)
    Q_PROPERTY(CommandMarksModel *marks READ marks CONSTANT)
    Q_PROPERTY(QString program MEMBER m_program)
    Q_PROPERTY(QStringList arguments MEMBER m_arguments)
    Q_PROPERTY(QString workingDirectory MEMBER m_workingDirectory)
public:
    enum CursorShape { Block, Underline, Bar };
    Q_ENUM(CursorShape)
    enum MouseMode { MouseNone, MouseClick, MouseDrag, MouseMove };
    Q_ENUM(MouseMode)

    explicit TerminalSession(QObject *parent = nullptr);
    ~TerminalSession() override;

    int cols() const { return m_cols; }
    int rows() const { return m_rows; }
    QString title() const { return m_title; }
    QString cwd() const { return m_cwd; }
    QString foregroundProcess() const { return m_foreground; }
    int scrollbackLimit() const { return m_scrollbackLimit; }
    void setScrollbackLimit(int lines);
    int scrollbackCount() const { return static_cast<int>(m_scrollback.size()); }
    // Absolute number of the oldest line still in scrollback.
    int firstLine() const { return m_firstLine; }
    int viewOffset() const { return m_viewOffset; }
    int cursorRow() const { return m_cursorRow; }
    int cursorColumn() const { return m_cursorCol; }
    bool cursorVisible() const { return m_cursorVisible; }
    CursorShape cursorShape() const { return m_cursorShape; }
    bool cursorBlink() const { return m_cursorBlink; }
    bool altScreen() const { return m_altScreen; }
    MouseMode mouseMode() const { return m_mouseMode; }
    bool running() const;
    TerminalPalette *palette() const { return m_palette; }
    void setPalette(TerminalPalette *palette);
    CommandMarksModel *marks() const { return m_marks; }

    // Absolute number of the line shown in the first row of the viewport.
    int viewTopLine() const { return m_firstLine + scrollbackCount() - m_viewOffset; }
    // Absolute number of the first line of the live screen.
    int screenTopLine() const { return m_firstLine + scrollbackCount(); }
    int lineCount() const { return scrollbackCount() + m_rows; }

    // Fills out with cols() cells of one viewport row.
    void fetchViewRow(int viewRow, QVector<TermCell> &out) const;
    // Fills out with cols() cells of an absolute line, or returns false when the line is gone.
    bool fetchLine(int absLine, QVector<TermCell> &out) const;
    QString clusterText(quint32 ch) const;
    // Text from the start cell up to, but not including, the end cell. Wrapped lines join without a newline.
    QString textBetween(int startLine, int startCol, int endLine, int endCol) const;
    QString lineText(int absLine) const;

    // Rows of the viewport changed since the last clearDirty().
    const QBitArray &dirtyRows() const { return m_dirty; }
    bool allDirty() const { return m_allDirty; }
    void clearDirty();
    void markAllDirty();

    // A key event mapped to terminal bytes.
    Q_INVOKABLE void sendKey(int qtKey, int modifiers, const QString &text);
    // Typed text; paste wraps it in bracketed paste when the application asked for it.
    Q_INVOKABLE void sendText(const QString &text, bool paste = false);
    Q_INVOKABLE void paste(const QString &text) { sendText(text, true); }
    Q_INVOKABLE void scrollViewport(int lines);
    Q_INVOKABLE void scrollToBottom();
    Q_INVOKABLE void resize(int cols, int rows, int pixelWidth = 0, int pixelHeight = 0);
    Q_INVOKABLE bool start();
    Q_INVOKABLE void focusChanged(bool focused);
    // Mouse reporting for programs that ask for it. Row and column count from zero.
    void mouseMove(int row, int col, int modifiers);
    void mouseButton(int button, bool pressed, int modifiers);

    // Test seam: feeds bytes as if the child wrote them.
    Q_INVOKABLE void feed(const QByteArray &bytes);
    // Test seam: bytes the terminal would have sent to the child while no process runs.
    Q_INVOKABLE QByteArray takeOutput();
    Pty *pty() const { return m_pty; }

signals:
    // Fired after output was processed and the grid or cursor changed.
    void damaged();
    void sizeChanged();
    void titleChanged();
    void cwdChanged();
    void foregroundProcessChanged();
    void scrollbackChanged();
    void viewportChanged();
    void cursorChanged();
    void altScreenChanged();
    void mouseModeChanged();
    void runningChanged();
    void paletteChanged();
    void bell();
    void childExited(int status);
    void commandStarted(int row);
    void commandFinished(int row);

private:
    friend struct SessionCallbacks;
    void writeOutput(const QByteArray &bytes);
    void onPtyData(const QByteArray &data);
    void markScreenRows(int top, int bottomExclusive);
    void pushScrollbackLine(int cols, const void *cells);
    int popScrollbackLine(int cols, void *cells);
    void handleTermProp(int prop, const void *value);
    void handleOsc(int command, const QByteArray &payload);
    void handleShellMark(const QByteArray &payload);
    void refreshForeground();
    void scheduleForegroundCheck();
    TermCell convertCell(const void *screenCell) const;
    quint32 internCluster(const QString &text);
    void trimScrollback();

    VTerm *m_vt = nullptr;
    VTermScreen *m_screen = nullptr;
    Pty *m_pty = nullptr;
    TerminalPalette *m_palette = nullptr;
    CommandMarksModel *m_marks = nullptr;

    int m_cols = 80;
    int m_rows = 24;
    QString m_title;
    QString m_cwd;
    QString m_foreground;
    QString m_program;
    QStringList m_arguments;
    QString m_workingDirectory;
    int m_scrollbackLimit = 10000;
    std::deque<QVector<TermCell>> m_scrollback;
    int m_firstLine = 0;
    int m_viewOffset = 0;
    int m_cursorRow = 0;
    int m_cursorCol = 0;
    bool m_cursorVisible = true;
    CursorShape m_cursorShape = Block;
    bool m_cursorBlink = true;
    bool m_altScreen = false;
    MouseMode m_mouseMode = MouseNone;
    bool m_exited = false;

    QBitArray m_dirty;
    bool m_allDirty = true;
    QByteArray m_output; // output kept while no process runs
    QByteArray m_titleBuffer;
    QByteArray m_oscBuffer;
    int m_oscCommand = -1;
    QStringList m_clusters;
    QHash<QString, quint32> m_clusterIndex;

    // Shell integration state.
    CommandMark m_pendingMark;
    int m_pendingStage = 0; // 0 none, 1 prompt seen, 2 command line seen
    int m_commandCol = 0;
    int m_runningRow = -1;

    QTimer m_foregroundTimer;
    qint64 m_lastForegroundCheck = 0;
};
