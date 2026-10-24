import QtQuick
import QtQuick.Layouts
import Opal.Material
import Opal.Media
// Docked mini player for phones and narrow windows: the bottom bar's surface, fully rounded, progress along its top edge. Tap to open.
Rectangle {
    id: m
    signal expand()
    visible: Player.hasMedia
    implicitHeight: 68
    radius: 24
    color: Theme.surface
    readonly property real p: Player.duration > 0 ? Player.position / Player.duration : 0
    Rectangle { x: 24; y: 0; width: (parent.width - 48) * m.p; height: 3; radius: 2; color: Theme.primary; visible: Player.current.kind !== "radio" }
    Rectangle { x: 24 + (parent.width - 48) * m.p; y: 0; width: (parent.width - 48) * (1 - m.p); height: 3; radius: 2; color: Theme.alpha(Theme.subtext, .25); visible: Player.current.kind !== "radio" }
    RowLayout {
        anchors.fill: parent; anchors.leftMargin: 12; anchors.rightMargin: 8; anchors.topMargin: 4; spacing: 12
        Cover { Layout.preferredWidth: 46; Layout.preferredHeight: 46; r: 12; source: Player.current.cover || ""; title: Player.current.album || Player.current.title || ""; icon: Player.current.kind === "radio" ? "radio" : "" }
        ColumnLayout {
            Layout.fillWidth: true; spacing: 0
            MText { Layout.fillWidth: true; text: Player.current.nowPlaying || Player.current.title || ""; role: "titleS"; elide: Text.ElideRight }
            MText { Layout.fillWidth: true; text: Player.current.kind === "radio" ? Player.current.title || "" : Player.current.artist || ""; role: "labelS"; color: Theme.subtext; elide: Text.ElideRight }
        }
        MButton { visible: Player.current.kind !== "radio"; icon: "skip_previous"; compact: true; tooltip: qsTr("Previous"); onClicked: Player.previous() }
        MButton { icon: Player.playing ? "pause" : "play_arrow"; compact: true; tooltip: Player.playing ? qsTr("Pause") : qsTr("Play"); onClicked: Player.toggle() }
        MButton { visible: Player.current.kind !== "radio"; icon: "skip_next"; compact: true; tooltip: qsTr("Next"); onClicked: Player.next() }
    }
    TapHandler { onTapped: m.expand() }
    DragHandler { target: null; yAxis.enabled: true; xAxis.enabled: false; onActiveChanged: if (!active && centroid.position.y < centroid.pressPosition.y - 30) m.expand() }
    Accessible.role: Accessible.Button; Accessible.name: qsTr("Now playing: %1").arg(Player.current.title || "")
}
