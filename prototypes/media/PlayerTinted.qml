import QtQuick
import QtQuick.Layouts
import Opal.Material
import "Fixtures.js" as F
// Player v1, "Tinted": the whole player takes a scheme from the album art; art in a soft card, left-aligned title, big controls.
Rectangle {
    id: root
    property string art: F.albums[2].img
    property string title: "Feature Presentation"
    property string album: "DELTARUNE Chapters 3+4"
    property color seed: "#e0785a"
    readonly property var s: Palette.schemeFor(seed, Theme.light)
    color: s.surface_container_low
    ColumnLayout {
        anchors.fill: parent; anchors.margins: 20; anchors.topMargin: 12; spacing: 16
        RowLayout { Layout.fillWidth: true
            Glyph { name: "keyboard_arrow_down"; color: root.s.on_surface; font.pixelSize: 28 }
            MText { Layout.fillWidth: true; horizontalAlignment: Text.AlignHCenter; text: "Playing from " + root.album; role: "label"; color: root.s.on_surface_variant }
            Glyph { name: "more_vert"; color: root.s.on_surface } }
        Cover { Layout.fillWidth: true; Layout.preferredHeight: width; r: 32; source: root.art }
        RowLayout { Layout.fillWidth: true; Layout.topMargin: 4
            ColumnLayout { Layout.fillWidth: true; spacing: 2
                MText { Layout.fillWidth: true; text: root.title; role: "headline"; color: root.s.on_surface }
                MText { text: "Toby Fox"; role: "body"; color: root.s.on_surface_variant } }
            Rectangle { implicitWidth: 48; implicitHeight: 48; radius: 16; color: root.s.tertiary_container
                Glyph { anchors.centerIn: parent; name: "favorite"; fill: 1; color: root.s.on_tertiary_container } } }
        TintedTransport { Layout.fillWidth: true; s: root.s; size: 72 }
        RowLayout { Layout.fillWidth: true
            Rectangle { implicitHeight: 40; implicitWidth: o.implicitWidth + 28; radius: 12; color: root.s.surface_container_highest
                RowLayout { id: o; anchors.centerIn: parent; spacing: 8; Glyph { name: "smartphone"; font.pixelSize: 18; color: root.s.primary } MText { text: "This phone"; role: "label"; color: root.s.on_surface } } }
            Item { Layout.fillWidth: true }
            Glyph { name: "lyrics"; color: root.s.on_surface_variant; Layout.rightMargin: 12 }
            Glyph { name: "queue_music"; color: root.s.on_surface_variant } }
        Item { Layout.fillHeight: true }
    }
}
