import QtQuick
import Opal.Material
// Opal Photos window. openPath opens one photo in the viewer at start (from a file manager).
AppWindow {
    id: win
    property string openPath: ""
    title: view.title
    seed: "#4f7cbf"
    width: 1440; height: 900
    PhotosView { id: view; anchors.fill: parent; openPath: win.openPath }
}
