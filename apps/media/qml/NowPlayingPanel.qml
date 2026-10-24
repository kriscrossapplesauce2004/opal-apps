import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Opal.Material
import Opal.Media
// The desktop's right panel: art, title, wavy seek, transport, output and what plays next. Click the art for the full player.
Rectangle {
    id: panel
    signal expand()
    signal openAlbum(string key)
    radius: 32
    color: Theme.surface
    ColumnLayout {
        anchors.fill: parent; anchors.margins: 20; spacing: 12
        RowLayout {
            Layout.fillWidth: true
            MText { text: Player.current.kind === "radio" ? qsTr("Live radio") : Player.current.kind === "book" ? qsTr("Listening") : qsTr("Now playing"); role: "label"; color: Theme.primary; Layout.fillWidth: true }
            Chip { text: outputName(); icon: "speaker"; trailingIcon: "expand_more"; glass: false; onClicked: outputMenu.popup()
                function outputName() { const o = Player.outputs.find(x => x.id === Player.output); return o ? o.name.split(" ").slice(0, 2).join(" ") : qsTr("This PC"); } }
        }
        Menu {
            id: outputMenu
            Repeater { model: Player.outputs
                MenuItem { required property var modelData; text: modelData.name; checkable: true; checked: modelData.id === Player.output; onTriggered: Player.setOutput(modelData.id) } }
        }
        Cover {
            Layout.fillWidth: true; Layout.preferredHeight: Math.min(width, panel.height * .42); Layout.alignment: Qt.AlignHCenter
            r: 28; source: Player.current.cover || ""; title: Player.current.album || Player.current.title || ""; icon: Player.current.kind === "radio" ? "radio" : ""
            HoverHandler { cursorShape: Qt.PointingHandCursor }
            TapHandler { onTapped: panel.expand() }
            MButton { anchors.right: parent.right; anchors.top: parent.top; anchors.margins: 10; icon: "open_in_full"; compact: true; tonal: true; tooltip: qsTr("Full player"); onClicked: panel.expand() }
        }
        ColumnLayout {
            spacing: 2; Layout.fillWidth: true
            MText { Layout.fillWidth: true; text: Player.current.nowPlaying || Player.current.title || ""; role: "headline"; elide: Text.ElideRight; maximumLineCount: 2; wrapMode: Text.Wrap }
            MText { Layout.fillWidth: true; text: Player.current.kind === "radio" ? (Player.current.title || "") : [Player.current.artist, Player.current.album].filter(x => !!x).join(" · "); role: "body"; color: Theme.subtext; elide: Text.ElideRight
                TapHandler { enabled: !!Player.current.albumKey; onTapped: panel.openAlbum(Player.current.albumKey) }
                HoverHandler { enabled: !!Player.current.albumKey; cursorShape: Qt.PointingHandCursor } }
        }
        Transport { Layout.fillWidth: true; size: 56 }
        MText { visible: Player.queue.length > Player.index + 1; text: Player.current.kind === "book" ? qsTr("Next chapters") : qsTr("Up next"); role: "label"; color: Theme.subtext; Layout.topMargin: 4 }
        ListView {
            Layout.fillWidth: true; Layout.fillHeight: true; clip: true; spacing: 0; interactive: contentHeight > height
            model: Player.queue.slice(Player.index + 1, Player.index + 40)
            delegate: Rectangle {
                id: nx
                required property var modelData
                required property int index
                width: ListView.view.width; height: 40; radius: 12; color: nxHover.hovered ? Theme.alpha(Theme.text, .05) : "transparent"
                RowLayout { anchors.fill: parent; anchors.leftMargin: 6; anchors.rightMargin: 6; spacing: 12
                    MText { text: nx.modelData.number || (nx.index + 1); role: "labelS"; color: Theme.muted; Layout.preferredWidth: 20 }
                    MText { Layout.fillWidth: true; text: nx.modelData.title || ""; role: "bodyS"; elide: Text.ElideRight }
                    MText { text: nx.modelData.length || ""; role: "labelS"; color: Theme.subtext } }
                HoverHandler { id: nxHover; cursorShape: Qt.PointingHandCursor }
                TapHandler { onTapped: Player.jump(Player.index + 1 + nx.index) }
                Accessible.role: Accessible.ListItem; Accessible.name: modelData.title || ""
            }
        }
    }
}
