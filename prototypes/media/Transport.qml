import QtQuick
import QtQuick.Layouts
import Opal.Material
// Seek bar with wavy progress and the transport controls.
ColumnLayout {
    id: t
    property real size: 64
    property var now
    spacing: 8
    WavyProgress { Layout.fillWidth: true; value: t.now.p; animate: false; phase: .8; amplitude: 3; thickness: 5; wavelength: 30 }
    RowLayout { Layout.fillWidth: true
        MText { text: t.now.pos; role: "labelS"; color: Theme.subtext }
        Item { Layout.fillWidth: true }
        MText { text: t.now.len; role: "labelS"; color: Theme.subtext } }
    RowLayout { Layout.alignment: Qt.AlignHCenter; spacing: t.size * .22
        Glyph { name: "shuffle"; font.pixelSize: t.size * .36; color: Theme.primary }
        Rectangle { implicitWidth: t.size * .8; implicitHeight: t.size * .8; radius: width / 2; color: Theme.secondaryContainer
            Glyph { anchors.centerIn: parent; name: "skip_previous"; fill: 1; font.pixelSize: t.size * .42; color: Theme.onSecondaryContainer } }
        PlayButton { size: t.size }
        Rectangle { implicitWidth: t.size * .8; implicitHeight: t.size * .8; radius: width / 2; color: Theme.secondaryContainer
            Glyph { anchors.centerIn: parent; name: "skip_next"; fill: 1; font.pixelSize: t.size * .42; color: Theme.onSecondaryContainer } }
        Glyph { name: "repeat"; font.pixelSize: t.size * .36; color: Theme.subtext } }
}
