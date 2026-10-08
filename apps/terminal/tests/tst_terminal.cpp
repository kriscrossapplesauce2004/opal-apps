#include "vt/keytranslation.h"
#include "vt/terminalpty.h"
#include "vt/terminalsession.h"

#include <QSignalSpy>
#include <QtTest>

#include <cerrno>
#include <sys/wait.h>

class TstTerminal : public QObject
{
    Q_OBJECT
private slots:
    void keyTranslation_data();
    void keyTranslation();
    void keyTranslationKinds();
    void osc133Marks();
    void osc133Failure();
    void osc133IgnoresPromptOnly();
    void osc7Cwd();
    void osc2Title();
    void scrollbackBound();
    void scrollbackAbsoluteLines();
    void resizeKeepsContent();
    void wideChars();
    void combiningCluster();
    void colors();
    void cursorShapeAndAltScreen();
    void bracketedPaste();
    void viewportScroll();
    void ptyDeliversOutputAndStatus();
    void ptyExitCode();
    void ptyNoZombie();
    void sessionWithPty();
};

static QByteArray typed(int key, int mods = 0, const QString &text = QString())
{
    TerminalSession s;
    s.sendKey(key, mods, text);
    return s.takeOutput();
}

void TstTerminal::keyTranslation_data()
{
    QTest::addColumn<int>("key");
    QTest::addColumn<int>("mods");
    QTest::addColumn<QString>("text");
    QTest::addColumn<QByteArray>("bytes");

    const int shift = Qt::ShiftModifier, ctrl = Qt::ControlModifier, alt = Qt::AltModifier;
    QTest::newRow("a") << int(Qt::Key_A) << 0 << "a" << QByteArray("a");
    QTest::newRow("A shift") << int(Qt::Key_A) << shift << "A" << QByteArray("A");
    QTest::newRow("utf8") << int(Qt::Key_Eacute) << 0 << QString::fromUtf8("é") << QByteArray("\xc3\xa9");
    QTest::newRow("space") << int(Qt::Key_Space) << 0 << " " << QByteArray(" ");
    QTest::newRow("enter") << int(Qt::Key_Return) << 0 << "\r" << QByteArray("\r");
    QTest::newRow("tab") << int(Qt::Key_Tab) << 0 << "\t" << QByteArray("\t");
    QTest::newRow("backtab") << int(Qt::Key_Backtab) << shift << QString() << QByteArray("\x1b[Z");
    QTest::newRow("backspace") << int(Qt::Key_Backspace) << 0 << QString() << QByteArray("\x7f");
    QTest::newRow("escape") << int(Qt::Key_Escape) << 0 << QString() << QByteArray("\x1b");
    QTest::newRow("up") << int(Qt::Key_Up) << 0 << QString() << QByteArray("\x1b[A");
    QTest::newRow("down") << int(Qt::Key_Down) << 0 << QString() << QByteArray("\x1b[B");
    QTest::newRow("right") << int(Qt::Key_Right) << 0 << QString() << QByteArray("\x1b[C");
    QTest::newRow("left") << int(Qt::Key_Left) << 0 << QString() << QByteArray("\x1b[D");
    QTest::newRow("ctrl-left") << int(Qt::Key_Left) << ctrl << QString() << QByteArray("\x1b[1;5D");
    QTest::newRow("home") << int(Qt::Key_Home) << 0 << QString() << QByteArray("\x1b[H");
    QTest::newRow("end") << int(Qt::Key_End) << 0 << QString() << QByteArray("\x1b[F");
    QTest::newRow("delete") << int(Qt::Key_Delete) << 0 << QString() << QByteArray("\x1b[3~");
    QTest::newRow("insert") << int(Qt::Key_Insert) << 0 << QString() << QByteArray("\x1b[2~");
    QTest::newRow("pgup") << int(Qt::Key_PageUp) << 0 << QString() << QByteArray("\x1b[5~");
    QTest::newRow("pgdn") << int(Qt::Key_PageDown) << 0 << QString() << QByteArray("\x1b[6~");
    QTest::newRow("f1") << int(Qt::Key_F1) << 0 << QString() << QByteArray("\x1bOP");
    QTest::newRow("f5") << int(Qt::Key_F5) << 0 << QString() << QByteArray("\x1b[15~");
    QTest::newRow("f12") << int(Qt::Key_F12) << 0 << QString() << QByteArray("\x1b[24~");
    QTest::newRow("ctrl-c") << int(Qt::Key_C) << ctrl << QString(QChar(3)) << QByteArray("\x03");
    QTest::newRow("ctrl-d") << int(Qt::Key_D) << ctrl << QString() << QByteArray("\x04");
    QTest::newRow("ctrl-z") << int(Qt::Key_Z) << ctrl << QString() << QByteArray("\x1a");
    QTest::newRow("ctrl-space") << int(Qt::Key_Space) << ctrl << QString() << QByteArray(1, '\0');
    QTest::newRow("ctrl-backslash") << int(Qt::Key_Backslash) << ctrl << QString() << QByteArray("\x1c");
    QTest::newRow("alt-x") << int(Qt::Key_X) << alt << "x" << QByteArray("\x1bx");
    QTest::newRow("alt-shift-x") << int(Qt::Key_X) << (alt | shift) << "X" << QByteArray("\x1bX");
    QTest::newRow("alt-enter") << int(Qt::Key_Return) << alt << "\r" << QByteArray("\x1b\r");
    QTest::newRow("shift-space") << int(Qt::Key_Space) << shift << " " << QByteArray("\x1b[32;2u");
    QTest::newRow("keypad-5") << int(Qt::Key_5) << int(Qt::KeypadModifier) << "5" << QByteArray("5");
}

void TstTerminal::keyTranslation()
{
    QFETCH(int, key);
    QFETCH(int, mods);
    QFETCH(QString, text);
    QFETCH(QByteArray, bytes);
    QCOMPARE(typed(key, mods, text), bytes);
}

void TstTerminal::keyTranslationKinds()
{
    QCOMPARE(KeyTranslator::translate(Qt::Key_Shift, Qt::ShiftModifier, QString()).kind, KeyTranslation::None);
    QCOMPARE(KeyTranslator::translate(Qt::Key_Up, 0, QString()).kind, KeyTranslation::Key);
    QCOMPARE(KeyTranslator::translate(Qt::Key_A, 0, QStringLiteral("a")).kind, KeyTranslation::Char);
    QCOMPARE(KeyTranslator::translate(0, 0, QStringLiteral("ab")).kind, KeyTranslation::Text);
    QCOMPARE(KeyTranslator::translate(Qt::Key_B, Qt::ControlModifier, QString()).unichar, uint32_t('b'));
}

static QByteArray osc133(const char *kind)
{
    return QByteArray("\x1b]133;") + kind + "\x07";
}

void TstTerminal::osc133Marks()
{
    TerminalSession s;
    s.resize(40, 10);
    QSignalSpy started(&s, &TerminalSession::commandStarted);
    QSignalSpy finished(&s, &TerminalSession::commandFinished);

    s.feed("first line\r\n");
    s.feed(osc133("A") + "$ " + osc133("B") + "echo hi\r\n" + osc133("C"));
    QCOMPARE(s.marks()->rowCount(), 1);
    QCOMPARE(started.count(), 1);
    QCOMPARE(s.marks()->data(s.marks()->index(0), CommandMarksModel::StateRole).toString(), QStringLiteral("running"));
    QCOMPARE(s.marks()->data(s.marks()->index(0), CommandMarksModel::CommandTextRole).toString(), QStringLiteral("echo hi"));

    s.feed("hi\r\n" + osc133("D;0"));
    QCOMPARE(finished.count(), 1);
    const QVariantMap m = s.marks()->get(0);
    QCOMPARE(m.value("state").toString(), QStringLiteral("ok"));
    QCOMPARE(m.value("exitCode").toInt(), 0);
    QCOMPARE(m.value("line").toInt(), s.firstLine() + s.scrollbackCount() + 1);
    QCOMPARE(m.value("commandLine").toInt(), m.value("line").toInt());
    QCOMPARE(m.value("outputLine").toInt(), m.value("line").toInt() + 1);
    QCOMPARE(m.value("endLine").toInt(), m.value("outputLine").toInt());
    QVERIFY(m.value("durationMs").toLongLong() >= 0);
    QCOMPARE(s.lineText(m.value("outputLine").toInt()), QStringLiteral("hi"));

    // The marks keep their absolute lines when the screen scrolls into the scrollback.
    const int promptLine = m.value("line").toInt();
    for (int i = 0; i < 30; ++i)
        s.feed("filler\r\n");
    QVERIFY(s.scrollbackCount() > 0);
    QCOMPARE(s.marks()->get(0).value("line").toInt(), promptLine);
    QVERIFY(s.lineText(promptLine).startsWith(QStringLiteral("$ echo hi")));
}

void TstTerminal::osc133Failure()
{
    TerminalSession s;
    s.resize(40, 10);
    s.feed(osc133("A") + "$ " + osc133("B") + "false\r\n" + osc133("C") + osc133("D;1"));
    s.feed(osc133("A") + "$ " + osc133("B") + "ls /nope\r\n" + osc133("C") + "err\r\n" + osc133("D;127"));
    QCOMPARE(s.marks()->rowCount(), 2);
    QCOMPARE(s.marks()->get(0).value("state").toString(), QStringLiteral("fail"));
    QCOMPARE(s.marks()->get(0).value("exitCode").toInt(), 1);
    QCOMPARE(s.marks()->get(1).value("state").toString(), QStringLiteral("fail"));
    QCOMPARE(s.marks()->get(1).value("exitCode").toInt(), 127);
    QCOMPARE(s.marks()->get(1).value("commandText").toString(), QStringLiteral("ls /nope"));
    QCOMPARE(s.marks()->rowForLine(s.marks()->get(1).value("line").toInt()), 1);
}

void TstTerminal::osc133IgnoresPromptOnly()
{
    TerminalSession s;
    s.resize(40, 10);
    // A prompt that is redrawn, or a D without a command, creates no mark.
    s.feed(osc133("A") + "$ " + osc133("B") + osc133("D;0"));
    s.feed(osc133("A") + "$ " + osc133("B"));
    QCOMPARE(s.marks()->rowCount(), 0);
}

void TstTerminal::osc7Cwd()
{
    TerminalSession s;
    QSignalSpy spy(&s, &TerminalSession::cwdChanged);
    s.feed("\x1b]7;file://host/home/me/My%20Dir/%C3%A9\x07");
    QCOMPARE(s.cwd(), QString::fromUtf8("/home/me/My Dir/é"));
    QCOMPARE(spy.count(), 1);
    s.feed("\x1b]7;file:///tmp\x1b\\");
    QCOMPARE(s.cwd(), QStringLiteral("/tmp"));
}

void TstTerminal::osc2Title()
{
    TerminalSession s;
    s.feed("\x1b]2;hello world\x07");
    QCOMPARE(s.title(), QStringLiteral("hello world"));
    s.feed("\x1b]0;second\x07");
    QCOMPARE(s.title(), QStringLiteral("second"));
}

void TstTerminal::scrollbackBound()
{
    TerminalSession s;
    s.setScrollbackLimit(50);
    s.resize(20, 10);
    for (int i = 0; i < 500; ++i)
        s.feed(QByteArray("line ") + QByteArray::number(i) + "\r\n");
    QCOMPARE(s.scrollbackCount(), 50);
    QVERIFY(s.firstLine() > 0);
    s.setScrollbackLimit(10);
    QCOMPARE(s.scrollbackCount(), 10);

    TerminalSession d;
    QCOMPARE(d.scrollbackLimit(), 10000);
}

void TstTerminal::scrollbackAbsoluteLines()
{
    TerminalSession s;
    s.setScrollbackLimit(20);
    s.resize(20, 5);
    for (int i = 0; i < 100; ++i)
        s.feed(QByteArray("n") + QByteArray::number(i) + "\r\n");
    // Absolute line numbers stay attached to their text after old lines were dropped.
    for (int l = s.firstLine(); l < s.firstLine() + 10; ++l)
        QCOMPARE(s.lineText(l), QStringLiteral("n%1").arg(l));
}

void TstTerminal::resizeKeepsContent()
{
    TerminalSession s;
    s.resize(30, 10);
    for (int i = 0; i < 25; ++i)
        s.feed(QByteArray("row") + QByteArray::number(i) + "\r\n");
    const int base = s.firstLine();
    auto check = [&](int count) {
        for (int i = 0; i < count; ++i)
            QCOMPARE(s.lineText(base + i), QStringLiteral("row%1").arg(i));
    };
    check(25);
    s.resize(30, 4);
    QCOMPARE(s.rows(), 4);
    check(25);
    s.resize(30, 20);
    QCOMPARE(s.rows(), 20);
    check(25);
    s.resize(12, 20);
    QCOMPARE(s.cols(), 12);
    check(25);
}

void TstTerminal::wideChars()
{
    TerminalSession s;
    s.resize(20, 4);
    s.feed(QString::fromUtf8("日本a😀b").toUtf8());
    QVector<TermCell> cells;
    QVERIFY(s.fetchLine(s.screenTopLine(), cells));
    QCOMPARE(cells[0].width, quint8(2));
    QCOMPARE(cells[0].ch, quint32(0x65e5));
    QCOMPARE(cells[1].width, quint8(0));
    QCOMPARE(cells[2].width, quint8(2));
    QCOMPARE(cells[4].ch, quint32('a'));
    QCOMPARE(cells[4].width, quint8(1));
    QCOMPARE(cells[5].width, quint8(2));
    QCOMPARE(cells[5].ch, quint32(0x1f600));
    QCOMPARE(cells[7].ch, quint32('b'));
    QCOMPARE(s.cursorColumn(), 8);
    QCOMPARE(s.lineText(s.screenTopLine()), QString::fromUtf8("日本a😀b"));
}

void TstTerminal::combiningCluster()
{
    TerminalSession s;
    s.resize(20, 4);
    s.feed(QString::fromUtf8("e\u0301x").toUtf8());
    QVector<TermCell> cells;
    QVERIFY(s.fetchLine(s.screenTopLine(), cells));
    QVERIFY(cells[0].ch & TermCell::ClusterFlag);
    QCOMPARE(s.clusterText(cells[0].ch), QString::fromUtf8("e\u0301"));
    QCOMPARE(cells[1].ch, quint32('x'));
}

void TstTerminal::colors()
{
    TerminalSession s;
    s.resize(20, 4);
    s.feed("\x1b[1;31mR\x1b[0m\x1b[38;5;200mI\x1b[38;2;10;20;30mT\x1b[44mB\x1b[0m\x1b[4;7mU\x1b[0m");
    QVector<TermCell> c;
    QVERIFY(s.fetchLine(s.screenTopLine(), c));
    QCOMPARE(c[0].fg, TermCell::ColorIndexed | 1);
    QVERIFY(c[0].attrs & TermCell::Bold);
    QCOMPARE(c[1].fg, TermCell::ColorIndexed | 200);
    QCOMPARE(c[2].fg, TermCell::ColorRgb | (10u << 16) | (20u << 8) | 30u);
    QCOMPARE(c[3].bg, TermCell::ColorIndexed | 4);
    QVERIFY(c[4].attrs & TermCell::Reverse);
    QCOMPARE((c[4].attrs & TermCell::UnderlineMask) >> 2, 1);
    QCOMPARE(c[5].fg, quint32(0));

    // The palette resolves indexed colours, including the cube and the grey ramp.
    TerminalPalette p;
    QCOMPARE(p.indexed(16), qRgb(0, 0, 0));
    QCOMPARE(p.indexed(231), qRgb(255, 255, 255));
    QCOMPARE(p.indexed(232), qRgb(8, 8, 8));
    p.setColor(1, QColor("#123456"));
    QCOMPARE(p.indexed(1), qRgb(0x12, 0x34, 0x56));
}

void TstTerminal::cursorShapeAndAltScreen()
{
    TerminalSession s;
    s.resize(20, 4);
    QCOMPARE(s.cursorShape(), TerminalSession::Block);
    s.feed("\x1b[5 q");
    QCOMPARE(s.cursorShape(), TerminalSession::Bar);
    s.feed("\x1b[4 q");
    QCOMPARE(s.cursorShape(), TerminalSession::Underline);
    s.feed("\x1b[2 q");
    QCOMPARE(s.cursorShape(), TerminalSession::Block);
    s.feed("\x1b[?25l");
    QVERIFY(!s.cursorVisible());
    s.feed("\x1b[?25h");
    QVERIFY(s.cursorVisible());
    QVERIFY(!s.altScreen());
    s.feed("\x1b[?1049h");
    QVERIFY(s.altScreen());
    s.feed("\x1b[?1049l");
    QVERIFY(!s.altScreen());
    QCOMPARE(s.mouseMode(), TerminalSession::MouseNone);
    s.feed("\x1b[?1002h");
    QCOMPARE(s.mouseMode(), TerminalSession::MouseDrag);
    s.mouseMove(2, 3, 0);
    s.takeOutput();
    s.mouseButton(1, true, 0);
    QVERIFY(s.takeOutput().startsWith("\x1b[M"));
}

void TstTerminal::bracketedPaste()
{
    TerminalSession s;
    s.paste(QStringLiteral("a\nb"));
    QCOMPARE(s.takeOutput(), QByteArray("a\rb"));
    s.feed("\x1b[?2004h");
    s.paste(QStringLiteral("a\nb\x1b[201~"));
    QCOMPARE(s.takeOutput(), QByteArray("\x1b[200~a\rb[201~\x1b[201~"));
    s.sendText(QStringLiteral("typed"));
    QCOMPARE(s.takeOutput(), QByteArray("typed"));
}

void TstTerminal::viewportScroll()
{
    TerminalSession s;
    s.resize(20, 5);
    for (int i = 0; i < 30; ++i)
        s.feed(QByteArray("v") + QByteArray::number(i) + "\r\n");
    QCOMPARE(s.viewOffset(), 0);
    const int bottom = s.viewTopLine();
    s.scrollViewport(3);
    QCOMPARE(s.viewOffset(), 3);
    QCOMPARE(s.viewTopLine(), bottom - 3);
    // New output keeps the view on the same lines while scrolled back.
    s.feed("more\r\n");
    QCOMPARE(s.viewTopLine(), bottom - 3);
    s.scrollViewport(100000);
    QCOMPARE(s.viewOffset(), s.scrollbackCount());
    s.sendKey(Qt::Key_A, 0, QStringLiteral("a"));
    QCOMPARE(s.viewOffset(), 0);
}

void TstTerminal::ptyDeliversOutputAndStatus()
{
    Pty pty;
    QByteArray out;
    connect(&pty, &Pty::dataReceived, this, [&](const QByteArray &d) { out += d; });
    QSignalSpy exited(&pty, &Pty::childExited);
    QVERIFY(pty.start(QStringLiteral("/bin/sh"), {QStringLiteral("-c"), QStringLiteral("printf hello")}, QString(), 80, 24));
    QVERIFY(exited.wait(5000));
    QCOMPARE(exited.first().first().toInt(), 0);
    QVERIFY(out.contains("hello"));
}

void TstTerminal::ptyExitCode()
{
    Pty pty;
    QSignalSpy exited(&pty, &Pty::childExited);
    QVERIFY(pty.start(QStringLiteral("/bin/sh"), {QStringLiteral("-c"), QStringLiteral("exit 7")}, QStringLiteral("/tmp"), 80, 24));
    QVERIFY(exited.wait(5000));
    QCOMPARE(exited.first().first().toInt(), 7);
}

void TstTerminal::ptyNoZombie()
{
    qint64 pid = 0;
    {
        Pty pty;
        QVERIFY(pty.start(QStringLiteral("/bin/sh"), {QStringLiteral("-c"), QStringLiteral("sleep 30")}, QString(), 80, 24));
        pid = pty.pid();
        QVERIFY(pty.isRunning());
        pty.resize(100, 30, 800, 600);
    }
    // The destructor already reaped the child, so there is nothing left to wait for.
    int status = 0;
    QCOMPARE(waitpid(static_cast<pid_t>(pid), &status, WNOHANG), -1);
    QCOMPARE(errno, ECHILD);
}

void TstTerminal::sessionWithPty()
{
    TerminalSession s;
    s.resize(40, 6);
    s.setProperty("program", QStringLiteral("/bin/sh"));
    s.setProperty("arguments", QStringList{QStringLiteral("-c"), QStringLiteral("printf '\\033[1;32mok\\033[0m\\n'; printf '\\033]7;file:///tmp\\007'")});
    QSignalSpy exited(&s, &TerminalSession::childExited);
    QVERIFY(s.start());
    QVERIFY(exited.wait(5000));
    QCOMPARE(s.lineText(s.screenTopLine()), QStringLiteral("ok"));
    QCOMPARE(s.cwd(), QStringLiteral("/tmp"));
    QVERIFY(!s.running());
}

QTEST_GUILESS_MAIN(TstTerminal)
#include "tst_terminal.moc"
