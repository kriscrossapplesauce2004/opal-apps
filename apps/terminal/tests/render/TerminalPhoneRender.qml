import QtQuick
import Opal.TerminalUi
// Render check: phone layout with the key row.
TerminalView { id: v; Timer { interval: 700; running: true; onTriggered: v.runInFocused("ls --color=always /usr") } }
