import QtQuick
import Opal.Material
// M3 focus indicator: an outline outside the target. Shows for keyboard and controller focus only, never for a pointer or touch press.
Rectangle {
    property Item target: parent
    property real radiusOffset: 3
    anchors.fill: target
    anchors.margins: -3
    radius: (target && target.radius !== undefined ? target.radius : Theme.shapeMd) + radiusOffset
    color: "transparent"
    border.width: Theme.couch ? 3 : 2
    border.color: Theme.primary
    visible: target && target.activeFocus && (Posture.controllerActive || !Posture.touch)
    z: 10
}
