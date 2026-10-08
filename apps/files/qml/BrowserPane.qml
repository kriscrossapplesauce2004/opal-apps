import QtQuick
import QtQuick.Controls
import Opal.Material
import Opal.Files
// One browsing pane: a folder model with history, search, three view modes, selection, keyboard handling and drops.
FocusScope {
    id: browser
    property alias folder: folderModel
    property alias search: searchModel
    property string viewMode: "details"
    property bool active: true
    property real zoom: .35
    property bool searching: searchModel.query !== ""
    readonly property var model: searching ? searchModel : folderModel
    property var history: []
    property int historyIndex: -1
    property int selTick: 0
    property int renaming: -1
    signal activated()
    signal contextMenu(int row, Item from, real x, real y)
    signal openFile(var uris)

    FolderModel { id: folderModel; thumbnailSize: 256; onSelectionChanged: browser.selTick++ }
    Search { id: searchModel; root: folderModel.location; onSelectionChanged: browser.selTick++ }
    readonly property string location: folderModel.location.toString()
    readonly property bool canBack: historyIndex > 0
    readonly property bool canForward: historyIndex < history.length - 1

    function navigate(uri, push) {
        if (!uri) return;
        const u = uri.toString();
        searchModel.query = "";
        if (push !== false && u !== location) {
            const h = history.slice(0, historyIndex + 1);
            h.push(u);
            history = h;
            historyIndex = h.length - 1;
        }
        folderModel.location = u;
        renaming = -1;
    }
    function back() { if (canBack) { historyIndex--; folderModel.location = history[historyIndex]; searchModel.query = ""; } }
    function forward() { if (canForward) { historyIndex++; folderModel.location = history[historyIndex]; searchModel.query = ""; } }
    function up() { if (folderModel.canGoUp) navigate(folderModel.parentLocation()); }
    function openRow(row) {
        const e = model.get(row);
        if (!e || !e.uri) return;
        if (e.isDir) navigate(e.uri); else openFile([e.uri]);
    }
    function openSelection() {
        const uris = model.selectedUris();
        if (uris.length === 1) { const r = model.indexOfUri(uris[0]); if (r >= 0) { openRow(r); return; } }
        const files = uris.filter(u => { const r = model.indexOfUri(u); return r >= 0 && !model.get(r).isDir; });
        if (files.length) openFile(files);
    }
    function isSelected(row) { return browser.selTick >= 0 && row >= 0 && row < model.count && model.get(row).selected; }
    function clickRow(row, mods) {
        browser.activated();
        browser.forceActiveFocus();
        if (mods & Qt.ShiftModifier) model.select(row, FolderModel.Range);
        else if (mods & Qt.ControlModifier) model.select(row, FolderModel.Toggle);
        else model.select(row, FolderModel.Replace);
        current = row;
    }
    property int current: -1
    function moveCurrent(delta, mods) {
        if (!model.count) return;
        const r = Math.max(0, Math.min(model.count - 1, (current < 0 ? 0 : current + delta)));
        if (mods & Qt.ShiftModifier) model.select(r, FolderModel.Range); else model.select(r, FolderModel.Replace);
        current = r;
        if (view.item && view.item.positionAt) view.item.positionAt(r);
    }
    // Type-to-select: letters jump to the next name with that prefix.
    property string typed: ""
    Timer { id: typedReset; interval: 900; onTriggered: browser.typed = "" }
    function typeSelect(text) {
        typed += text.toLowerCase(); typedReset.restart();
        for (let i = 0; i < model.count; i++) { const n = model.get(i).name.toLowerCase(); if (n.startsWith(typed)) { model.select(i, FolderModel.Replace); current = i; if (view.item && view.item.positionAt) view.item.positionAt(i); return; } }
    }
    readonly property int columns: view.item && view.item.columns ? view.item.columns : 1
    Keys.onPressed: e => {
        const ctrl = e.modifiers & Qt.ControlModifier;
        if (renaming >= 0) return;
        if (e.key === Qt.Key_Down) { moveCurrent(viewMode === "grid" ? columns : 1, e.modifiers); e.accepted = true; }
        else if (e.key === Qt.Key_Up) { moveCurrent(viewMode === "grid" ? -columns : -1, e.modifiers); e.accepted = true; }
        else if (e.key === Qt.Key_Right && viewMode === "grid") { moveCurrent(1, e.modifiers); e.accepted = true; }
        else if (e.key === Qt.Key_Left && viewMode === "grid") { moveCurrent(-1, e.modifiers); e.accepted = true; }
        else if (e.key === Qt.Key_Right && viewMode !== "grid" && current >= 0 && model.get(current).isDir) { openRow(current); e.accepted = true; }
        else if (e.key === Qt.Key_Left && viewMode !== "grid") { up(); e.accepted = true; }
        else if (e.key === Qt.Key_Home) { moveCurrent(-model.count, e.modifiers); e.accepted = true; }
        else if (e.key === Qt.Key_End) { moveCurrent(model.count, e.modifiers); e.accepted = true; }
        else if (e.key === Qt.Key_Return || e.key === Qt.Key_Enter) { openSelection(); e.accepted = true; }
        else if (e.key === Qt.Key_Backspace) { up(); e.accepted = true; }
        else if (e.key === Qt.Key_Escape && searching) { searchModel.query = ""; e.accepted = true; }
        else if (e.key === Qt.Key_Escape) { model.clearSelection(); e.accepted = true; }
        else if (e.key === Qt.Key_F2 && model.selectedCount === 1) { renaming = model.indexOfUri(model.selectedUris()[0]); e.accepted = true; }
        else if (e.key === Qt.Key_Menu) { const r = current >= 0 ? current : -1; contextMenu(r, browser, width / 2, height / 2); e.accepted = true; }
        else if (ctrl && e.key === Qt.Key_A) { model.selectAll(); e.accepted = true; }
        else if (!ctrl && !(e.modifiers & Qt.AltModifier) && e.text.length === 1 && e.text.trim() !== "") { typeSelect(e.text); e.accepted = true; }
    }

    Loader {
        id: view
        anchors.fill: parent
        sourceComponent: browser.viewMode === "grid" ? gridView : browser.viewMode === "columns" ? columnsView : browser.viewMode === "phone" ? phoneView : detailsView
    }
    Component { id: detailsView; DetailsList { pane: browser } }
    Component { id: gridView; IconGrid { pane: browser } }
    Component { id: columnsView; ColumnsView { pane: browser } }
    Component { id: phoneView; PhoneList { pane: browser } }

    // Empty and error states.
    Column {
        anchors.centerIn: parent; spacing: 10
        visible: !folderModel.loading && browser.model.count === 0
        Glyph { anchors.horizontalCenter: parent.horizontalCenter; name: folderModel.error ? "error" : browser.searching ? "search_off" : "folder_open"; font.pixelSize: 56; color: Theme.muted }
        MText { anchors.horizontalCenter: parent.horizontalCenter; text: folderModel.error ? folderModel.error : browser.searching ? (searchModel.running ? qsTr("Searching…") : qsTr("Nothing found")) : qsTr("This folder is empty"); role: "title"; color: Theme.subtext }
    }

    // Drops from other panes and other apps: move with Shift, link with Ctrl+Shift, else copy (move within one disk).
    DropArea {
        anchors.fill: parent
        keys: ["text/uri-list"]
        onDropped: drop => {
            const target = browser.searching ? "" : browser.location;
            if (!target) return;
            const uris = drop.urls.map(u => u.toString()).filter(u => u !== target);
            if (!uris.length) return;
            if (drop.proposedAction === Qt.MoveAction || (drop.keys && (drop.modifiers & Qt.ShiftModifier))) FileOperations.move(uris, target);
            else FileOperations.copy(uris, target);
            drop.acceptProposedAction();
        }
    }
    TapHandler { acceptedButtons: Qt.LeftButton; onTapped: { browser.activated(); browser.forceActiveFocus(); } }
}
