import QtQuick
import QtQuick.Layouts
import Opal.Material
import Opal.Files
// Couch layout: console tabs on the shoulders, breadcrumbs, a shelf of places and big tiles; the controller drives a focus ring.
AppFrame {
    id: couch
    property var view
    destinations: [{title: qsTr("Home"), icon: "home"}, {title: qsTr("Recent"), icon: "schedule"}, {title: qsTr("Devices"), icon: "devices"}, {title: qsTr("Network"), icon: "lan"}]
    hints: [{b: "A", t: qsTr("Open")}, {b: "B", t: qsTr("Up")}, {b: "X", t: qsTr("Actions")}, {b: "Y", t: qsTr("Search")}, {b: "LT", t: qsTr("Page")}]
    onCurrentIndexChanged: pane.navigate(["file://" + couch.view.places.homePath, "recent:///", "computer:///", "network:///"][currentIndex] || pane.location)
    Component.onCompleted: { view.mobilePane = pane; pane.forceActiveFocus(); }
    Component.onDestruction: if (view.mobilePane === pane) view.mobilePane = null
    ColumnLayout {
        anchors.fill: parent; anchors.leftMargin: 72; anchors.rightMargin: 72; spacing: 20
        RowLayout { Layout.fillWidth: true; spacing: 16
            Breadcrumbs { path: pane.location, couch.view.crumbsFor(pane); location: pane.location; onNavigate: uri => pane.navigate(uri) }
            Item { Layout.fillWidth: true }
            MText { text: qsTr("%n item(s)", "", pane.model.count) + (couch.view.storageText ? " · " + couch.view.storageText : ""); font.pixelSize: 20; color: Theme.subtext } }
        ListView {
            Layout.fillWidth: true; Layout.preferredHeight: 116; orientation: ListView.Horizontal; spacing: 18; clip: true
            model: couch.view.places
            delegate: Rectangle {
                id: pl
                required property string name
                required property string uri
                required property string glyph
                required property string section
                readonly property bool cur: pane.location === uri
                visible: section !== "network"
                width: visible ? 176 : -18; height: 112; radius: 28
                color: cur ? Theme.secondaryContainer : Theme.surfaceHigh
                Column { anchors.left: parent.left; anchors.bottom: parent.bottom; anchors.margins: 18; spacing: 6
                    Glyph { name: pl.glyph || "folder"; fill: pl.cur ? 1 : 0; font.pixelSize: 34; color: pl.cur ? Theme.onSecondaryContainer : Theme.primary }
                    MText { width: 140; text: pl.name; font.pixelSize: 20; font.weight: Font.DemiBold; color: pl.cur ? Theme.onSecondaryContainer : Theme.text } }
                TapHandler { onTapped: pane.navigate(pl.uri) }
            }
        }
        MText { text: pane.folder.locationName; font.pixelSize: 40; font.weight: Font.Bold; font.variableAxes: ({"wdth": 115, "ROND": 80}) }
        BrowserPane { id: pane; Layout.fillWidth: true; Layout.fillHeight: true; viewMode: "grid"; focus: true
            Component.onCompleted: navigate(FilesEnv.startLocation.toString())
            onContextMenu: (row, from, x, y) => couch.view.showMenu(row, from, x, y)
            onOpenFile: uris => couch.view.openFiles(uris)
            Keys.onEscapePressed: up() }
    }
}
