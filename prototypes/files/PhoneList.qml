import QtQuick
import QtQuick.Layouts
import Opal.Material
import "Fixtures.js" as F
// Files on a phone: search bar on top, breadcrumbs as chips, a thumbnail list, a FAB and bottom navigation.
AppFrame {
    destinations: [{title: "Browse", icon: "folder"}, {title: "Recent", icon: "schedule"}, {title: "Starred", icon: "star"}, {title: "Devices", icon: "devices"}]
    currentIndex: 0
    ColumnLayout {
        anchors.fill: parent; spacing: 10
        SearchField { Layout.fillWidth: true; Layout.margins: 16; Layout.topMargin: 14; Layout.bottomMargin: 0; leadingIcon: "menu"; placeholder: "Search in Wallpapers"
            MButton { icon: "more_vert"; compact: true } }
        Flickable { Layout.fillWidth: true; Layout.preferredHeight: crumbs.height; contentWidth: crumbs.width + 32; clip: true
            Breadcrumbs { id: crumbs; x: 16; path: F.path } }
        Flickable { Layout.fillWidth: true; Layout.preferredHeight: 40; contentWidth: chips.width + 32; clip: true
            Row { id: chips; x: 16; spacing: 8
                Chip { text: "Images"; icon: "image"; glass: false }
                Chip { text: "Folders"; icon: "folder"; glass: false }
                Chip { text: "Starred"; icon: "star"; glass: false }
                Chip { text: "Modified"; icon: "sort"; trailingIcon: "expand_more"; glass: false } } }
        ListView {
            Layout.fillWidth: true; Layout.fillHeight: true; clip: true; spacing: 2
            leftMargin: 8; rightMargin: 8
            model: F.files
            delegate: Rectangle {
                id: row
                required property var modelData
                width: ListView.view.width - 16; height: 72; radius: 24; color: "transparent"
                RowLayout { anchors.fill: parent; anchors.leftMargin: 8; anchors.rightMargin: 4; spacing: 14
                    FileGlyph { file: row.modelData; size: 52 }
                    ColumnLayout { Layout.fillWidth: true; spacing: 2
                        MText { Layout.fillWidth: true; text: row.modelData.n; role: "titleS"; font.weight: Font.Medium; elide: Text.ElideMiddle }
                        MText { Layout.fillWidth: true; text: row.modelData.s + " · " + row.modelData.m; role: "bodyS"; color: Theme.subtext } }
                    Glyph { visible: !!row.modelData.star; name: "star"; fill: 1; font.pixelSize: 18; color: Theme.tertiary }
                    MButton { icon: "more_vert"; compact: true } }
            }
        }
    }
    Fab { anchors.right: parent.right; anchors.bottom: parent.bottom; anchors.margins: 16; icon: "add"; size: "medium" }
}
