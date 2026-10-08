import QtQuick
import QtQuick.Layouts
import Opal.Material
// Path as connected chips: earlier segments are quiet, the current folder is filled. Clicking the trailing area turns it into a text field.
Rectangle {
    id: crumbs
    property var path: []
    implicitHeight: Theme.couch ? 56 : Theme.touch ? 44 : 40
    implicitWidth: row.implicitWidth + 8
    radius: height / 2
    color: Theme.surfaceHigh
    RowLayout {
        id: row
        anchors.verticalCenter: parent.verticalCenter; x: 4; spacing: 2
        Repeater {
            model: crumbs.path
            RowLayout {
                id: seg
                required property var modelData
                required property int index
                readonly property bool last: index === crumbs.path.length - 1
                spacing: 2
                Rectangle {
                    implicitHeight: crumbs.height - 8; implicitWidth: segRow.implicitWidth + 22
                    radius: height / 2
                    color: seg.last ? Theme.secondaryContainer : "transparent"
                    RowLayout { id: segRow; anchors.centerIn: parent; spacing: 6
                        Glyph { visible: !!seg.modelData.i; name: seg.modelData.i || ""; font.pixelSize: Theme.px(18); fill: 1; color: seg.last ? Theme.onSecondaryContainer : Theme.subtext }
                        MText { text: seg.modelData.t; role: "label"; font.pixelSize: Theme.px(13); font.weight: seg.last ? Font.DemiBold : Font.Medium; color: seg.last ? Theme.onSecondaryContainer : Theme.subtext } }
                }
                Glyph { visible: !seg.last; name: "chevron_right"; font.pixelSize: Theme.px(16); color: Theme.muted }
            }
        }
    }
}
