import QtQuick
import QtQuick.Layouts
import Opal.Material
// Mini player pill. It is the same surface that grows into the full player.
Rectangle {
    id: m
    property var now
    implicitHeight: 64
    radius: 32
    color: Theme.secondaryContainer
    RowLayout {
        anchors.fill: parent; anchors.leftMargin: 8; anchors.rightMargin: 8; spacing: 12
        Cover { Layout.preferredWidth: 48; Layout.preferredHeight: 48; r: 24; source: m.now.img }
        ColumnLayout { Layout.fillWidth: true; spacing: 0
            MText { Layout.fillWidth: true; text: m.now.t; role: "titleS"; color: Theme.onSecondaryContainer }
            MText { Layout.fillWidth: true; text: m.now.a; role: "labelS"; color: Theme.alpha(Theme.onSecondaryContainer, .75) } }
        Glyph { name: "devices"; font.pixelSize: 20; color: Theme.onSecondaryContainer }
        PlayButton { size: 44 }
    }
    // Progress runs along the bottom edge of the pill.
    Rectangle { x: 32; anchors.bottom: parent.bottom; anchors.bottomMargin: 3; height: 3; radius: 2; width: (parent.width - 64) * m.now.p; color: Theme.primary }
}
