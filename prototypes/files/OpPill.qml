import QtQuick
import QtQuick.Layouts
import Opal.Material
// A running file operation as a pill: wavy progress, what it is doing, and the time left. It morphs into a sheet with every operation.
Rectangle {
    id: pill
    property string title: "Copying 3 items to Samsung T7"
    property string detail: "1.4 of 3.2 GB · 40 s left"
    property real progress: .44
    implicitHeight: Theme.couch ? 72 : 56
    implicitWidth: 360
    radius: height / 2
    color: Theme.inverseSurface
    RowLayout {
        anchors.fill: parent; anchors.leftMargin: 8; anchors.rightMargin: 10; spacing: 12
        Rectangle { Layout.preferredWidth: pill.height - 16; Layout.preferredHeight: pill.height - 16; radius: width / 2; color: Theme.inversePrimary
            Glyph { anchors.centerIn: parent; name: "content_copy"; font.pixelSize: Theme.px(20); color: Theme.inverseSurface } }
        ColumnLayout { Layout.fillWidth: true; spacing: 3
            MText { Layout.fillWidth: true; text: pill.title; role: "label"; font.weight: Font.DemiBold; color: Theme.inverseInk }
            RowLayout { Layout.fillWidth: true; spacing: 10
                WavyProgress { Layout.fillWidth: true; value: pill.progress; animate: false; phase: 1.2; amplitude: 2.2; wavelength: 18; thickness: 3 }
                MText { text: pill.detail; role: "labelS"; color: Theme.alpha(Theme.inverseInk, .75) } } }
        Glyph { name: "close"; font.pixelSize: Theme.px(18); color: Theme.inverseInk }
    }
}
