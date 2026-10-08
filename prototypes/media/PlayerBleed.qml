import QtQuick
import QtQuick.Layouts
import Opal.Material
import "Fixtures.js" as F
// Player v2, "Full bleed": the art runs edge to edge and fades into an art-tinted background; title sits on the fade.
Rectangle {
    id: root
    property string art: F.albums[1].img
    property color seed: "#2fc4e0"
    readonly property var s: Palette.schemeFor(seed, Theme.light)
    color: s.surface
    Image { anchors.left: parent.left; anchors.right: parent.right; height: width * 1.1; source: root.art; fillMode: Image.PreserveAspectCrop; sourceSize: Qt.size(824, 900) }
    Rectangle { anchors.left: parent.left; anchors.right: parent.right; y: parent.width * .55; height: parent.width * .56
        gradient: Gradient { GradientStop { position: 0; color: "transparent" } GradientStop { position: 1; color: root.s.surface } } }
    RowLayout { anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top; anchors.margins: 16
        Rectangle { implicitWidth: 40; implicitHeight: 40; radius: 20; color: Qt.rgba(0, 0, 0, .35); Glyph { anchors.centerIn: parent; name: "keyboard_arrow_down"; color: "#ffffff" } }
        Item { Layout.fillWidth: true }
        Rectangle { implicitWidth: 40; implicitHeight: 40; radius: 20; color: Qt.rgba(0, 0, 0, .35); Glyph { anchors.centerIn: parent; name: "more_vert"; color: "#ffffff" } } }
    ColumnLayout {
        anchors.left: parent.left; anchors.right: parent.right; anchors.bottom: parent.bottom; anchors.margins: 20; anchors.bottomMargin: 28; spacing: 14
        MText { text: "DELTARUNE Chapter 2"; role: "label"; color: root.s.primary }
        MText { Layout.fillWidth: true; text: "WELCOME TO THE CITY"; font.pixelSize: 34; font.weight: Font.Black; font.variableAxes: ({"wdth": 115, "ROND": 60}); wrapMode: Text.WordWrap; color: root.s.on_surface }
        MText { text: "Toby Fox"; role: "title"; color: root.s.on_surface_variant }
        TintedTransport { Layout.fillWidth: true; s: root.s; size: 70; pos: "2:04"; len: "4:47"; p: .43 }
        RowLayout { Layout.fillWidth: true; spacing: 8
            Repeater { model: [{i: "speaker_group", t: "Living room"}, {i: "lyrics", t: "Lyrics"}, {i: "queue_music", t: "Queue"}]
                Rectangle { required property var modelData; Layout.fillWidth: true; implicitHeight: 44; radius: 14; color: root.s.surface_container_high
                    RowLayout { anchors.centerIn: parent; spacing: 6; Glyph { name: modelData.i; font.pixelSize: 18; color: root.s.primary } MText { text: modelData.t; role: "label"; color: root.s.on_surface } } } } }
    }
}
