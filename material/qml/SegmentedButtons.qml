import QtQuick
import QtQuick.Layouts
import Opal.Material
// M3 Expressive connected button group: the selected segment grows rounder and fills.
Row {
    id: group
    property var options: []
    property var current: null
    signal picked(var id)
    spacing: 2
    Repeater {
        model: group.options
        Rectangle {
            id: seg
            required property var modelData
            required property int index
            readonly property bool sel: modelData.id === group.current
            readonly property bool first: index === 0
            readonly property bool last: index === group.options.length - 1
            readonly property real outer: height / 2
            readonly property real inner: sel ? height / 2 : 6
            implicitHeight: Theme.couch ? 56 : Theme.touch ? 44 : 36
            implicitWidth: row.implicitWidth + 28
            topLeftRadius: first || sel ? outer : inner; bottomLeftRadius: topLeftRadius
            topRightRadius: last || sel ? outer : inner; bottomRightRadius: topRightRadius
            color: sel ? Theme.secondaryContainer : Theme.surfaceHigh
            Behavior on topLeftRadius { NumberAnimation { duration: Theme.motion(Theme.durMedium); easing.type: Easing.OutCubic } }
            Behavior on topRightRadius { NumberAnimation { duration: Theme.motion(Theme.durMedium); easing.type: Easing.OutCubic } }
            RowLayout { id: row; anchors.centerIn: parent; spacing: 6
                Glyph { visible: !!seg.modelData.icon; name: seg.modelData.icon || ""; fill: seg.sel ? 1 : 0; font.pixelSize: Theme.px(18); color: seg.sel ? Theme.onSecondaryContainer : Theme.subtext }
                MText { visible: !!seg.modelData.name; text: seg.modelData.name || ""; role: "label"; color: seg.sel ? Theme.onSecondaryContainer : Theme.text } }
            TapHandler { onTapped: group.picked(seg.modelData.id) }
            Accessible.role: Accessible.RadioButton; Accessible.name: modelData.name || modelData.icon; Accessible.checked: sel
        }
    }
}
