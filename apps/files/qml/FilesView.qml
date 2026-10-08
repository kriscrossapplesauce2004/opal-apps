import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Opal.Material
import Opal.Files
// The whole Files interface. Desktop: places panel, tabs, toolbar, chips, one or two panes, status strip and the op pill.
// Phone: search pill, breadcrumbs, chips, list, FAB and bottom navigation. Couch: console tabs, places shelf and big tiles.
Item {
    id: root
    readonly property bool phone: Posture.touch && width < 600 || width < 520
    readonly property bool couch: Posture.couch
    property var tabs: [{location: FilesEnv.startLocation.toString()}]
    property int currentTab: 0
    property bool split: false
    property int activeSide: 0
    property string viewMode: "details"
    property real zoom: .35
    // Phone and couch layouts bring their own pane and register it here.
    property BrowserPane mobilePane: null
    property int paneTick: 0
    readonly property BrowserPane pane: paneTick >= 0 && (phone || couch) && mobilePane ? mobilePane : activeSide === 1 && split ? sidePane : (tabPanes.count > currentTab ? tabPanes.itemAt(currentTab) : null)
    readonly property var model: pane ? pane.model : null
    Places { id: places }
    property alias places: places
    property alias dialogs: dialogs
    FileInfo { id: launcher; onFailed: m => toast.show(m) }
    // Storage meter for the disk under the current folder.
    property string storageText: ""
    property real storageUsed: -1
    Connections { target: places; function onStorageInfoReady(location, total, free, used, text) { root.storageText = text; root.storageUsed = total > 0 ? used / total : -1; }
        function onError(title, message) { toast.show(title + ": " + message); }
        function onServerConnected(uri) { root.navigate(uri); } }
    Connections { target: root.pane ? root.pane.folder : null; function onLocationChanged() { places.storageInfo(root.pane.location); root.syncTab(); } }
    Connections { target: FileOperations
        function onFailed(title, message) { toast.show(title + (message ? ": " + message : "")); }
        function onCreated(uri) { if (root.pendingRename) { root.pendingRename = false; renameWhenListed.uri = uri; renameWhenListed.tries = 0; renameWhenListed.start(); } } }
    // The monitor lists a new item a moment after it is made, so the inline rename waits for its row.
    Timer {
        id: renameWhenListed
        property string uri: ""
        property int tries: 0
        interval: 50; repeat: true
        onTriggered: { const p = root.pane; const r = p ? p.model.indexOfUri(uri) : -1; if (r >= 0) { stop(); p.model.select(r, FolderModel.Replace); p.current = r; p.renaming = r; } else if (++tries > 60) stop(); }
    }
    property bool pendingRename: false

    function navigate(uri) { if (pane) pane.navigate(uri); }
    function syncTab() { const t = tabs.slice(); if (tabPanes.itemAt(currentTab)) { t[currentTab] = {location: tabPanes.itemAt(currentTab).location}; tabs = t; } }
    function tabInfo(loc) {
        const u = loc || "";
        const home = "file://" + places.homePath;
        if (u === home || u === home + "/") return {name: qsTr("Home"), glyph: "home"};
        if (u.startsWith("trash:")) return {name: qsTr("Trash"), glyph: "delete"};
        if (u.startsWith("recent:")) return {name: qsTr("Recent"), glyph: "schedule"};
        if (u.startsWith("starred:")) return {name: qsTr("Starred"), glyph: "star"};
        const parts = decodeURIComponent(u.replace(/\/$/, "")).split("/");
        return {name: parts[parts.length - 1] || u, glyph: u.startsWith("file:") ? "folder" : "cloud"};
    }
    function newTab(loc) { const t = tabs.slice(); t.push({location: loc || pane.location}); tabs = t; currentTab = t.length - 1; }
    function closeTab(i) { if (tabs.length < 2) return; const t = tabs.slice(); t.splice(i, 1); tabs = t; currentTab = Math.min(currentTab, t.length - 1); }
    function selection() { return model ? model.selectedUris() : []; }
    function copy(cut) { const s = selection(); if (s.length) { SystemBridge.setClipboard(s, cut); toast.show(cut ? qsTr("Cut %n item(s)", "", s.length) : qsTr("Copied %n item(s)", "", s.length)); } }
    function paste() { if (!SystemBridge.hasClipboardFiles || !pane) return; if (SystemBridge.clipboardCut) { FileOperations.move(SystemBridge.clipboardUris, pane.location); SystemBridge.clearClipboard(); } else FileOperations.copy(SystemBridge.clipboardUris, pane.location); }
    function trash() { const s = selection(); if (s.length) FileOperations.trash(s); }
    function newFolder() { pendingRename = true; FileOperations.createFolder(pane.location); }
    function newFile() { pendingRename = true; FileOperations.createFile(pane.location); }
    function openFiles(uris) { launcher.launchDefault(uris); }
    function act(id) {
        const s = selection();
        switch (id) {
        case "open": pane.openSelection(); break;
        case "newtab": { const r = model.indexOfUri(s[0]); if (r >= 0 && model.get(r).isDir) newTab(s[0]); break; }
        case "copy": copy(false); break;
        case "cut": copy(true); break;
        case "paste": paste(); break;
        case "trash": trash(); break;
        case "delete": dialogs.confirmDelete(s); break;
        case "rename": if (s.length === 1) pane.renaming = model.indexOfUri(s[0]); break;
        case "star": for (const u of s) Starred.toggle(u); break;
        case "compress": FileOperations.compress(s, pane.location, s.length === 1 ? tabInfo(s[0]).name : qsTr("Archive")); break;
        case "extract": FileOperations.extract(s, pane.location, true); break;
        case "terminal": SystemBridge.openTerminal(decodeURIComponent((s.length === 1 && model.get(model.indexOfUri(s[0])).isDir ? s[0] : pane.location).replace("file://", ""))); break;
        case "copypath": SystemBridge.copyText(s.map(u => decodeURIComponent(u.replace("file://", ""))).join("\n")); toast.show(qsTr("Path copied")); break;
        case "properties": dialogs.showProperties(s.length ? s[0] : pane.location); break;
        case "restore": FileOperations.restore(s); break;
        case "emptytrash": FileOperations.emptyTrash(); break;
        case "newfolder": newFolder(); break;
        case "newfile": newFile(); break;
        case "selectall": model.selectAll(); break;
        case "split": split = !split; break;
        default: if (id.startsWith("with:")) { for (const u of s) launcher.launch(u, id.slice(5)); }
        }
    }
    FileInfo { id: menuInfo }
    function menuEntries(row) {
        const inTrash = pane.location.startsWith("trash:");
        if (row < 0) return [
            {id: "newfolder", t: qsTr("New folder"), i: "create_new_folder", k: "Ctrl+Shift+N"}, {id: "newfile", t: qsTr("New file"), i: "note_add"},
            {id: "paste", t: qsTr("Paste"), i: "content_paste", k: "Ctrl+V", disabled: !SystemBridge.hasClipboardFiles}, {sep: true},
            {id: "selectall", t: qsTr("Select all"), i: "select_all", k: "Ctrl+A"}, {id: "terminal", t: qsTr("Open terminal here"), i: "terminal", disabled: SystemBridge.terminalName === ""},
            {id: "split", t: split ? qsTr("Close split view") : qsTr("Split view"), i: "vertical_split", k: "F3"}, {sep: true}, {id: "properties", t: qsTr("Folder properties"), i: "info"}];
        const e = model.get(row);
        menuInfo.uri = e.uri;
        const out = [{id: "open", t: e.isDir ? qsTr("Open") : qsTr("Open with %1").arg(menuInfo.openWith.length ? menuInfo.openWith[0].name : qsTr("default app")), i: "open_in_new", k: "Enter"}];
        if (e.isDir) out.push({id: "newtab", t: qsTr("Open in new tab"), i: "tab"});
        if (!e.isDir) for (const a of menuInfo.openWith.slice(1, 4)) out.push({id: "with:" + a.id, t: qsTr("Open with %1").arg(a.name), i: "apps"});
        out.push({sep: true});
        if (inTrash) out.push({id: "restore", t: qsTr("Restore"), i: "restore_from_trash"}, {id: "delete", t: qsTr("Delete for good"), i: "delete_forever", danger: true});
        else out.push({id: "cut", t: qsTr("Cut"), i: "content_cut", k: "Ctrl+X"}, {id: "copy", t: qsTr("Copy"), i: "content_copy", k: "Ctrl+C"},
            {id: "rename", t: qsTr("Rename"), i: "edit", k: "F2", disabled: model.selectedCount !== 1}, {id: "star", t: e.starred ? qsTr("Unstar") : qsTr("Star"), i: "star"},
            {id: "copypath", t: qsTr("Copy path"), i: "link"});
        if (FileOperations.archiveSupported && !inTrash) out.push(e.kind === "archive" ? {id: "extract", t: qsTr("Extract here"), i: "unarchive"} : {id: "compress", t: qsTr("Compress"), i: "folder_zip"});
        if (e.isDir) out.push({id: "terminal", t: qsTr("Open terminal here"), i: "terminal", disabled: SystemBridge.terminalName === ""});
        if (!inTrash) out.push({sep: true}, {id: "trash", t: qsTr("Move to Trash"), i: "delete", k: "Del", danger: true});
        out.push({id: "properties", t: qsTr("Properties"), i: "info", k: "Alt+Enter"});
        return out;
    }
    function showMenu(row, from, x, y) { menu.entries = menuEntries(row); menu.openAt(from, x, y); }

    // Shortcuts (desktop and keyboards on phones).
    Shortcut { sequences: [StandardKey.Copy]; onActivated: root.copy(false) }
    Shortcut { sequences: [StandardKey.Cut]; onActivated: root.copy(true) }
    Shortcut { sequences: [StandardKey.Paste]; onActivated: root.paste() }
    Shortcut { sequence: "Delete"; onActivated: root.trash() }
    Shortcut { sequence: "Shift+Delete"; onActivated: dialogs.confirmDelete(root.selection()) }
    Shortcut { sequences: [StandardKey.Undo]; enabled: FileOperations.canUndo; onActivated: { toast.show(qsTr("Undo: %1").arg(FileOperations.undoText)); FileOperations.undo(); } }
    Shortcut { sequence: "Ctrl+T"; onActivated: root.newTab() }
    Shortcut { sequence: "Ctrl+W"; onActivated: root.closeTab(root.currentTab) }
    Shortcut { sequence: "Ctrl+Tab"; onActivated: root.currentTab = (root.currentTab + 1) % root.tabs.length }
    Shortcut { sequence: "Ctrl+Shift+Tab"; onActivated: root.currentTab = (root.currentTab + root.tabs.length - 1) % root.tabs.length }
    Shortcut { sequence: "Alt+Left"; onActivated: root.pane.back() }
    Shortcut { sequence: "Alt+Right"; onActivated: root.pane.forward() }
    Shortcut { sequence: "Alt+Up"; onActivated: root.pane.up() }
    Shortcut { sequence: "Alt+Home"; onActivated: root.navigate("file://" + places.homePath) }
    Shortcut { sequence: "Ctrl+H"; onActivated: root.pane.folder.showHidden = !root.pane.folder.showHidden }
    Shortcut { sequences: ["Ctrl+F", "Ctrl+K"]; onActivated: searchField.input.forceActiveFocus() }
    Shortcut { sequence: "Ctrl+L"; onActivated: crumbs.startEdit() }
    Shortcut { sequence: "F5"; onActivated: root.pane.folder.refresh() }
    Shortcut { sequence: "F3"; onActivated: root.split = !root.split }
    Shortcut { sequence: "Ctrl+Shift+N"; onActivated: root.newFolder() }
    Shortcut { sequence: "Alt+Return"; onActivated: root.act("properties") }
    Shortcut { sequence: "Ctrl+1"; onActivated: root.viewMode = "details" }
    Shortcut { sequence: "Ctrl+2"; onActivated: root.viewMode = "grid" }
    Shortcut { sequence: "Ctrl+3"; onActivated: root.viewMode = "columns" }
    Shortcut { sequences: ["Ctrl+=", "Ctrl++"]; onActivated: root.zoom = Math.min(1, root.zoom + .1) }
    Shortcut { sequence: "Ctrl+-"; onActivated: root.zoom = Math.max(0, root.zoom - .1) }
    Shortcut { sequence: "Ctrl+Alt+T"; onActivated: root.act("terminal") }

    // Desktop layout.
    Rectangle {
        anchors.fill: parent
        visible: !root.phone && !root.couch
        color: Theme.base
        RowLayout {
            anchors.fill: parent; spacing: 0
            PlacesPanel {
                id: placesPanel
                visible: root.width >= 840
                Layout.fillHeight: true; Layout.preferredWidth: 248
                places: places; current: root.pane ? root.pane.location : ""
                storageText: root.storageText; storageUsed: root.storageUsed
                onNavigate: uri => root.navigate(uri)
                onConnectRequested: dialogs.connectServer()
                onNewRequested: from => { menu.entries = [{id: "newfolder", t: qsTr("New folder"), i: "create_new_folder", k: "Ctrl+Shift+N"}, {id: "newfile", t: qsTr("New empty file"), i: "note_add"}]; menu.openAt(from, 0, from.height + 6); }
            }
            NavRail {
                visible: root.width < 840
                Layout.fillHeight: true; Layout.preferredWidth: 96
                destinations: [{title: qsTr("Home"), icon: "home"}, {title: qsTr("Recent"), icon: "schedule"}, {title: qsTr("Starred"), icon: "star"}, {title: qsTr("Trash"), icon: "delete"}]
                currentIndex: -1; fabIcon: "create_new_folder"
                onPicked: i => root.navigate(["file://" + places.homePath, "recent:///", "starred:///", "trash:///"][i])
                onFabClicked: root.newFolder()
            }
            ColumnLayout {
                // A small preferred width stops long paths from widening the column past the window.
                Layout.fillWidth: true; Layout.preferredWidth: 100; Layout.fillHeight: true; Layout.rightMargin: 12; Layout.bottomMargin: 12; spacing: 8
                RowLayout { Layout.fillWidth: true; Layout.topMargin: 10; spacing: 4
                    TabsBar { tabs: root.tabs.map(t => root.tabInfo(t.location)); current: root.currentTab; onPicked: i => root.currentTab = i; onClosed: i => root.closeTab(i); onAdded: root.newTab() }
                    Item { Layout.fillWidth: true }
                    MButton { visible: FileOperations.canUndo; icon: "undo"; compact: true; tooltip: qsTr("Undo %1").arg(FileOperations.undoText); onClicked: FileOperations.undo() }
                    MButton { icon: "terminal"; compact: true; tooltip: qsTr("Open terminal here"); enabled: SystemBridge.terminalName !== ""; onClicked: root.act("terminal") }
                    MButton { icon: "more_vert"; compact: true; onClicked: root.showMenu(-1, this, 0, height) } }
                RowLayout { Layout.fillWidth: true; spacing: 6
                    MButton { icon: "arrow_back"; compact: true; enabled: root.pane && root.pane.canBack; onClicked: root.pane.back(); tooltip: qsTr("Back") }
                    MButton { icon: "arrow_forward"; compact: true; enabled: root.pane && root.pane.canForward; onClicked: root.pane.forward(); tooltip: qsTr("Forward") }
                    MButton { icon: "arrow_upward"; compact: true; enabled: root.pane && root.pane.folder.canGoUp; onClicked: root.pane.up(); tooltip: qsTr("Up") }
                    Breadcrumbs { id: crumbs; Layout.leftMargin: 4; Layout.preferredWidth: Math.min(implicitWidth, root.width * .3); Layout.minimumWidth: 120
                        location: root.pane ? root.pane.location : ""
                        path: root.pane ? (root.pane.location, root.crumbsFor(root.pane)) : []
                        onNavigate: uri => root.navigate(uri) }
                    Item { Layout.fillWidth: true }
                    SearchField { id: searchField; Layout.preferredWidth: 280; Layout.minimumWidth: 160; implicitHeight: 40; placeholder: root.pane ? qsTr("Search %1").arg(root.pane.folder.locationName) : ""
                        onTextChanged: { if (!root.pane) return; root.pane.search.query = text; if (text.length > 1) root.pane.search.start(); else root.pane.search.cancel(); }
                        input.Keys.onDownPressed: root.pane.forceActiveFocus()
                        input.Keys.onEscapePressed: { text = ""; root.pane.forceActiveFocus(); } }
                    SegmentedButtons { current: root.viewMode; onPicked: id => root.viewMode = id
                        options: [{id: "details", icon: "view_list"}, {id: "grid", icon: "grid_view"}, {id: "columns", icon: "view_column"}] }
                    MButton { icon: "vertical_split"; compact: true; tonal: root.split; tooltip: qsTr("Split view (F3)"); onClicked: root.split = !root.split } }
                FilterChips { Layout.fillWidth: true; Layout.preferredWidth: 100; clip: true; folder: root.pane ? root.pane.folder : null; visible: root.pane && !root.pane.searching }
                RowLayout {
                    Layout.fillWidth: true; Layout.fillHeight: true; spacing: 8
                    Rectangle {
                        Layout.fillWidth: true; Layout.preferredWidth: 6; Layout.fillHeight: true; radius: 24; color: Theme.surfaceLow
                        border.width: root.split && root.activeSide === 0 ? 2 : 0; border.color: Theme.alpha(Theme.primary, .35)
                        ColumnLayout {
                            anchors.fill: parent; anchors.margins: 8; spacing: 0
                            Item {
                                Layout.fillWidth: true; Layout.fillHeight: true
                                Repeater {
                                    id: tabPanes
                                    model: root.tabs.length
                                    onItemAdded: Qt.callLater(() => root.paneTick++)
                                    BrowserPane {
                                        required property int index
                                        anchors.fill: parent
                                        visible: index === root.currentTab
                                        active: !root.split || root.activeSide === 0
                                        viewMode: root.viewMode; zoom: root.zoom
                                        focus: visible
                                        Component.onCompleted: navigate(root.tabs[index].location)
                                        onActivated: root.activeSide = 0
                                        onContextMenu: (row, from, x, y) => root.showMenu(row, from, x, y)
                                        onOpenFile: uris => root.openFiles(uris)
                                    }
                                }
                            }
                            StatusStrip { Layout.fillWidth: true; Layout.preferredHeight: 32; Layout.leftMargin: 12; Layout.rightMargin: 12; model: root.model; zoom: root.zoom; onZoomMoved: v => root.zoom = v }
                        }
                    }
                    Rectangle {
                        visible: root.split
                        Layout.fillWidth: true; Layout.preferredWidth: 5; Layout.fillHeight: true; radius: 24; color: Theme.surfaceLow
                        border.width: root.activeSide === 1 ? 2 : 0; border.color: Theme.alpha(Theme.primary, .35)
                        ColumnLayout { anchors.fill: parent; anchors.margins: 8; spacing: 6
                            RowLayout { Layout.fillWidth: true; Layout.leftMargin: 8; spacing: 6
                                Glyph { name: root.tabInfo(sidePane.location).glyph; font.pixelSize: 18; color: root.activeSide === 1 ? Theme.primary : Theme.subtext }
                                MText { Layout.fillWidth: true; text: sidePane.folder.locationName; role: "titleS"; color: root.activeSide === 1 ? Theme.text : Theme.subtext }
                                MText { text: qsTr("%n item(s)", "", sidePane.model.count); role: "labelS"; color: Theme.muted }
                                MButton { icon: "close"; compact: true; onClicked: root.split = false } }
                            BrowserPane { id: sidePane; Layout.fillWidth: true; Layout.fillHeight: true; viewMode: root.viewMode === "columns" ? "details" : root.viewMode; zoom: root.zoom
                                active: root.activeSide === 1
                                onActivated: root.activeSide = 1
                                onContextMenu: (row, from, x, y) => root.showMenu(row, from, x, y)
                                onOpenFile: uris => root.openFiles(uris)
                                Component.onCompleted: navigate("file://" + places.homePath) } }
                    }
                }
            }
        }
        OpPill { anchors.right: parent.right; anchors.bottom: parent.bottom; anchors.rightMargin: 36; anchors.bottomMargin: 56; width: 380; onOpenSheet: dialogs.showOperations() }
        SelectionBar {
            visible: root.viewMode === "grid" && root.model && root.model.selectedCount > 1
            anchors.horizontalCenter: parent.horizontalCenter; anchors.bottom: parent.bottom; anchors.bottomMargin: 56
            count: root.model ? root.model.selectedCount : 0
            actions: [{id: "copy", i: "content_copy", t: qsTr("Copy")}, {id: "cut", i: "content_cut", t: qsTr("Cut")}, {id: "star", i: "star", t: qsTr("Star")}, {id: "compress", i: "folder_zip", t: qsTr("Compress")}, {id: "trash", i: "delete", t: qsTr("Trash")}]
            onTriggered: id => root.act(id); onCleared: root.model.clearSelection()
        }
    }
    function crumbsFor(p) {
        const list = p.folder.breadcrumbs();
        const home = "file://" + places.homePath;
        const i = list.findIndex(c => c.uri === home || c.uri === home + "/");
        const out = (i >= 0 ? list.slice(i) : list).map(c => ({name: c.name, uri: c.uri}));
        if (i >= 0) { out[0].name = qsTr("Home"); out[0].glyph = "home"; }
        return out;
    }

    // Phone layout.
    Loader { anchors.fill: parent; active: root.phone && !root.couch; sourceComponent: PhoneFiles { view: root } }
    // Couch layout.
    Loader { anchors.fill: parent; active: root.couch; sourceComponent: CouchFiles { view: root } }

    FileMenu { id: menu; parent: root; onTriggered: id => root.act(id) }
    Dialogs { id: dialogs; places: places }
    // Small transient message at the bottom.
    Rectangle {
        id: toast
        function show(t) { label.text = t; opacity = 1; hide.restart(); }
        anchors.horizontalCenter: parent.horizontalCenter; anchors.bottom: parent.bottom; anchors.bottomMargin: 128
        implicitWidth: label.implicitWidth + 36; implicitHeight: 44; radius: 22; color: Theme.inverseSurface
        opacity: 0; visible: opacity > 0; z: 60
        Behavior on opacity { NumberAnimation { duration: Theme.motion(Theme.durMedium) } }
        MText { id: label; anchors.centerIn: parent; role: "label"; color: Theme.inverseInk }
        Timer { id: hide; interval: 2600; onTriggered: toast.opacity = 0 }
    }
}
