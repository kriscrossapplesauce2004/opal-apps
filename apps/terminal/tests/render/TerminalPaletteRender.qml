import QtQuick
import Opal.TerminalUi
// Render check: the command palette.
TerminalView { id: v; Timer { interval: 700; running: true; onTriggered: { v.runInFocused("echo palette"); v.commandPalette.open = true; } } }
