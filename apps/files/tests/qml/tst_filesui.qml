import QtQuick
import QtTest
import Opal.Files

// Drives the real Files interface: loading, opening, history, selection, tabs, views, new folder, copy and paste, trash.
Item {
    id: host
    width: 1440; height: 900
    Component { id: viewComponent; FilesView { anchors.fill: parent } }
    TestCase {
        name: "FilesUi"
        when: windowShown
        property var view
        function init() {
            view = createTemporaryObject(viewComponent, host);
            verify(view);
            view.navigate(fixtureDir);
            tryVerify(() => view.pane && view.pane.location === fixtureDir && !view.pane.folder.loading && view.model.count === 3, 5000);
        }
        function rowOf(name) { return view.model.indexOfUri(fixtureDir + "/" + name); }
        function test_load() {
            // Folders come first.
            compare(view.model.get(0).name, "Folder");
            compare(view.model.get(1).name, "alpha.txt");
        }
        function test_openAndBack() {
            view.pane.openRow(0);
            tryCompare(view.pane, "location", fixtureDir + "/Folder", 3000);
            tryCompare(view.model, "count", 1, 3000);
            verify(view.pane.canBack);
            view.pane.back();
            tryCompare(view.pane, "location", fixtureDir, 3000);
            view.pane.forward();
            tryCompare(view.pane, "location", fixtureDir + "/Folder", 3000);
            view.pane.up();
            tryCompare(view.pane, "location", fixtureDir, 3000);
        }
        function test_selection() {
            view.pane.clickRow(1, 0);
            compare(view.model.selectedCount, 1);
            view.pane.clickRow(2, Qt.ShiftModifier);
            compare(view.model.selectedCount, 2);
            view.pane.clickRow(1, Qt.ControlModifier);
            compare(view.model.selectedCount, 1);
            view.model.selectAll();
            compare(view.model.selectedCount, 3);
        }
        function test_tabs() {
            view.newTab(fixtureDir + "/Folder");
            compare(view.tabs.length, 2);
            compare(view.currentTab, 1);
            tryVerify(() => view.pane && view.pane.location === fixtureDir + "/Folder", 3000);
            view.closeTab(1);
            compare(view.tabs.length, 1);
            tryVerify(() => view.pane.location === fixtureDir, 3000);
        }
        function test_viewModes() {
            for (const m of ["grid", "columns", "details"]) {
                view.viewMode = m;
                wait(50);
                compare(view.pane.viewMode, m);
            }
        }
        function test_newFolderStartsRename() {
            view.act("newfolder");
            tryCompare(view.model, "count", 4, 5000);
            tryVerify(() => view.pane.renaming >= 0, 3000);
            const made = view.model.get(view.pane.renaming);
            compare(made.isDir, true);
            verify(FileOperations.canUndo);
            view.pane.renaming = -1;
            // Undo of a create goes through the Trash, which /tmp fixtures may not have; remove it directly.
            FileOperations.remove([made.uri]);
            tryCompare(view.model, "count", 3, 5000);
        }
        function test_copyPaste() {
            view.pane.clickRow(rowOf("alpha.txt"), 0);
            view.act("copy");
            tryVerify(() => SystemBridge.hasClipboardFiles && !SystemBridge.clipboardCut, 2000);
            view.navigate(fixtureDir + "/Folder");
            tryCompare(view.model, "count", 1, 3000);
            view.act("paste");
            tryCompare(view.model, "count", 2, 5000);
            verify(view.model.indexOfUri(fixtureDir + "/Folder/alpha.txt") >= 0);
            // Leave the fixture as it was for the other tests.
            FileOperations.remove([fixtureDir + "/Folder/alpha.txt"]);
            tryCompare(view.model, "count", 1, 5000);
        }
        function test_contextMenuEntries() {
            const file = view.menuEntries(rowOf("beta.txt"));
            verify(file.some(e => e.id === "trash"));
            verify(file.some(e => e.id === "rename"));
            const bg = view.menuEntries(-1);
            verify(bg.some(e => e.id === "newfolder"));
        }
    }
}
