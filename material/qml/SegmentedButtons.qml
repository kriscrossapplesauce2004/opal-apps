import QtQuick
import QtQuick.Layouts
import Opal.Material
// M3 Expressive button group as one full pill: the track is a single pill and the selected option is a filled pill inside it that slides between options.
Rectangle {
    id: group
    property var options: []
    property var current: null
    signal picked(var id)
    readonly property int currentIndex: options.findIndex(o => o.id === current)
    // itemAt() does not notify, so this counter makes the selected pill look up its target again when items appear.
    property int tick: 0
    readonly property real segH: Theme.couch ? 48 : Theme.touch ? 38 : 30
    implicitHeight: segH + 6
    implicitWidth: row.implicitWidth + 6
    radius: height / 2
    color: Theme.surfaceHigh
    // The selected pill moves under the options instead of each option changing colour on its own.
    Rectangle {
        readonly property Item target: group.tick >= 0 && group.currentIndex >= 0 && group.currentIndex < rep.count ? rep.itemAt(group.currentIndex) : null
        visible: target !== null
        x: 3 + (target ? target.x : 0); y: 3
        width: target ? target.width : 0; height: group.segH
        radius: height / 2
        color: Theme.secondaryContainer
        Behavior on x { NumberAnimation { duration: Theme.motion(Theme.durMedium); easing.type: Easing.BezierSpline; easing.bezierCurve: Theme.emphasized } }
        Behavior on width { NumberAnimation { duration: Theme.motion(Theme.durMedium); easing.type: Easing.BezierSpline; easing.bezierCurve: Theme.emphasized } }
    }
    Row {
        id: row
        x: 3; y: 3
        Repeater {
            id: rep
            model: group.options
            onItemAdded: Qt.callLater(() => group.tick++)
            Item {
                id: seg
                required property var modelData
                required property int index
                readonly property bool sel: index === group.currentIndex
                implicitHeight: group.segH
                implicitWidth: content.implicitWidth + (seg.modelData.name ? 28 : 22)
                width: implicitWidth; height: implicitHeight
                RowLayout { id: content; anchors.centerIn: parent; spacing: 6
                    Glyph { visible: !!seg.modelData.icon; name: seg.modelData.icon || ""; fill: seg.sel ? 1 : 0; font.pixelSize: Theme.px(18); color: seg.sel ? Theme.onSecondaryContainer : Theme.subtext }
                    MText { visible: !!seg.modelData.name; text: seg.modelData.name || ""; role: "label"; font.weight: seg.sel ? Font.DemiBold : Font.Medium; color: seg.sel ? Theme.onSecondaryContainer : Theme.text } }
                TapHandler { onTapped: group.picked(seg.modelData.id) }
                Accessible.role: Accessible.RadioButton; Accessible.name: modelData.name || modelData.icon; Accessible.checked: sel
            }
        }
    }
}
