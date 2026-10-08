import QtQuick
import QtQuick.Layouts
import Opal.Material
import "Fixtures.js" as F
// Photo viewer on a phone: edge-to-edge photo, a slim top bar and the bottom action row.
Rectangle {
    color: "#000000"
    readonly property var cur: F.photos.filter(p => p.n.indexOf("dr5-keyart") === 0)[0] || F.photos[0]
    RoundedImage { anchors.centerIn: parent; width: parent.width; height: width * cur.h / cur.w; radius: 0; source: cur.u; sourceSize: Qt.size(820, 460) }
    RowLayout { anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top; anchors.margins: 12
        Glyph { name: "arrow_back"; color: "#ffffff" }
        ColumnLayout { Layout.leftMargin: 12; spacing: 0; MText { text: "Today"; role: "titleS"; color: "#ffffff" } MText { text: "13:58 · Wallpapers"; role: "labelS"; color: Qt.rgba(1, 1, 1, .7) } }
        Item { Layout.fillWidth: true }
        Glyph { name: "cast"; color: "#ffffff"; Layout.rightMargin: 16 }
        Glyph { name: "more_vert"; color: "#ffffff" } }
    Rectangle { anchors.horizontalCenter: parent.horizontalCenter; anchors.bottom: parent.bottom; anchors.bottomMargin: 26; implicitHeight: 64; implicitWidth: ar.implicitWidth + 16; radius: 32; color: Theme.alpha(Theme.surfaceHigh, .92)
        RowLayout { id: ar; anchors.centerIn: parent; spacing: 4
            Repeater { model: [{i: "share"}, {i: "edit"}, {i: "favorite", on: true}, {i: "info"}, {i: "delete"}]
                Rectangle { required property var modelData; implicitWidth: 56; implicitHeight: 48; radius: 24; color: modelData.on ? Theme.secondaryContainer : "transparent"
                    Glyph { anchors.centerIn: parent; name: modelData.i; fill: modelData.on ? 1 : 0; color: modelData.on ? Theme.onSecondaryContainer : Theme.text } } } } }
}
