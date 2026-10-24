import QtQuick
import QtQuick.Layouts
import Opal.Material
// One pill split into separate buttons (ChromeOS style): round outer ends, small inner corners, a gap between segments.
RowLayout {
    id: pill
    property var items: []
    property var s: null
    property real h: 48
    signal picked(string id)
    spacing: 3
    Repeater {
        model: pill.items
        Rectangle {
            id: seg
            required property var modelData
            required property int index
            readonly property bool first: index === 0
            readonly property bool last: index === pill.items.length - 1
            Layout.fillWidth: true; implicitHeight: pill.h; implicitWidth: segRow.implicitWidth + 28
            topLeftRadius: first ? pill.h / 2 : 6; bottomLeftRadius: topLeftRadius
            topRightRadius: last ? pill.h / 2 : 6; bottomRightRadius: topRightRadius
            color: modelData.on ? (pill.s ? pill.s.secondary_container : Theme.secondaryContainer) : (pill.s ? pill.s.surface_container_high : Theme.surfaceHigh)
            Rectangle { anchors.fill: parent; topLeftRadius: parent.topLeftRadius; bottomLeftRadius: parent.bottomLeftRadius; topRightRadius: parent.topRightRadius; bottomRightRadius: parent.bottomRightRadius
                color: pill.s ? pill.s.on_surface : Theme.text; opacity: segTap.pressed ? .12 : segHover.hovered ? .06 : 0 }
            RowLayout { id: segRow; anchors.centerIn: parent; spacing: 7
                Glyph { name: seg.modelData.i; fill: seg.modelData.on ? 1 : 0; font.pixelSize: 19; color: pill.s ? pill.s.primary : Theme.primary }
                MText { text: seg.modelData.t; role: "label"; elide: Text.ElideRight; Layout.maximumWidth: seg.width - 60; color: pill.s ? pill.s.on_surface : Theme.text } }
            HoverHandler { id: segHover; cursorShape: Qt.PointingHandCursor }
            TapHandler { id: segTap; onTapped: pill.picked(seg.modelData.id) }
            Accessible.role: Accessible.Button; Accessible.name: modelData.t; Accessible.checked: !!modelData.on
        }
    }
}
