import QtQuick
import QtQuick.Effects
import QtQuick.Layouts
import Opal.Material
import "Fixtures.js" as F
// Immersive round 3: the cover fills the height on the left; title, seek, transport, output and the queue share one column on the right. Calm art tint.
Rectangle {
    id: root
    property string art: F.albums[2].img
    property color seed: "#e0785a"
    readonly property var s: Palette.schemeFor(seed, Theme.light, .5)
    color: s.surface
    Image { id: bg; anchors.fill: parent; source: root.art; fillMode: Image.PreserveAspectCrop; visible: false; sourceSize: Qt.size(720, 450) }
    MultiEffect { anchors.fill: parent; source: bg; blurEnabled: true; blur: 1; blurMax: 64; brightness: -.45; saturation: -.2; opacity: .55 }
    RowLayout {
        anchors.fill: parent; anchors.margins: 40; spacing: 40
        Cover { Layout.fillHeight: true; Layout.preferredWidth: height; r: 36; source: root.art }
        ColumnLayout {
            Layout.fillWidth: true; Layout.fillHeight: true; spacing: 14
            RowLayout { Layout.fillWidth: true
                MText { text: "Playing from DELTARUNE Chapters 3+4"; role: "label"; color: root.s.primary; Layout.fillWidth: true }
                Rectangle { implicitWidth: 40; implicitHeight: 40; radius: 20; color: root.s.surface_container_high; Glyph { anchors.centerIn: parent; name: "close_fullscreen"; color: root.s.on_surface } } }
            MText { Layout.fillWidth: true; text: "Feature Presentation"; font.pixelSize: 54; font.weight: Font.Black; font.variableAxes: ({"wdth": 115, "ROND": 60}); color: root.s.on_surface; wrapMode: Text.WordWrap }
            MText { text: "Toby Fox"; role: "title"; color: root.s.on_surface_variant }
            TintedTransport { Layout.fillWidth: true; Layout.topMargin: 8; s: root.s; size: 64; pos: "0:41"; len: "2:58"; p: .23 }
            RowLayout { Layout.fillWidth: true; spacing: 12
                Glyph { name: "volume_up"; color: root.s.on_surface_variant }
                ExpressiveSlider { Layout.preferredWidth: 200; value: 60; compact: true; showIcon: false; activeColor: root.s.primary; inactiveColor: root.s.secondary_container }
                Item { Layout.fillWidth: true }
                SplitPill { Layout.preferredWidth: 420; s: root.s; h: 44; items: [{i: "speaker_group", t: "Living room"}, {i: "lyrics", t: "Lyrics"}, {i: "queue_music", t: "Queue"}] } }
            // Queue fills the rest of the column.
            Rectangle {
                Layout.fillWidth: true; Layout.fillHeight: true; Layout.topMargin: 6; radius: 28; color: Theme.alpha(root.s.surface_container, .85)
                ColumnLayout { anchors.fill: parent; anchors.margins: 14; spacing: 2
                    MText { text: "Up next"; role: "titleS"; color: root.s.on_surface; Layout.leftMargin: 8; Layout.bottomMargin: 6 }
                    Repeater { model: [{n: 3, t: "And Now For Today's Sponsors…!", d: "0:32"}, {n: 4, t: "MIKE, the BOARD, please!", d: "0:47"}, {n: 5, t: "Sandy Board", d: "2:10"}, {n: 6, t: "Raft Ride", d: "1:44"}, {n: 7, t: "Ice Palace", d: "2:31"}, {n: 8, t: "Hall of Fame", d: "1:12"}]
                        RowLayout { required property var modelData; Layout.fillWidth: true; Layout.leftMargin: 8; Layout.rightMargin: 8; implicitHeight: 40; spacing: 14
                            MText { text: modelData.n; role: "labelS"; color: root.s.on_surface_variant; Layout.preferredWidth: 18 }
                            MText { Layout.fillWidth: true; text: modelData.t; role: "body"; color: root.s.on_surface }
                            MText { text: modelData.d; role: "labelS"; color: root.s.on_surface_variant } } }
                    Item { Layout.fillHeight: true } } }
        }
    }
}
