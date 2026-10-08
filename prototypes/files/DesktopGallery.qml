import QtQuick
import QtQuick.Layouts
import Opal.Material
import "Fixtures.js" as F
// Files direction B, "Gallery": thumbnail-first. Filter chips, date groups, big tiles, and a selection that becomes a floating toolbar.
Rectangle {
    color: Theme.base
    component Tile: ColumnLayout {
        id: tile
        property var file
        property bool sel: false
        property real edge: 176
        spacing: 8
        Item {
            Layout.preferredWidth: tile.edge; Layout.preferredHeight: tile.edge * .75
            Rectangle {
                anchors.centerIn: parent
                width: parent.width * (tile.sel ? .9 : 1); height: parent.height * (tile.sel ? .88 : 1)
                radius: tile.sel ? 28 : 18
                color: Theme.surfaceHighest
                RoundedImage { anchors.fill: parent; radius: parent.radius; source: tile.file.t || ""; sourceSize: Qt.size(360, 270); visible: !!tile.file.t }
                FileGlyph { anchors.centerIn: parent; visible: !tile.file.t; file: tile.file; size: 72; thumb: false; color: "transparent" }
            }
            Rectangle { visible: tile.sel; x: 2; y: 2; width: 28; height: 28; radius: 14; color: Theme.primary; border.width: 2; border.color: Theme.base
                Glyph { anchors.centerIn: parent; name: "check"; font.pixelSize: 18; color: Theme.onPrimary } }
            Rectangle { visible: !!tile.file.star; anchors.right: parent.right; anchors.bottom: parent.bottom; anchors.margins: 8; width: 26; height: 26; radius: 13; color: Theme.alpha("#000000", .45)
                Glyph { anchors.centerIn: parent; name: "star"; fill: 1; font.pixelSize: 16; color: "#ffd86b" } }
        }
        MText { Layout.preferredWidth: tile.edge; text: tile.file.n; role: "label"; font.pixelSize: 13; font.weight: tile.sel ? Font.DemiBold : Font.Medium; elide: Text.ElideMiddle; color: tile.sel ? Theme.primary : Theme.text }
        MText { Layout.preferredWidth: tile.edge; text: tile.file.s; role: "labelS"; color: Theme.subtext }
    }
    RowLayout {
        anchors.fill: parent; spacing: 0
        PlacesPanel { Layout.fillHeight: true; Layout.preferredWidth: 248; current: "pictures" }
        Rectangle {
            Layout.fillWidth: true; Layout.fillHeight: true; Layout.margins: 12; Layout.leftMargin: 0
            radius: 28; color: Theme.surfaceLow
            ColumnLayout {
                anchors.fill: parent; anchors.margins: 24; anchors.topMargin: 18; spacing: 14
                RowLayout { Layout.fillWidth: true; spacing: 8
                    MButton { icon: "arrow_back"; compact: true }
                    Breadcrumbs { path: F.path.slice(0, 2) }
                    Item { Layout.fillWidth: true }
                    SearchField { Layout.preferredWidth: 300; implicitHeight: 44; placeholder: "Search Wallpapers" }
                    MButton { icon: "vertical_split"; compact: true }
                    MButton { icon: "more_vert"; compact: true } }
                RowLayout { Layout.fillWidth: true; spacing: 16
                    MText { text: "Wallpapers"; role: "displayS" }
                    MText { text: "16 items · 45 MB"; role: "body"; color: Theme.subtext; Layout.alignment: Qt.AlignBaseline }
                    Item { Layout.fillWidth: true }
                    SegmentedButtons { current: "grid"; options: [{id: "list", icon: "view_list"}, {id: "grid", icon: "grid_view"}, {id: "cols", icon: "view_column"}] } }
                RowLayout { Layout.fillWidth: true; spacing: 8
                    Chip { text: "All"; selected: true; glass: false }
                    Chip { text: "Images · 11"; icon: "image"; glass: false }
                    Chip { text: "Folders · 3"; icon: "folder"; glass: false }
                    Chip { text: "Archives · 1"; icon: "folder_zip"; glass: false }
                    Chip { text: "Starred"; icon: "star"; glass: false }
                    Item { Layout.fillWidth: true }
                    Chip { text: "Modified"; icon: "sort"; trailingIcon: "expand_more"; glass: false } }
                // Folders as wide tonal cards.
                RowLayout { Layout.fillWidth: true; Layout.topMargin: 6; spacing: 10
                    Repeater {
                        model: F.files.slice(0, 3)
                        Rectangle {
                            id: folder
                            required property var modelData
                            Layout.fillWidth: true; implicitHeight: 64; radius: 20; color: Theme.surfaceHigh
                            RowLayout { anchors.fill: parent; anchors.leftMargin: 12; anchors.rightMargin: 14; spacing: 12
                                FileGlyph { file: folder.modelData; size: 40 }
                                ColumnLayout { Layout.fillWidth: true; spacing: 0
                                    MText { Layout.fillWidth: true; text: folder.modelData.n; role: "titleS" }
                                    MText { Layout.fillWidth: true; text: folder.modelData.s + " · " + folder.modelData.m; role: "labelS"; color: Theme.subtext } }
                                Glyph { name: "chevron_right"; color: Theme.subtext; font.pixelSize: 20 } }
                        }
                    } }
                MText { text: "Today"; role: "titleS"; color: Theme.primary; Layout.topMargin: 8 }
                RowLayout { spacing: 18
                    Tile { file: F.files[3]; sel: true }
                    Tile { file: F.files[4]; sel: true }
                    Tile { file: F.files[5]; sel: true } }
                MText { text: "This week"; role: "titleS"; color: Theme.subtext; Layout.topMargin: 4 }
                RowLayout { spacing: 18
                    Repeater { model: F.files.slice(6, 11); Tile { required property var modelData; file: modelData } } }
                Item { Layout.fillHeight: true }
            }
            SelectionToolbar { anchors.horizontalCenter: parent.horizontalCenter; anchors.bottom: parent.bottom; anchors.bottomMargin: 24 }
        }
    }
}
