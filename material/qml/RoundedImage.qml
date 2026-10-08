import QtQuick
import QtQuick.Effects
import Opal.Material
// Image clipped to a rounded rectangle through a mask, so corners stay round on every renderer that has shaders.
Item {
    id: root
    property alias source: img.source
    property alias fillMode: img.fillMode
    property alias sourceSize: img.sourceSize
    property alias status: img.status
    property real radius: Theme.shapeMd
    Image { id: img; anchors.fill: parent; fillMode: Image.PreserveAspectCrop; asynchronous: true; visible: false; layer.enabled: true }
    Rectangle { id: mask; anchors.fill: parent; radius: root.radius; visible: false; layer.enabled: true; layer.smooth: true }
    MultiEffect { anchors.fill: parent; source: img; maskEnabled: true; maskSource: mask; maskThresholdMin: .5; maskSpreadAtMin: 1 }
}
