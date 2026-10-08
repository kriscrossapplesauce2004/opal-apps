import QtQuick
import QtQuick.Layouts
import Opal.Material
// M3 Expressive flexible navigation bar for compact widths: icon pill with a label below.
Rectangle {
    id: bar
    property var destinations: []
    property int currentIndex: 0
    signal picked(int index)
    implicitHeight: 80
    color: Theme.surface
    RowLayout {
        anchors.fill: parent; anchors.leftMargin: 8; anchors.rightMargin: 8; spacing: 0
        Repeater {
            model: bar.destinations
            Item {
                id: item
                required property var modelData
                required property int index
                readonly property bool current: index === bar.currentIndex
                Layout.fillWidth: true; Layout.fillHeight: true
                Rectangle {
                    id: pill
                    anchors.horizontalCenter: parent.horizontalCenter; y: 12
                    width: item.current ? 64 : 56; height: 32; radius: 16
                    color: item.current ? Theme.secondaryContainer : "transparent"
                    Behavior on width { NumberAnimation { duration: Theme.motion(Theme.durMedium); easing.type: Easing.BezierSpline; easing.bezierCurve: Theme.springy } }
                    Glyph { anchors.centerIn: parent; name: item.modelData.icon; fill: item.current ? 1 : 0; color: item.current ? Theme.onSecondaryContainer : Theme.subtext; font.pixelSize: 24 }
                }
                MText { anchors.horizontalCenter: parent.horizontalCenter; y: 50; text: item.modelData.title; role: "label"; font.weight: item.current ? Font.Bold : Font.Medium; color: item.current ? Theme.text : Theme.subtext }
                TapHandler { onTapped: bar.picked(item.index) }
                Accessible.role: Accessible.PageTab; Accessible.name: modelData.title; Accessible.selected: current
            }
        }
    }
}
