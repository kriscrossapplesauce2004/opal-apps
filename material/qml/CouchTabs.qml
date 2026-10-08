import QtQuick
import QtQuick.Layouts
import Opal.Material
// Couch navigation: a centred row of large tabs with LB and RB hints, like a console dashboard.
Item {
    id: tabs
    property var destinations: []
    property int currentIndex: 0
    signal picked(int index)
    implicitHeight: 88
    RowLayout {
        anchors.centerIn: parent; spacing: 18
        ShoulderKey { text: "LB" }
        Repeater {
            model: tabs.destinations
            Rectangle {
                id: tab
                required property var modelData
                required property int index
                readonly property bool current: index === tabs.currentIndex
                implicitHeight: 56; implicitWidth: label.implicitWidth + (current ? 76 : 48)
                radius: 28
                color: current ? Theme.primary : "transparent"
                Behavior on implicitWidth { NumberAnimation { duration: Theme.motion(Theme.durMedium); easing.type: Easing.BezierSpline; easing.bezierCurve: Theme.springy } }
                Behavior on color { ColorAnimation { duration: Theme.motion(Theme.durShort) } }
                RowLayout { anchors.centerIn: parent; spacing: 10
                    Glyph { visible: tab.current; name: tab.modelData.icon; fill: 1; color: Theme.onPrimary; font.pixelSize: 28 }
                    MText { id: label; text: tab.modelData.title; font.pixelSize: 22; font.weight: tab.current ? Font.Bold : Font.Medium; color: tab.current ? Theme.onPrimary : Theme.subtext } }
                TapHandler { onTapped: tabs.picked(tab.index) }
            }
        }
        ShoulderKey { text: "RB" }
    }
    component ShoulderKey: Rectangle {
        property alias text: t.text
        implicitWidth: 52; implicitHeight: 32; radius: 10
        color: Theme.secondaryContainer
        MText { id: t; anchors.centerIn: parent; font.pixelSize: 14; font.weight: Font.Bold; color: Theme.onSecondaryContainer }
    }
}
