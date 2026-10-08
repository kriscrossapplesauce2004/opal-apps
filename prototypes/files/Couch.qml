import QtQuick
import QtQuick.Layouts
import Opal.Material
import "Fixtures.js" as F
// Files on the couch: console-style tabs, a shelf of places, and a big-tile grid with one focused tile and the hint bar.
AppFrame {
    destinations: [{title: "Home", icon: "home"}, {title: "Recent", icon: "schedule"}, {title: "Devices", icon: "devices"}, {title: "Network", icon: "lan"}]
    currentIndex: 0
    hints: [{b: "A", t: "Open"}, {b: "B", t: "Up"}, {b: "X", t: "Actions"}, {b: "Y", t: "Search"}, {b: "LB", t: "RB"}].slice(0, 4).concat([{b: "LT", t: "Page"}])
    ColumnLayout {
        anchors.fill: parent; anchors.leftMargin: 72; anchors.rightMargin: 72; spacing: 22
        RowLayout { Layout.fillWidth: true; spacing: 16
            Breadcrumbs { path: F.path }
            Item { Layout.fillWidth: true }
            MText { text: "16 items · 1.2 TB free"; font.pixelSize: 20; color: Theme.subtext } }
        // Places shelf.
        Row {
            spacing: 18
            Repeater {
                model: [{t: "Desktop", i: "desktop_windows"}, {t: "Documents", i: "description"}, {t: "Downloads", i: "download"}, {t: "Pictures", i: "image", cur: true}, {t: "Music", i: "music_note"}, {t: "Videos", i: "movie"}, {t: "Samsung T7", i: "usb"}, {t: "OnePlus 7", i: "smartphone"}]
                Rectangle {
                    required property var modelData
                    width: 176; height: 112; radius: 28
                    color: modelData.cur ? Theme.secondaryContainer : Theme.surfaceHigh
                    Column { anchors.left: parent.left; anchors.bottom: parent.bottom; anchors.margins: 18; spacing: 6
                        Glyph { name: modelData.i; fill: modelData.cur ? 1 : 0; font.pixelSize: 34; color: modelData.cur ? Theme.onSecondaryContainer : Theme.primary }
                        MText { text: modelData.t; font.pixelSize: 20; font.weight: Font.DemiBold; color: modelData.cur ? Theme.onSecondaryContainer : Theme.text } }
                }
            }
        }
        MText { text: "Wallpapers"; font.pixelSize: 40; font.weight: Font.Bold; font.variableAxes: ({"wdth": 115, "ROND": 80}) }
        GridLayout {
            columns: 5; rowSpacing: 26; columnSpacing: 26
            Repeater {
                model: F.files.slice(0, 10)
                ColumnLayout {
                    id: tile
                    required property var modelData
                    required property int index
                    readonly property bool focusTile: index === 4
                    spacing: 10
                    Item {
                        Layout.preferredWidth: 300; Layout.preferredHeight: 180
                        Rectangle { anchors.centerIn: parent; width: parent.width * (tile.focusTile ? 1.06 : 1) + 12; height: parent.height * (tile.focusTile ? 1.06 : 1) + 12; radius: 34; color: "transparent"; border.width: 4; border.color: Theme.primary; visible: tile.focusTile }
                        Rectangle { anchors.centerIn: parent; width: parent.width * (tile.focusTile ? 1.06 : 1); height: parent.height * (tile.focusTile ? 1.06 : 1); radius: 28; color: Theme.surfaceHigh
                            RoundedImage { anchors.fill: parent; radius: 28; visible: !!tile.modelData.t; source: tile.modelData.t || ""; sourceSize: Qt.size(600, 360) }
                            FileGlyph { anchors.centerIn: parent; visible: !tile.modelData.t; file: tile.modelData; size: 110 } }
                    }
                    MText { Layout.preferredWidth: 300; Layout.topMargin: tile.focusTile ? 10 : 0; text: tile.modelData.n; font.pixelSize: tile.focusTile ? 22 : 19; font.weight: tile.focusTile ? Font.Bold : Font.Medium; color: tile.focusTile ? Theme.primary : Theme.text; elide: Text.ElideMiddle }
                }
            }
        }
        Item { Layout.fillHeight: true }
    }
}
