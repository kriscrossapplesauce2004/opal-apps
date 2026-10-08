import QtQuick
import Opal.TerminalUi
// Render check: Appearance panel open with a built-in theme, the sessions sidebar, and a split.
TerminalView {
    id: v; appearance: true; sidebar: true
    Timer { interval: 700; running: true; onTriggered: { TerminalConfig.set("theme", "name", "catppuccin-mocha"); TerminalConfig.set("cursor", "trail", true); v.runInFocused("ls --color=always /usr | head -6"); v.split("v"); } }
}
