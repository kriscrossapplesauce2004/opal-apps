import QtQuick
import QtQuick.Effects
import QtQuick.Layouts
import Opal.Material
// Tablet or desktop: the keyboard floats as a compact card you can drag and resize.
Rectangle {
    color: Theme.base
    ColumnLayout { anchors.fill: parent; anchors.margins: 40; spacing: 16
        MText { text: "Notes"; role: "displayS" }
        Rectangle { Layout.fillWidth: true; Layout.fillHeight: true; radius: 28; color: Theme.surface
            MText { anchors.fill: parent; anchors.margins: 24; wrapMode: Text.Wrap; font.pixelSize: 20; text: "Floating keyboard: drag the handle to move it anywhere, pull the corner to resize, double-tap the handle to dock it again." } } }
    Item {
        id: floating
        x: 560; y: 330; width: 520; height: card.height
        RectangularShadow { anchors.fill: card; radius: 30; blur: 40; spread: 2; color: Qt.rgba(0, 0, 0, .45); offset.y: 10 }
        Rectangle { id: card; width: parent.width; height: kbd.height + 24; radius: 30; color: Theme.surfaceLow; clip: true
            Rectangle { anchors.horizontalCenter: parent.horizontalCenter; y: 8; width: 44; height: 5; radius: 3; color: Theme.alpha(Theme.subtext, .6) }
            Kbd { id: kbd; y: 18; width: parent.width; keyH: 44; color: "transparent" }
            Glyph { anchors.right: parent.right; anchors.bottom: parent.bottom; anchors.margins: 4; name: "drag_indicator"; rotation: 45; font.pixelSize: 18; color: Theme.subtext } }
    }
}
