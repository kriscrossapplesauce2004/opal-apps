import QtQuick
import Opal.Material
import Opal.Terminal

// Render check: a real shell command drawn by the glyph-atlas renderer inside a rounded pane.
Rectangle {
    color: Theme.base
    TerminalSession {
        id: session
        program: "/bin/sh"
        arguments: ["-c", "printf '\\033[1;32m❯\\033[0m ls\\n'; ls --color=always /usr; sleep 5"]
        Component.onCompleted: start()
    }
    Rectangle {
        anchors.fill: parent
        anchors.margins: 8
        radius: 20
        color: Theme.surfaceLowest
        clip: true
        TerminalItem {
            anchors.fill: parent
            session: session
            drawBackground: false
            fontFamily: "CaskaydiaCove Nerd Font Mono"
            fontPixelSize: 14
        }
    }
}
