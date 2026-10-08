import QtQuick
import QtQuick.Layouts
import Opal.Material
import "Fixtures.js" as F
// Player v3, "Stack": the cover is a card with the next track peeking behind it; swipe it to skip. Tinted from the art.
Rectangle {
    id: root
    property color seed: "#8a6bd6"
    readonly property var s: Palette.schemeFor(seed, Theme.light)
    color: s.surface_container_lowest
    ColumnLayout {
        anchors.fill: parent; anchors.margins: 20; anchors.topMargin: 12; spacing: 16
        RowLayout { Layout.fillWidth: true
            Glyph { name: "keyboard_arrow_down"; color: root.s.on_surface; font.pixelSize: 28 }
            Item { Layout.fillWidth: true }
            Rectangle { implicitHeight: 34; implicitWidth: sr.implicitWidth + 24; radius: 12; color: root.s.secondary_container
                RowLayout { id: sr; anchors.centerIn: parent; spacing: 6; Glyph { name: "auto_stories"; visible: false } MText { text: "Queue · 8"; role: "label"; color: root.s.on_secondary_container } } }
            Item { Layout.fillWidth: true }
            Glyph { name: "more_vert"; color: root.s.on_surface } }
        Item {
            Layout.fillWidth: true; Layout.preferredHeight: width * .95
            Cover { x: parent.width * .2; y: parent.width * .06; width: parent.width * .78; height: width; r: 30; source: F.albums[3].img; opacity: .5; rotation: 5 }
            Cover { x: parent.width * .12; y: parent.width * .03; width: parent.width * .78; height: width; r: 30; source: F.albums[1].img; opacity: .75; rotation: 2.5 }
            Cover { x: 0; y: 0; width: parent.width * .84; height: width; r: 32; source: F.albums[0].img }
        }
        ColumnLayout { Layout.fillWidth: true; spacing: 2
            MText { Layout.fillWidth: true; text: "Field of Hopes and Dreams"; role: "headline"; color: root.s.on_surface }
            MText { text: "Toby Fox · next: WELCOME TO THE CITY"; role: "body"; color: root.s.on_surface_variant } }
        TintedTransport { Layout.fillWidth: true; s: root.s; size: 72 }
        Item { Layout.fillHeight: true }
    }
}
