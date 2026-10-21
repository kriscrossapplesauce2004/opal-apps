import QtQuick
import QtQuick.Layouts
import Opal.Material
// M3 Expressive navigation rail. Collapsed: icons in pills with labels below. Expanded: a wider rail with icon and label in one pill and an extended FAB.
Rectangle {
    id: rail
    property var destinations: []
    property int currentIndex: 0
    property bool expanded: false
    property string fabIcon: ""
    property string fabText: ""
    default property alias footer: foot.data
    signal picked(int index)
    signal fabClicked()
    implicitWidth: expanded ? 240 : 96
    color: "transparent"
    Behavior on implicitWidth { NumberAnimation { duration: Theme.motion(Theme.durLong); easing.type: Easing.BezierSpline; easing.bezierCurve: Theme.emphasized } }
    ColumnLayout {
        anchors.fill: parent; anchors.topMargin: 16; anchors.bottomMargin: 16
        anchors.leftMargin: rail.expanded ? 16 : 12; anchors.rightMargin: rail.expanded ? 16 : 12
        spacing: rail.expanded ? 2 : 10
        Fab {
            visible: rail.fabIcon !== ""
            Layout.alignment: rail.expanded ? Qt.AlignLeft : Qt.AlignHCenter
            Layout.bottomMargin: 18
            icon: rail.fabIcon; text: rail.expanded ? rail.fabText : ""
            size: "small"
            onClicked: rail.fabClicked()
        }
        Repeater {
            model: rail.destinations
            Item {
                id: item
                required property var modelData
                required property int index
                readonly property bool current: index === rail.currentIndex
                Layout.fillWidth: true
                implicitHeight: rail.expanded ? 56 : 64
                activeFocusOnTab: true
                Keys.onReturnPressed: rail.picked(index)
                Rectangle {
                    id: pill
                    x: rail.expanded ? 0 : (parent.width - width) / 2
                    y: rail.expanded ? 0 : 0
                    width: rail.expanded ? parent.width : 56
                    height: rail.expanded ? 56 : 32
                    radius: height / 2
                    color: item.current ? Theme.secondaryContainer : "transparent"
                    Rectangle { anchors.fill: parent; radius: parent.radius; color: Theme.text; opacity: tap.pressed ? .12 : hover.hovered ? .08 : 0 }
                    Glyph { id: glyph; x: rail.expanded ? 18 : (parent.width - width) / 2; anchors.verticalCenter: parent.verticalCenter; name: item.modelData.icon; fill: item.current ? 1 : 0; color: item.current ? Theme.onSecondaryContainer : Theme.subtext; font.pixelSize: 24 }
                    MText { visible: rail.expanded; x: 56; anchors.verticalCenter: parent.verticalCenter; width: parent.width - 64; text: item.modelData.title; role: "titleS"; font.weight: item.current ? Font.Bold : Font.Medium; color: item.current ? Theme.onSecondaryContainer : Theme.subtext }
                    Rectangle { visible: !!item.modelData.badge; anchors.right: parent.right; anchors.rightMargin: rail.expanded ? 16 : 10; anchors.top: rail.expanded ? undefined : parent.top; anchors.verticalCenter: rail.expanded ? parent.verticalCenter : undefined; width: Math.max(16, badgeText.implicitWidth + 8); height: 16; radius: 8; color: Theme.error
                        MText { id: badgeText; anchors.centerIn: parent; text: item.modelData.badge || ""; font.pixelSize: 10; font.weight: Font.Bold; color: Theme.role("on_error", "#690005") } }
                }
                MText { visible: !rail.expanded; anchors.horizontalCenter: parent.horizontalCenter; y: 36; text: item.modelData.title; role: "label"; font.weight: item.current ? Font.Bold : Font.Medium; color: item.current ? Theme.text : Theme.subtext }
                HoverHandler { id: hover }
                TapHandler { id: tap; onTapped: rail.picked(item.index) }
                FocusRing { target: pill; anchors.fill: undefined; x: pill.x - 3; y: pill.y - 3; width: pill.width + 6; height: pill.height + 6; radius: pill.radius + 3; visible: item.activeFocus && (Posture.controllerActive || !Posture.touch) }
                Accessible.role: Accessible.PageTab; Accessible.name: modelData.title; Accessible.selected: current
            }
        }
        Item { Layout.fillHeight: true }
        ColumnLayout { id: foot; Layout.fillWidth: true; spacing: 4 }
    }
}
