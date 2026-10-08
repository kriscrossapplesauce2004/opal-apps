import QtQuick
import Opal.Material
import Opal.Terminal

// Minimal shell around the terminal engine: one rounded pane, as in the approved "Clean" prototype.
AppWindow {
    id: window
    seed: "#3e8e7e"
    allowCouch: false
    title: session.title !== "" ? session.title : qsTr("Terminal")

    function tint(hex) { return Theme.mix(hex, Theme.primary, .06); }

    TerminalSession {
        id: session
        onChildExited: Qt.quit()
        Component.onCompleted: start()
    }

    // The ANSI colours follow the Opal palette, like the prototype does.
    Binding { target: session.palette; property: "background"; value: Theme.surfaceLowest }
    Binding { target: session.palette; property: "foreground"; value: Theme.text }
    Binding { target: session.palette; property: "cursor"; value: Theme.primary }
    Binding { target: session.palette; property: "cursorText"; value: Theme.primaryInk }
    Binding { target: session.palette; property: "selection"; value: Theme.alpha(Theme.primary, .32) }
    Binding {
        target: session.palette
        property: "ansi"
        value: Theme.light
            ? [window.tint("#3c4043"), window.tint("#c5221f"), window.tint("#188038"), window.tint("#b06000"), window.tint("#1967d2"), window.tint("#9334e6"), window.tint("#007b83"), window.tint("#5f6368"),
               window.tint("#5f6368"), window.tint("#d93025"), window.tint("#1e8e3e"), window.tint("#c26401"), window.tint("#1a73e8"), window.tint("#a142f4"), window.tint("#12a4af"), window.tint("#202124")]
            : [window.tint("#2b2f38"), window.tint("#f28b82"), window.tint("#81c995"), window.tint("#fdd663"), window.tint("#8ab4f8"), window.tint("#c58af9"), window.tint("#78d9ec"), window.tint("#d7dae0"),
               window.tint("#5b6270"), window.tint("#f6a8a1"), window.tint("#a3dbb1"), window.tint("#fee391"), window.tint("#abc9fa"), window.tint("#d6aefb"), window.tint("#a2e6f4"), window.tint("#ffffff")]
    }

    Rectangle {
        anchors.fill: parent
        anchors.margins: 8
        radius: 20
        color: Qt.rgba(Theme.surfaceLowest.r, Theme.surfaceLowest.g, Theme.surfaceLowest.b, term.backgroundAlpha)

        TerminalItem {
            id: term
            anchors.fill: parent
            session: session
            drawBackground: false
            focus: true
            fontFamily: "CaskaydiaCove Nerd Font Mono"
            fontPixelSize: 14
            lineHeightFactor: 1.2
            Accessible.role: Accessible.Terminal
            Accessible.name: qsTr("Terminal")
        }
    }
}
