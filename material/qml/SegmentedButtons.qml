import QtQuick
import QtQuick.Layouts
import Opal.Material
// M3 Expressive connected button group: one pill split into segments with a small gap. Shapes stay fixed; selection only changes the colour.
Row {
    id: group
    property var options: []
    property var current: null
    signal picked(var id)
    readonly property real segH: Theme.couch ? 52 : Theme.touch ? 40 : 34
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
            readonly property real inner: Math.min(6, height / 4)
            implicitHeight: group.segH
            implicitWidth: content.implicitWidth + (modelData.name ? 28 : 22)
            topLeftRadius: first ? height / 2 : inner; bottomLeftRadius: topLeftRadius
            topRightRadius: last ? height / 2 : inner; bottomRightRadius: topRightRadius
            color: sel ? Theme.secondaryContainer : Theme.surfaceHigh
            Behavior on color { ColorAnimation { duration: Theme.motion(Theme.durShort) } }
            Rectangle { anchors.fill: parent; topLeftRadius: seg.topLeftRadius; bottomLeftRadius: seg.bottomLeftRadius; topRightRadius: seg.topRightRadius; bottomRightRadius: seg.bottomRightRadius
                color: seg.sel ? Theme.onSecondaryContainer : Theme.text; opacity: tap.pressed ? .12 : hover.hovered ? .06 : 0 }
            RowLayout { id: content; anchors.centerIn: parent; spacing: 6
                Glyph { visible: !!seg.modelData.icon; name: seg.modelData.icon || ""; fill: seg.sel ? 1 : 0; font.pixelSize: Theme.px(18); color: seg.sel ? Theme.onSecondaryContainer : Theme.subtext }
                MText { visible: !!seg.modelData.name; text: seg.modelData.name || ""; role: "label"; font.weight: seg.sel ? Font.DemiBold : Font.Medium; color: seg.sel ? Theme.onSecondaryContainer : Theme.text } }
            HoverHandler { id: hover }
            TapHandler { id: tap; onTapped: group.picked(seg.modelData.id) }
            activeFocusOnTab: true
            Keys.onReturnPressed: group.picked(modelData.id)
            FocusRing {}
            Accessible.role: Accessible.RadioButton; Accessible.name: modelData.name || modelData.icon; Accessible.checked: sel
        }
    }
}
