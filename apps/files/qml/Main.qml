import QtQuick
import Opal.Material
// Opal Files window.
AppWindow {
    title: qsTr("Files")
    seed: "#c9a23c"
    width: 1440; height: 900
    FilesView { anchors.fill: parent }
}
