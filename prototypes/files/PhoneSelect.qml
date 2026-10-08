import QtQuick
import QtQuick.Layouts
import Opal.Material
import "Fixtures.js" as F
// Files on a phone in selection mode: a contextual top bar, a three-column grid with picked tiles, and the floating toolbar in place of the FAB.
AppFrame {
    destinations: [{title: "Browse", icon: "folder"}, {title: "Recent", icon: "schedule"}, {title: "Starred", icon: "star"}, {title: "Devices", icon: "devices"}]
    currentIndex: 0
    ColumnLayout {
        anchors.fill: parent; spacing: 6
        Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 64; color: Theme.surfaceHigh
            RowLayout { anchors.fill: parent; anchors.leftMargin: 4; anchors.rightMargin: 4; spacing: 4
                MButton { icon: "close"; compact: true }
                MText { Layout.fillWidth: true; text: "3 selected"; role: "title" }
                MButton { icon: "select_all"; compact: true }
                MButton { icon: "more_vert"; compact: true } } }
        MText { text: "Today"; role: "titleS"; color: Theme.primary; Layout.leftMargin: 16; Layout.topMargin: 6 }
        GridLayout {
            Layout.fillWidth: true; Layout.leftMargin: 12; Layout.rightMargin: 12; columns: 3; rowSpacing: 4; columnSpacing: 4
            Repeater {
                model: F.files.slice(3, 15)
                Item {
                    id: cell
                    required property var modelData
                    required property int index
                    readonly property bool sel: index < 3
                    Layout.fillWidth: true; Layout.preferredHeight: width
                    RoundedImage { anchors.centerIn: parent; width: parent.width * (cell.sel ? .86 : 1); height: width; radius: cell.sel ? 26 : 10; source: cell.modelData.t || ""; sourceSize: Qt.size(260, 260); visible: !!cell.modelData.t }
                    FileGlyph { anchors.centerIn: parent; visible: !cell.modelData.t; file: cell.modelData; size: parent.width; radius: 10 }
                    Rectangle { x: 6; y: 6; width: 26; height: 26; radius: 13; color: cell.sel ? Theme.primary : Theme.alpha("#000000", .3); border.width: 2; border.color: cell.sel ? Theme.base : "#ffffff"
                        Glyph { anchors.centerIn: parent; visible: cell.sel; name: "check"; font.pixelSize: 17; color: Theme.onPrimary } }
                }
            }
        }
        Item { Layout.fillHeight: true }
    }
    SelectionToolbar { anchors.horizontalCenter: parent.horizontalCenter; anchors.bottom: parent.bottom; anchors.bottomMargin: 96; showLabels: false; showCount: false
        actions: [{i: "content_copy"}, {i: "drive_file_move"}, {i: "share"}, {i: "delete"}] }
}
