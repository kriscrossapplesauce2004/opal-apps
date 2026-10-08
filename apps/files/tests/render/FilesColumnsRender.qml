import QtQuick
import Opal.Files
// Render check: columns view with one image selected for the preview pane.
FilesView { viewMode: "columns"; Timer { interval: 1500; running: true; onTriggered: parent.model.select(parent.model.indexOfUri(parent.pane.location + "/dr5-keyart.png"), 0) } }
