import QtQuick
import Opal.Material
// Bottom sheet for touch and compact widths, centred dialog-sized card otherwise. Drag the handle down to dismiss.
Item {
    id: sheet
    property bool open: false
    property real sheetHeight: 360
    property bool docked: Posture.touch || width < 600
    default property alias content: body.data
    signal dismissed()
    anchors.fill: parent
    visible: t > 0
    property real t: open ? 1 : 0
    Behavior on t { NumberAnimation { duration: Theme.motion(sheet.open ? 420 : 280); easing.type: Easing.BezierSpline; easing.bezierCurve: sheet.open ? Theme.emphasizedDecelerate : Theme.emphasizedAccelerate } }
    z: 50
    Rectangle { anchors.fill: parent; color: "#000000"; opacity: .32 * sheet.t
        TapHandler { onTapped: { sheet.open = false; sheet.dismissed(); } } }
    Rectangle {
        id: card
        width: sheet.docked ? Math.min(parent.width, 640) : Math.min(parent.width - 48, 560)
        height: Math.min(sheet.sheetHeight, parent.height - 48)
        x: (parent.width - width) / 2
        y: sheet.docked ? parent.height - height * sheet.t + drag.translation : (parent.height - height) / 2 + (1 - sheet.t) * 24
        opacity: sheet.docked ? 1 : sheet.t
        topLeftRadius: 28; topRightRadius: 28
        bottomLeftRadius: sheet.docked ? 0 : 28; bottomRightRadius: bottomLeftRadius
        color: Theme.surfaceLow
        Rectangle { visible: sheet.docked; anchors.horizontalCenter: parent.horizontalCenter; y: 12; width: 32; height: 4; radius: 2; color: Theme.alpha(Theme.subtext, .5) }
        Item { id: body; anchors.fill: parent; anchors.topMargin: sheet.docked ? 28 : 20 }
        DragHandler { id: drag; enabled: sheet.docked; target: null; yAxis.minimum: 0
            property real translation: active ? Math.max(0, centroid.position.y - centroid.pressPosition.y) : 0
            onActiveChanged: if (!active && translation > card.height * .3) { sheet.open = false; sheet.dismissed(); } }
    }
    Keys.onEscapePressed: { open = false; dismissed(); }
}
