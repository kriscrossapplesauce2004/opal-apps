import QtQuick
import QtQuick.Layouts
import Opal.Material
import "Fixtures.js" as F
// Files direction C, "Columns": Miller columns with a live preview pane. Left and right walk the tree, which suits keyboards and controllers.
Rectangle {
    color: Theme.base
    component Column: Rectangle {
        id: col
        property var items: []
        property int selected: -1
        property bool active: false
        Layout.fillHeight: true; Layout.preferredWidth: 236
        radius: 24; color: active ? Theme.surfaceLow : Theme.alpha(Theme.surfaceLow, .55)
        ColumnLayout {
            anchors.fill: parent; anchors.margins: 6; spacing: 1
            Repeater {
                model: col.items
                Rectangle {
                    id: it
                    required property var modelData
                    required property int index
                    readonly property bool sel: index === col.selected
                    Layout.fillWidth: true; implicitHeight: 38; radius: 19
                    color: sel ? (col.active ? Theme.primary : Theme.secondaryContainer) : "transparent"
                    readonly property color ink: sel ? (col.active ? Theme.onPrimary : Theme.onSecondaryContainer) : Theme.text
                    RowLayout { anchors.fill: parent; anchors.leftMargin: 8; anchors.rightMargin: 8; spacing: 10
                        FileGlyph { file: it.modelData; size: 26 }
                        MText { Layout.fillWidth: true; text: it.modelData.n; font.pixelSize: 13; font.weight: it.sel ? Font.DemiBold : Font.Normal; color: it.ink }
                        Glyph { visible: it.modelData.k === "folder"; name: "chevron_right"; font.pixelSize: 18; color: it.sel ? it.ink : Theme.muted } }
                }
            }
            Item { Layout.fillHeight: true }
        }
    }
    readonly property var home: [
        {n: "Desktop", k: "folder"}, {n: "Documents", k: "folder"}, {n: "Downloads", k: "folder", c: "tertiary"}, {n: "Music", k: "folder"},
        {n: "Pictures", k: "folder"}, {n: "Videos", k: "folder"}, {n: "opal-apps", k: "folder", c: "secondary"}, {n: "achroot", k: "folder", c: "secondary"},
        {n: "daterune", k: "folder", c: "secondary"}, {n: "notes.md", k: "text"}, {n: "keys.kdbx", k: "app"}]
    readonly property var pictures: [
        {n: "Screenshots", k: "folder", c: "tertiary"}, {n: "Wallpapers", k: "folder"}, {n: "Deltarune", k: "folder"},
        {n: "7576-kris-deltarune.png", k: "image", t: "file:///home/melissa/Pictures/7576-kris-deltarune.png"},
        {n: "click.webp", k: "image", t: "file:///home/melissa/Pictures/click.webp"}, {n: "AU1388.pdf", k: "pdf"}]
    RowLayout {
        anchors.fill: parent; spacing: 0
        PlacesPanel { Layout.fillHeight: true; Layout.preferredWidth: 248; current: "home" }
        ColumnLayout {
            Layout.fillWidth: true; Layout.fillHeight: true; Layout.rightMargin: 12; Layout.bottomMargin: 12; spacing: 10
            RowLayout { Layout.fillWidth: true; Layout.topMargin: 12; spacing: 6
                MButton { icon: "arrow_back"; compact: true }
                MButton { icon: "arrow_forward"; compact: true }
                Breadcrumbs { path: [{t: "Home", i: "home"}, {t: "Pictures"}, {t: "Wallpapers"}, {t: "dr5-keyart.png"}] }
                Item { Layout.fillWidth: true }
                SearchField { Layout.preferredWidth: 260; implicitHeight: 40; placeholder: "Search Home" }
                SegmentedButtons { current: "cols"; options: [{id: "list", icon: "view_list"}, {id: "grid", icon: "grid_view"}, {id: "cols", icon: "view_column"}] } }
            RowLayout {
                Layout.fillWidth: true; Layout.fillHeight: true; spacing: 6
                Column { items: home; selected: 4 }
                Column { items: pictures; selected: 1 }
                Column { items: F.files.slice(3, 16); selected: 0; active: true }
                // Preview pane.
                Rectangle {
                    Layout.fillWidth: true; Layout.fillHeight: true; radius: 28; color: Theme.surface
                    ColumnLayout {
                        anchors.fill: parent; anchors.margins: 20; spacing: 14
                        RoundedImage { Layout.fillWidth: true; Layout.preferredHeight: width * 9 / 16; radius: 22; source: F.files[3].t; sourceSize: Qt.size(900, 506) }
                        MText { Layout.fillWidth: true; text: "dr5-keyart.png"; role: "headline" }
                        RowLayout { spacing: 8
                            Chip { text: "Starred"; icon: "star"; selected: true }
                            Chip { text: "Wallpaper"; icon: "wallpaper"; glass: false }
                            Chip { text: "Add tag"; icon: "add"; glass: false } }
                        GridLayout {
                            Layout.fillWidth: true; Layout.topMargin: 6; columns: 2; columnSpacing: 24; rowSpacing: 10
                            Repeater {
                                model: [["Kind", "PNG image"], ["Size", "4.8 MB"], ["Dimensions", "3840 × 2160"], ["Modified", "Today, 13:58"], ["Created", "6 Jun 2026"], ["Where", "~/Pictures/Wallpapers"], ["Permissions", "You can read and write"]]
                                RowLayout {
                                    required property var modelData
                                    Layout.columnSpan: 2; spacing: 0
                                    MText { Layout.preferredWidth: 110; text: modelData[0]; role: "bodyS"; color: Theme.subtext }
                                    MText { Layout.fillWidth: true; text: modelData[1]; role: "bodyS" } } } }
                        Item { Layout.fillHeight: true }
                        RowLayout { Layout.fillWidth: true; spacing: 8
                            MButton { text: "Open in Photos"; icon: "photo"; filled: true }
                            MButton { text: "Open with"; icon: "apps"; tonal: true }
                            Item { Layout.fillWidth: true }
                            MButton { icon: "share"; compact: true }
                            MButton { icon: "more_vert"; compact: true } }
                    }
                }
            }
        }
    }
}
