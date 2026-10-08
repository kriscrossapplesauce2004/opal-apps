import QtQuick
import QtQuick.Layouts
import Opal.Material
import "Fixtures.js" as F
// Desktop sidebar: places in sections, devices with eject buttons, and a storage meter at the bottom.
Item {
    id: panel
    property string current: "pictures"
    property bool showFab: true
    implicitWidth: 248
    ColumnLayout {
        anchors.fill: parent; anchors.margins: 12; anchors.topMargin: 14; spacing: 0
        Fab { visible: panel.showFab; size: "small"; icon: "create_new_folder"; text: "New"; Layout.bottomMargin: 14; Layout.leftMargin: 4 }
        Repeater {
            model: F.places
            ColumnLayout {
                id: section
                required property var modelData
                required property int index
                Layout.fillWidth: true; spacing: 0
                Rectangle { visible: section.index > 0 && section.modelData.section === ""; Layout.fillWidth: true; Layout.preferredHeight: 1; Layout.topMargin: 8; Layout.bottomMargin: 8; Layout.leftMargin: 16; Layout.rightMargin: 16; color: Theme.outline }
                MText { visible: section.modelData.section !== ""; text: section.modelData.section; role: "label"; color: Theme.subtext; Layout.leftMargin: 16; Layout.topMargin: 14; Layout.bottomMargin: 4 }
                Repeater {
                    model: section.modelData.items
                    Rectangle {
                        id: place
                        required property var modelData
                        readonly property bool cur: modelData.id === panel.current
                        Layout.fillWidth: true; implicitHeight: 40; radius: 20
                        color: cur ? Theme.secondaryContainer : "transparent"
                        RowLayout { anchors.fill: parent; anchors.leftMargin: 14; anchors.rightMargin: 8; spacing: 12
                            Glyph { name: place.modelData.icon; fill: place.cur ? 1 : 0; font.pixelSize: 20; color: place.cur ? Theme.onSecondaryContainer : Theme.subtext }
                            MText { Layout.fillWidth: true; text: place.modelData.title; role: "titleS"; font.pixelSize: 14; font.weight: place.cur ? Font.Bold : Font.Medium; color: place.cur ? Theme.onSecondaryContainer : Theme.text }
                            MText { visible: !!place.modelData.detail && !place.modelData.eject; text: place.modelData.detail || ""; role: "labelS"; color: Theme.muted }
                            Rectangle { visible: !!place.modelData.badge; implicitWidth: 22; implicitHeight: 18; radius: 9; color: Theme.primary
                                MText { anchors.centerIn: parent; text: place.modelData.badge || ""; font.pixelSize: 11; font.weight: Font.Bold; color: Theme.onPrimary } }
                            Glyph { visible: !!place.modelData.eject; name: "eject"; font.pixelSize: 18; color: Theme.subtext } }
                    }
                }
            }
        }
        Item { Layout.fillHeight: true }
        // Storage meter for the disk that holds the current folder.
        Rectangle {
            Layout.fillWidth: true; implicitHeight: 76; radius: 20; color: Theme.surface
            ColumnLayout { anchors.fill: parent; anchors.margins: 14; spacing: 8
                RowLayout { Layout.fillWidth: true
                    Glyph { name: "hard_drive"; font.pixelSize: 18; color: Theme.primary }
                    MText { Layout.fillWidth: true; text: "Home"; role: "label"; font.weight: Font.DemiBold }
                    MText { text: "1.2 TB free"; role: "labelS"; color: Theme.subtext } }
                Item { Layout.fillWidth: true; implicitHeight: 8
                    Rectangle { width: parent.width * .41 - 3; height: 8; radius: 4; color: Theme.primary }
                    Rectangle { x: parent.width * .41 + 3; width: parent.width - x; height: 8; radius: 4; color: Theme.secondaryContainer } } }
        }
    }
}
