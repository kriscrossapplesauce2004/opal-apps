import QtQuick
import QtQuick.Layouts
import Opal.Material
import "Fixtures.js" as F
// Files direction A, "Workbench": Dolphin-class power. Tabs, a split view with a details list and a grid, and a running copy as a pill.
Rectangle {
    color: Theme.base
    component Tab: Rectangle {
        property string label; property string icon: "folder"; property bool cur: false
        implicitHeight: 40; implicitWidth: tabRow.implicitWidth + 30; radius: 20
        color: cur ? Theme.surfaceHigh : "transparent"
        RowLayout { id: tabRow; anchors.centerIn: parent; spacing: 8
            Glyph { name: icon; fill: cur ? 1 : 0; font.pixelSize: 18; color: cur ? Theme.primary : Theme.subtext }
            MText { text: label; role: "label"; font.pixelSize: 13; font.weight: cur ? Font.DemiBold : Font.Medium; color: cur ? Theme.text : Theme.subtext }
            Glyph { visible: cur; name: "close"; font.pixelSize: 16; color: Theme.subtext } }
    }
    component HeaderCell: Item {
        property string label; property bool sorted: false
        implicitHeight: 36
        Row { anchors.verticalCenter: parent.verticalCenter; spacing: 4
            MText { text: label; role: "label"; color: sorted ? Theme.primary : Theme.subtext; font.weight: sorted ? Font.Bold : Font.Medium }
            Glyph { visible: sorted; name: "arrow_downward"; font.pixelSize: 14; color: Theme.primary } }
    }
    RowLayout {
        anchors.fill: parent; spacing: 0
        PlacesPanel { Layout.fillHeight: true; Layout.preferredWidth: 248; current: "pictures" }
        ColumnLayout {
            Layout.fillWidth: true; Layout.fillHeight: true; Layout.rightMargin: 12; Layout.bottomMargin: 12; spacing: 8
            // Tabs
            RowLayout { Layout.fillWidth: true; Layout.topMargin: 10; spacing: 4
                Tab { label: "Wallpapers"; icon: "image"; cur: true }
                Tab { label: "Downloads"; icon: "download" }
                Tab { label: "opal-apps"; icon: "code" }
                MButton { icon: "add"; compact: true }
                Item { Layout.fillWidth: true }
                MButton { icon: "terminal"; compact: true; tooltip: "Open terminal here" }
                MButton { icon: "more_vert"; compact: true } }
            // Toolbar
            RowLayout { Layout.fillWidth: true; spacing: 6
                MButton { icon: "arrow_back"; compact: true }
                MButton { icon: "arrow_forward"; compact: true; enabled: false }
                MButton { icon: "arrow_upward"; compact: true }
                Breadcrumbs { path: F.path; Layout.leftMargin: 4 }
                Item { Layout.fillWidth: true }
                SearchField { Layout.preferredWidth: 280; implicitHeight: 40; placeholder: "Search Wallpapers" }
                SegmentedButtons { current: "list"; options: [{id: "list", icon: "view_list"}, {id: "grid", icon: "grid_view"}, {id: "cols", icon: "view_column"}] }
                MButton { icon: "vertical_split"; compact: true; tonal: true; tooltip: "Split view" } }
            // Split panes
            RowLayout {
                Layout.fillWidth: true; Layout.fillHeight: true; spacing: 8
                // Active pane: details list.
                Rectangle {
                    Layout.fillWidth: true; Layout.preferredWidth: 6; Layout.fillHeight: true; radius: 24; color: Theme.surfaceLow
                    border.width: 2; border.color: Theme.alpha(Theme.primary, .35)
                    ColumnLayout {
                        anchors.fill: parent; anchors.margins: 8; spacing: 0
                        RowLayout { Layout.fillWidth: true; Layout.preferredHeight: 36; Layout.leftMargin: 52; Layout.rightMargin: 16; spacing: 0
                            Item { Layout.preferredWidth: 0 }
                            HeaderCell { label: "Name"; Layout.fillWidth: true; Layout.rightMargin: 27 }
                            HeaderCell { label: "Size"; Layout.preferredWidth: 96 }
                            HeaderCell { label: "Modified"; sorted: true; Layout.preferredWidth: 120 }
                            HeaderCell { label: "Kind"; Layout.preferredWidth: 96 } }
                        Repeater {
                            model: F.files.slice(0, 15)
                            Rectangle {
                                id: r
                                required property var modelData
                                required property int index
                                readonly property bool sel: index >= 3 && index <= 5
                                readonly property bool cursor: index === 5
                                Layout.fillWidth: true; implicitHeight: 38
                                topLeftRadius: sel && index !== 3 ? 4 : 19; topRightRadius: topLeftRadius
                                bottomLeftRadius: sel && index !== 5 ? 4 : 19; bottomRightRadius: bottomLeftRadius
                                color: sel ? Theme.secondaryContainer : index === 8 ? Theme.alpha(Theme.text, .05) : "transparent"
                                RowLayout { anchors.fill: parent; anchors.leftMargin: 10; anchors.rightMargin: 16; spacing: 0
                                    FileGlyph { file: r.modelData; size: 28; Layout.rightMargin: 14 }
                                    MText { Layout.fillWidth: true; text: r.modelData.n; font.pixelSize: 13; font.weight: r.sel ? Font.DemiBold : Font.Normal; color: r.sel ? Theme.onSecondaryContainer : Theme.text }
                                    Glyph { visible: !!r.modelData.star; name: "star"; fill: 1; font.pixelSize: 15; color: Theme.tertiary; Layout.rightMargin: 12 }
                                    MText { Layout.preferredWidth: 96; text: r.modelData.s; role: "bodyS"; color: r.sel ? Theme.onSecondaryContainer : Theme.subtext }
                                    MText { Layout.preferredWidth: 120; text: r.modelData.m; role: "bodyS"; color: r.sel ? Theme.onSecondaryContainer : Theme.subtext }
                                    MText { Layout.preferredWidth: 96; text: F.kindName[r.modelData.k]; role: "bodyS"; color: r.sel ? Theme.onSecondaryContainer : Theme.subtext } }
                            }
                        }
                        Item { Layout.fillHeight: true }
                        RowLayout { Layout.fillWidth: true; Layout.preferredHeight: 32; Layout.leftMargin: 12; Layout.rightMargin: 12
                            MText { text: "16 items"; role: "label"; color: Theme.subtext }
                            Rectangle { implicitWidth: selText.implicitWidth + 20; implicitHeight: 24; radius: 12; color: Theme.secondaryContainer
                                MText { id: selText; anchors.centerIn: parent; text: "3 selected · 7.7 MB"; role: "labelS"; color: Theme.onSecondaryContainer } }
                            Item { Layout.fillWidth: true }
                            MText { text: "Zoom"; role: "labelS"; color: Theme.subtext }
                            ExpressiveSlider { compact: true; showIcon: false; Layout.preferredWidth: 120; value: 30 } }
                    }
                }
                // Second pane: Downloads as a grid.
                Rectangle {
                    Layout.fillWidth: true; Layout.preferredWidth: 4; Layout.fillHeight: true; radius: 24; color: Theme.surfaceLow
                    ColumnLayout {
                        anchors.fill: parent; anchors.margins: 14; spacing: 10
                        RowLayout { Layout.fillWidth: true
                            Glyph { name: "download"; font.pixelSize: 18; color: Theme.subtext }
                            MText { text: "Downloads"; role: "titleS"; color: Theme.subtext; Layout.fillWidth: true }
                            MText { text: "8 items"; role: "labelS"; color: Theme.muted } }
                        GridLayout {
                            Layout.fillWidth: true; columns: 3; rowSpacing: 14; columnSpacing: 10
                            Repeater {
                                model: F.downloads
                                ColumnLayout {
                                    id: cell
                                    required property var modelData
                                    Layout.fillWidth: true; spacing: 6
                                    FileGlyph { file: cell.modelData; size: 96; Layout.alignment: Qt.AlignHCenter }
                                    MText { Layout.fillWidth: true; horizontalAlignment: Text.AlignHCenter; text: cell.modelData.n; role: "labelS"; font.pixelSize: 12; elide: Text.ElideMiddle }
                                }
                            }
                        }
                        Item { Layout.fillHeight: true }
                    }
                }
            }
        }
    }
    OpPill { anchors.right: parent.right; anchors.bottom: parent.bottom; anchors.rightMargin: 36; anchors.bottomMargin: 36; width: 380 }
}
