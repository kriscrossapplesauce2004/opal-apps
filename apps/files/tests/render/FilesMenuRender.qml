import QtQuick
import Opal.Files
// Render check: the context menu on a file and the split view.
FilesView { id: v; split: true; Timer { interval: 1500; running: true; onTriggered: { v.model.select(4, 0); v.showMenu(4, v.pane, 300, 300); } } }
