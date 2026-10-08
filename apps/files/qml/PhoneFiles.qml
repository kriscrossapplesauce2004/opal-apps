import QtQuick
import QtQuick.Layouts
import Opal.Material
import Opal.Files
// Phone layout: search pill, breadcrumb chips, filter chips, the list, a FAB and bottom navigation; selection mode swaps the top bar.
AppFrame {
    id: phone
    property var view
    destinations: [{title: qsTr("Browse"), icon: "folder"}, {title: qsTr("Recent"), icon: "schedule"}, {title: qsTr("Starred"), icon: "star"}, {title: qsTr("Devices"), icon: "devices"}]
    currentIndex: 0
    readonly property bool selecting: pane.model.selectedCount > 0
    onCurrentIndexChanged: { if (currentIndex === 0) pane.navigate(FilesEnv.startLocation.toString()); else if (currentIndex === 1) pane.navigate("recent:///"); else if (currentIndex === 2) pane.navigate("starred:///"); }
    Component.onCompleted: { view.mobilePane = pane; }
    Component.onDestruction: if (view.mobilePane === pane) view.mobilePane = null
    ColumnLayout {
        anchors.fill: parent; spacing: 10
        // Normal top: search pill. Selection: contextual bar.
        SearchField {
            visible: !phone.selecting
            Layout.fillWidth: true; Layout.margins: 16; Layout.topMargin: 14; Layout.bottomMargin: 0
            leadingIcon: "menu"; placeholder: qsTr("Search in %1").arg(pane.folder.locationName)
            onLeadingClicked: phone.currentIndex = 3
            onTextChanged: { pane.search.query = text; if (text.length > 1) pane.search.start(); }
            MButton { icon: "more_vert"; compact: true; onClicked: phone.view.showMenu(-1, this, 0, height) }
        }
        Rectangle {
            visible: phone.selecting
            Layout.fillWidth: true; Layout.preferredHeight: 64; color: Theme.surfaceHigh
            RowLayout { anchors.fill: parent; anchors.leftMargin: 4; anchors.rightMargin: 4; spacing: 4
                MButton { icon: "close"; compact: true; onClicked: pane.model.clearSelection() }
                MText { Layout.fillWidth: true; text: qsTr("%1 selected").arg(pane.model.selectedCount); role: "title" }
                MButton { icon: "select_all"; compact: true; onClicked: pane.model.selectAll() }
                MButton { icon: "more_vert"; compact: true; onClicked: phone.view.showMenu(pane.model.indexOfUri(pane.model.selectedUris()[0]), this, 0, height) } }
        }
        Flickable { visible: phone.currentIndex !== 3; Layout.fillWidth: true; Layout.preferredHeight: crumbs.height; contentWidth: crumbs.width + 32; clip: true
            Breadcrumbs { id: crumbs; x: 16; path: pane.location, phone.view.crumbsFor(pane); location: pane.location; onNavigate: uri => pane.navigate(uri) } }
        Flickable { visible: phone.currentIndex !== 3 && !pane.searching; Layout.fillWidth: true; Layout.preferredHeight: 40; contentWidth: chips.implicitWidth + 32; clip: true
            FilterChips { id: chips; x: 16; folder: pane.folder } }
        BrowserPane { id: pane; visible: phone.currentIndex !== 3; Layout.fillWidth: true; Layout.fillHeight: true; viewMode: "phone"; focus: true
            Component.onCompleted: navigate(FilesEnv.startLocation.toString())
            onContextMenu: (row, from, x, y) => phone.view.showMenu(row, from, x, y)
            onOpenFile: uris => phone.view.openFiles(uris) }
        // Devices tab: the places list full width.
        PlacesPanel { visible: phone.currentIndex === 3; Layout.fillWidth: true; Layout.fillHeight: true; showFab: false
            places: phone.view.places ? phone.view.places : null; current: pane.location; storageText: phone.view.storageText; storageUsed: phone.view.storageUsed
            onNavigate: uri => { pane.navigate(uri); phone.currentIndex = 0; } onConnectRequested: phone.view.dialogs.connectServer() }
    }
    Fab { visible: !phone.selecting && phone.currentIndex === 0; anchors.right: parent.right; anchors.bottom: parent.bottom; anchors.margins: 16; icon: "add"; onClicked: phone.view.showMenu(-1, this, -200, -120) }
    SelectionBar { visible: phone.selecting; anchors.horizontalCenter: parent.horizontalCenter; anchors.bottom: parent.bottom; anchors.bottomMargin: 16; showLabels: false; showCount: false
        actions: [{id: "copy", i: "content_copy", t: qsTr("Copy")}, {id: "cut", i: "drive_file_move", t: qsTr("Move")}, {id: "star", i: "star", t: qsTr("Star")}, {id: "trash", i: "delete", t: qsTr("Trash")}]
        onTriggered: id => phone.view.act(id) }
}
