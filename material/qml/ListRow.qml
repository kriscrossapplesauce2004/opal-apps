import QtQuick
import QtQuick.Layouts
import Opal.Material
// One-, two- or three-line list item with a leading glyph or image and trailing content. Grouped rows round only the group ends.
Rectangle {
    id: row
    property string title: ""
    property string subtitle: ""
    property string icon: ""
    property string image: ""
    property color iconTint: Theme.subtext
    property bool iconBadge: false
    property string trailing: ""
    property string trailingIcon: ""
    property bool selected: false
    property bool grouped: false
    property bool first: true
    property bool last: true
    property real leadSize: Theme.couch ? 48 : subtitle !== "" ? 40 : 28
    default property alias extra: tail.data
    signal clicked()
    signal activated()
    signal contextRequested(real x, real y)
    implicitHeight: Math.max(Theme.row, subtitle !== "" ? Theme.row + Theme.px(14) : Theme.row)
    implicitWidth: 320
    readonly property real r: grouped ? Theme.shapeLg : Theme.shapeLg
    topLeftRadius: grouped && !first ? 6 : height / 2 > r ? r : height / 2
    topRightRadius: topLeftRadius
    bottomLeftRadius: grouped && !last ? 6 : height / 2 > r ? r : height / 2
    bottomRightRadius: bottomLeftRadius
    color: selected ? Theme.secondaryContainer : grouped ? Theme.alpha(Theme.surfaceHigh, .9) : "transparent"
    Behavior on color { ColorAnimation { duration: Theme.motion(Theme.durShort) } }
    readonly property color ink: selected ? Theme.onSecondaryContainer : Theme.text
    Rectangle { anchors.fill: parent; topLeftRadius: row.topLeftRadius; topRightRadius: row.topRightRadius; bottomLeftRadius: row.bottomLeftRadius; bottomRightRadius: row.bottomRightRadius; color: row.ink; opacity: mouse.pressed ? .12 : mouse.containsMouse ? .06 : 0 }
    RowLayout {
        anchors.fill: parent; anchors.leftMargin: Theme.px(14); anchors.rightMargin: Theme.px(12); spacing: Theme.px(14)
        Rectangle {
            visible: row.icon !== "" || row.image !== ""
            Layout.preferredWidth: row.leadSize; Layout.preferredHeight: row.leadSize
            radius: row.iconBadge ? row.leadSize / 2 : Theme.shapeSm
            color: row.iconBadge ? Theme.alpha(row.iconTint, .16) : "transparent"
            clip: row.image !== ""
            Glyph { anchors.centerIn: parent; visible: row.image === ""; name: row.icon; fill: row.selected ? 1 : 0; color: row.selected ? row.ink : row.iconTint; font.pixelSize: Math.round(row.leadSize * (row.iconBadge ? .55 : .8)) }
            Image { anchors.fill: parent; visible: row.image !== ""; source: row.image; fillMode: Image.PreserveAspectCrop; asynchronous: true; sourceSize: Qt.size(row.leadSize * 2, row.leadSize * 2) }
        }
        ColumnLayout {
            Layout.fillWidth: true; spacing: 1
            MText { Layout.fillWidth: true; text: row.title; role: Theme.couch ? "title" : "titleS"; font.weight: Font.Medium; color: row.ink }
            MText { visible: row.subtitle !== ""; Layout.fillWidth: true; text: row.subtitle; role: "bodyS"; color: row.selected ? row.ink : Theme.subtext }
        }
        Row { id: tail; spacing: Theme.gapSm; Layout.alignment: Qt.AlignVCenter }
        MText { visible: row.trailing !== ""; text: row.trailing; role: "label"; color: Theme.subtext }
        Glyph { visible: row.trailingIcon !== ""; name: row.trailingIcon; color: Theme.subtext; font.pixelSize: Theme.px(20) }
    }
    // Below the row content, so buttons put in the trailing slot get their own clicks.
    MouseArea {
        id: mouse; anchors.fill: parent; z: -1; hoverEnabled: true; acceptedButtons: Qt.LeftButton | Qt.RightButton
        onClicked: e => { if (e.button === Qt.RightButton) row.contextRequested(e.x, e.y); else row.clicked(); }
        onDoubleClicked: row.activated()
        onPressAndHold: e => row.contextRequested(e.x, e.y)
    }
    activeFocusOnTab: true
    Keys.onReturnPressed: { row.clicked(); row.activated(); }
    Keys.onMenuPressed: row.contextRequested(width / 2, height / 2)
    FocusRing {}
    Accessible.role: Accessible.ListItem
    Accessible.name: title
    Accessible.description: subtitle
    Accessible.selected: selected
    Accessible.onPressAction: clicked()
}
