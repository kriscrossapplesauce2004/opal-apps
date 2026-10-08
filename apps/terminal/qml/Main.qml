import QtQuick
import Opal.Material
import Opal.TerminalUi
// Opal Terminal window. A see-through window colour lets terminal.conf opacity show the desktop behind.
AppWindow {
    id: window
    seed: "#3e8e7e"
    allowCouch: false
    title: qsTr("Terminal")
    color: "transparent"
    TerminalView { anchors.fill: parent }
}
