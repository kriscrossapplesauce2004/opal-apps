import QtQuick
import Opal.Material
import Opal.Files

// Debug window for the Files backend (opal-files --debug).
AppWindow {
    title: "Opal Files (debug)"
    seed: "#4a7fb5"
    DebugView { anchors.fill: parent }
}
