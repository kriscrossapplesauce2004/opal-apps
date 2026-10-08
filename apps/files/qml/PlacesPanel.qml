import QtQuick
import QtQuick.Layouts
import Opal.Material
// Places sidebar bound to the Places model: sections, devices with eject, Trash, and the storage meter of the current disk.
Item {
    id: panel
    property var places
    property string current: ""
    property string storageText: ""
    property real storageUsed: -1
    property bool showFab: true
    signal navigate(string uri)
    signal newRequested(Item from)
    signal connectRequested()
    implicitWidth: 248
    readonly property var sectionNames: ({places: "", folders: qsTr("Folders"), bookmarks: qsTr("Bookmarks"), devices: qsTr("Devices"), network: qsTr("Network"), trash: ""})
    ColumnLayout {
        anchors.fill: parent; anchors.margins: 12; anchors.topMargin: 14; spacing: 0
        Fab { id: fab; visible: panel.showFab; size: "small"; icon: "create_new_folder"; text: qsTr("New"); Layout.bottomMargin: 14; Layout.leftMargin: 4; onClicked: panel.newRequested(fab) }
        ListView {
            id: list
            Layout.fillWidth: true; Layout.fillHeight: true
            clip: true; boundsBehavior: Flickable.StopAtBounds
            model: panel.places
            section.property: "section"
            section.delegate: Item {
                required property string section
                width: list.width; height: section === "trash" || section === "places" ? 17 : 34
                Rectangle { visible: section === "trash"; anchors.verticalCenter: parent.verticalCenter; x: 16; width: parent.width - 32; height: 1; color: Theme.outline }
                MText { visible: section !== "trash" && section !== "places"; anchors.bottom: parent.bottom; anchors.bottomMargin: 4; x: 16; text: panel.sectionNames[section] || section; role: "label"; color: Theme.subtext }
            }
            delegate: Rectangle {
                id: place
                required property string name
                required property string uri
                required property string glyph
                required property string section
                required property string detail
                required property bool canEject
                required property bool canUnmount
                required property bool mounted
                required property int index
                readonly property bool cur: uri === panel.current || (uri.length > 8 && panel.current.startsWith(uri + "/") && section !== "places")
                width: list.width; height: Theme.touch ? 48 : 40; radius: height / 2
                color: cur ? Theme.secondaryContainer : hover.hovered ? Theme.alpha(Theme.text, .05) : "transparent"
                RowLayout { anchors.fill: parent; anchors.leftMargin: 14; anchors.rightMargin: 6; spacing: 12
                    Glyph { name: place.glyph || "folder"; fill: place.cur ? 1 : 0; font.pixelSize: 20; color: place.cur ? Theme.onSecondaryContainer : Theme.subtext }
                    MText { Layout.fillWidth: true; text: place.name; role: "titleS"; font.pixelSize: 14; font.weight: place.cur ? Font.Bold : Font.Medium; color: place.cur ? Theme.onSecondaryContainer : Theme.text }
                    MText { visible: place.detail !== "" && !place.canEject && (place.section === "devices" || place.section === "trash" || place.section === "network"); text: place.detail; role: "labelS"; color: Theme.muted }
                    MButton { visible: place.canEject || (place.canUnmount && place.section === "network"); icon: "eject"; compact: true; implicitWidth: 32; implicitHeight: 32; tooltip: qsTr("Eject")
                        onClicked: place.canEject ? panel.places.ejectPlace(place.index) : panel.places.unmountPlace(place.index) } }
                HoverHandler { id: hover }
                TapHandler { onTapped: { if (place.uri === "network:///add" || place.glyph === "add_link") panel.connectRequested(); else if (!place.mounted && place.section === "devices") panel.places.mountPlace(place.index); else panel.navigate(place.uri); } }
                Accessible.role: Accessible.ListItem; Accessible.name: place.name; Accessible.selected: cur
            }
        }
        Rectangle {
            Layout.fillWidth: true; Layout.topMargin: 8; implicitHeight: 76; radius: 20; color: Theme.surface
            visible: panel.storageUsed >= 0
            ColumnLayout { anchors.fill: parent; anchors.margins: 14; spacing: 8
                RowLayout { Layout.fillWidth: true
                    Glyph { name: "hard_drive"; font.pixelSize: 18; color: Theme.primary }
                    MText { Layout.fillWidth: true; text: qsTr("This disk"); role: "label"; font.weight: Font.DemiBold }
                    MText { text: panel.storageText; role: "labelS"; color: Theme.subtext } }
                Item { Layout.fillWidth: true; implicitHeight: 8
                    Rectangle { width: Math.max(8, parent.width * panel.storageUsed - 3); height: 8; radius: 4; color: panel.storageUsed > .9 ? Theme.error : Theme.primary }
                    Rectangle { x: parent.width * panel.storageUsed + 3; width: Math.max(0, parent.width - x); height: 8; radius: 4; color: Theme.secondaryContainer } } }
        }
    }
}
