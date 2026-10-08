import QtQuick
import QtQuick.Layouts
import Opal.Material
// One pill split into separate buttons (ChromeOS style): full round outer ends, small inner corners, a clear gap between segments.
RowLayout {
    id: pill
    property var items: []
    property var s: null
    property real h: 48
    spacing: 3
    Repeater {
        model: pill.items
        Rectangle {
            required property var modelData
            required property int index
            readonly property bool first: index === 0
            readonly property bool last: index === pill.items.length - 1
            Layout.fillWidth: true; implicitHeight: pill.h
            topLeftRadius: first ? pill.h / 2 : 6; bottomLeftRadius: topLeftRadius
            topRightRadius: last ? pill.h / 2 : 6; bottomRightRadius: topRightRadius
            color: pill.s ? pill.s.surface_container_high : Theme.surfaceHigh
            RowLayout { anchors.centerIn: parent; spacing: 7
                Glyph { name: modelData.i; font.pixelSize: 19; color: pill.s ? pill.s.primary : Theme.primary }
                MText { text: modelData.t; role: "label"; color: pill.s ? pill.s.on_surface : Theme.text } }
            Accessible.role: Accessible.Button; Accessible.name: modelData.t
        }
    }
}
