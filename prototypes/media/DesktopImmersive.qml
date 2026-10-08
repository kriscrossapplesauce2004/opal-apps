import QtQuick
import QtQuick.Effects
import QtQuick.Layouts
import Opal.Material
import "Fixtures.js" as F
// Media direction B, "Immersive": the art fills the window, blurred; big cover, the queue as a side card, controls along the bottom.
Rectangle {
    color: "#000000"
    Image { id: bg; anchors.fill: parent; source: F.now.img; fillMode: Image.PreserveAspectCrop; visible: false; sourceSize: Qt.size(800, 800) }
    MultiEffect { anchors.fill: parent; source: bg; blurEnabled: true; blur: 1; blurMax: 64; brightness: -.35; saturation: .2 }
    RowLayout {
        anchors.fill: parent; anchors.margins: 48; anchors.bottomMargin: 150; spacing: 48
        ColumnLayout { Layout.fillWidth: true; Layout.fillHeight: true; spacing: 18
            Cover { Layout.preferredWidth: 480; Layout.preferredHeight: 480; r: 40; source: F.now.img }
            MText { text: F.now.t; role: "displayS"; color: "#ffffff" }
            MText { text: F.now.a + " · " + F.now.al; role: "title"; color: Qt.rgba(1, 1, 1, .75) } }
        Rectangle { Layout.preferredWidth: 420; Layout.fillHeight: true; radius: 32; color: Theme.alpha(Theme.surface, .78)
            ColumnLayout { anchors.fill: parent; anchors.margins: 20; spacing: 4
                RowLayout { Layout.fillWidth: true; Layout.bottomMargin: 8
                    SegmentedButtons { current: "queue"; options: [{id: "queue", name: "Queue"}, {id: "lyrics", name: "Lyrics"}, {id: "about", name: "About"}] } }
                Repeater { model: F.queue
                    ListRow { required property var modelData; Layout.fillWidth: true; title: modelData.t; trailing: modelData.d; selected: !!modelData.cur; icon: modelData.cur ? "graphic_eq" : "music_note"; implicitHeight: 48 } }
                Item { Layout.fillHeight: true } } }
    }
    Rectangle {
        anchors.left: parent.left; anchors.right: parent.right; anchors.bottom: parent.bottom; anchors.margins: 24; height: 110; radius: 40; color: Theme.alpha(Theme.surfaceHigh, .9)
        RowLayout { anchors.fill: parent; anchors.leftMargin: 28; anchors.rightMargin: 28; spacing: 28
            Transport { Layout.preferredWidth: 520; now: F.now; size: 52 }
            Item { Layout.fillWidth: true }
            Glyph { name: "volume_up"; color: Theme.text }
            ExpressiveSlider { Layout.preferredWidth: 180; value: 60; compact: true; showIcon: false }
            Chip { text: "Living room speaker"; icon: "speaker_group"; glass: false }
            MButton { icon: "close_fullscreen"; compact: true } }
    }
}
