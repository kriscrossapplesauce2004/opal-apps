import QtQuick
import Opal.Material
import Opal.Media
// Opal Media window. openFiles plays files handed over by a file manager.
AppWindow {
    id: win
    title: Player.hasMedia && Player.playing ? (Player.current.title || qsTr("Media")) + " · " + qsTr("Media") : qsTr("Media")
    seed: "#c2477b"
    width: 1440; height: 900
    function openFiles(tracks) { view.openFiles(tracks); }
    MediaView { id: view; anchors.fill: parent }
}
