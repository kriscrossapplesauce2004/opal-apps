import QtQuick
import QtQuick.Layouts
import Opal.Material
// M3 Expressive floating toolbar for a selection. The count chip on the left is where the selection badge morphs from.
Rectangle {
    id: bar
    property int count: 3
    property bool showLabels: true
    property bool showCount: true
    property var actions: [{i: "content_copy", t: "Copy"}, {i: "drive_file_move", t: "Move"}, {i: "share", t: "Share"}, {i: "edit", t: "Rename"}, {i: "star", t: "Star"}, {i: "delete", t: "Delete"}]
    implicitHeight: Theme.couch ? 80 : 64
    implicitWidth: row.implicitWidth + 16
    radius: height / 2
    color: Theme.primaryContainer
    RowLayout {
        id: row; anchors.centerIn: parent; spacing: 4
        Rectangle { visible: bar.showCount; implicitHeight: bar.height - 16; implicitWidth: countRow.implicitWidth + 28; radius: height / 2; color: Theme.primary
            RowLayout { id: countRow; anchors.centerIn: parent; spacing: 6
                Glyph { name: "close"; font.pixelSize: Theme.px(18); color: Theme.onPrimary }
                MText { text: bar.count + " selected"; role: "label"; font.pixelSize: Theme.px(13); font.weight: Font.Bold; color: Theme.onPrimary } } }
        Repeater {
            model: bar.actions
            Rectangle {
                id: act
                required property var modelData
                implicitHeight: bar.height - 16; implicitWidth: actRow.implicitWidth + (bar.showLabels ? 24 : 16); radius: height / 2
                color: "transparent"
                RowLayout { id: actRow; anchors.centerIn: parent; spacing: 6
                    Glyph { name: act.modelData.i; font.pixelSize: Theme.px(20); color: act.modelData.i === "delete" ? Theme.error : Theme.onContainer }
                    MText { visible: bar.showLabels; text: act.modelData.t; role: "label"; font.pixelSize: Theme.px(13); color: Theme.onContainer } }
            }
        }
        MButton { icon: "more_vert"; compact: true; ink: Theme.onContainer }
    }
}
