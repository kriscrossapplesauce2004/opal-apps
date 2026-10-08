import QtQuick
import Opal.TerminalUi
// Render check of the real Terminal interface: a split with real commands in the user's shell.
TerminalView {
    id: v
    Timer { interval: 900; running: true; onTriggered: { v.runInFocused("ls --color=always ~/Pictures/Wallpapers | head -8"); v.split("h"); } }
    Timer { interval: 1700; running: true; onTriggered: { v.runInFocused(Qt.application.arguments.length > 99 ? "" : "fastfetch --pipe false 2>/dev/null || uname -a"); v.split("v"); } }
    Timer { interval: 2500; running: true; onTriggered: v.runInFocused("false") }
}
