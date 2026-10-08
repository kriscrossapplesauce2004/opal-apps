import QtQuick
import QtQuick.Layouts
import Opal.Material
import "../terminal/Fixtures.js" as T
import "../terminal" as Term
// A phone screen with an app above the keyboard. app: chat, terminal or notes.
Rectangle {
    id: scene
    property string app: "chat"
    property alias kbd: kb
    color: Theme.base
    ColumnLayout {
        anchors.fill: parent; spacing: 0
        Item {
            Layout.fillWidth: true; Layout.fillHeight: true
            // A plain text field, so the keyboard has something to type into.
            ColumnLayout { visible: scene.app === "chat"; anchors.fill: parent; anchors.margins: 16; spacing: 12
                MText { text: "Notes"; role: "headline" }
                Rectangle { Layout.fillWidth: true; Layout.fillHeight: true; radius: 24; color: Theme.surface
                    MText { anchors.fill: parent; anchors.margins: 18; wrapMode: Text.Wrap; font.pixelSize: 17; text: "Borealis should feel like part of the phone, not a bolted-on keyboard. Glide typing, the clipboard and a real terminal mode, all in one deck. hel" }
                    Rectangle { x: 120; y: 104; width: 2; height: 24; color: Theme.primary } } }
            Term.TermView { visible: scene.app === "terminal"; anchors.fill: parent; anchors.margins: 6; lines: T.build.slice(6, 18); fontSize: 12; padding: 12; radius: 22; marks: ({0: "ok", 5: "fail", 11: "run"}) }
        }
        Kbd { id: kb; Layout.fillWidth: true }
    }
}
