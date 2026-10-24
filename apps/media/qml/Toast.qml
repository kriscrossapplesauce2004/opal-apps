import QtQuick
import Opal.Material
// A short message at the bottom of the window, with an optional action.
Rectangle {
    id: toast
    property string text: ""
    property string action: ""
    property var run: null
    function show(message, actionText, actionRun) { text = message; action = actionText || ""; run = actionRun || null; shown = true; hide.restart(); }
    property bool shown: false
    anchors.horizontalCenter: parent.horizontalCenter
    y: parent.height - (shown ? height + 28 : -height)
    Behavior on y { NumberAnimation { duration: Theme.motion(Theme.durMedium); easing.type: Easing.BezierSpline; easing.bezierCurve: Theme.emphasizedDecelerate } }
    width: Math.min(parent.width - 32, row.implicitWidth + 32); height: 52; radius: 26
    color: Theme.inverseSurface
    opacity: shown ? 1 : 0
    Behavior on opacity { NumberAnimation { duration: Theme.motion(Theme.durShort) } }
    z: 100
    Timer { id: hide; interval: toast.action ? 6000 : 3200; onTriggered: toast.shown = false }
    Row {
        id: row
        anchors.centerIn: parent; spacing: 12
        MText { text: toast.text; color: Theme.inverseInk; anchors.verticalCenter: parent.verticalCenter; elide: Text.ElideRight; width: Math.min(implicitWidth, toast.parent.width - 180) }
        MButton { visible: toast.action !== ""; text: toast.action; compact: true; ink: Theme.inversePrimary; anchors.verticalCenter: parent.verticalCenter
            onClicked: { if (toast.run) toast.run(); toast.shown = false; } }
    }
    Accessible.role: Accessible.AlertMessage; Accessible.name: text
}
