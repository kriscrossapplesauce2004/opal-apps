import QtQuick
import Opal.Files
// Render check: grid view with a selection and the floating selection bar.
FilesView { viewMode: "grid"; Timer { interval: 1500; running: true; onTriggered: { parent.model.select(4, 0); parent.model.select(5, 1); parent.model.select(6, 1); } } }
