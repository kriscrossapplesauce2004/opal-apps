import QtQuick
import QtQuick.Layouts
import Opal.Material
import Opal.Files
// Grid view: rounded thumbnails or glyph tiles; selected tiles shrink and round with a check badge.
Item {
    id: grid
    property var pane
    readonly property var model: pane.model
    readonly property real edge: Theme.couch ? 280 : 96 + pane.zoom * 160
    readonly property int columns: Math.max(1, Math.floor(view.width / view.cellWidth))
    function positionAt(r) { view.positionViewAtIndex(r, GridView.Contain); }
    GridView {
        id: view
        anchors.fill: parent; anchors.margins: 8
        clip: true; boundsBehavior: Flickable.StopAtBounds
        model: grid.model
        cellWidth: grid.edge + 18; cellHeight: grid.edge * .78 + 58
        reuseItems: true
        onContentYChanged: thumbs.restart()
        onHeightChanged: thumbs.restart()
        onCountChanged: thumbs.restart()
        Timer { id: thumbs; interval: 60; onTriggered: { const f = view.indexAt(10, view.contentY + 2), l = view.indexAt(view.width - 10, view.contentY + view.height - 2); grid.model.requestThumbnails(Math.max(0, f), l < 0 ? grid.model.count - 1 : l); } }
        delegate: Item {
            id: tile
            required property int index
            required property string name
            required property string uri
            required property bool isDir
            required property string kind
            required property string sizeText
            required property url thumbnail
            required property bool selected
            required property bool starred
            readonly property bool focused: tile.index === grid.pane.current && (Posture.controllerActive || Posture.couch)
            width: view.cellWidth; height: view.cellHeight
            Item {
                id: art
                x: 9; y: 4; width: grid.edge; height: grid.edge * .78
                Rectangle { anchors.fill: parent; anchors.margins: -5; radius: 28; color: "transparent"; border.width: 3; border.color: Theme.primary; visible: tile.focused }
                Rectangle {
                    anchors.centerIn: parent
                    width: parent.width * (tile.selected ? .9 : 1); height: parent.height * (tile.selected ? .88 : 1)
                    radius: tile.selected ? 28 : 18
                    color: tile.thumbnail.toString() !== "" ? Theme.surfaceHighest : Theme.alpha(Theme.surfaceHigh, .9)
                    Behavior on width { NumberAnimation { duration: Theme.motion(Theme.durShort) } }
                    Behavior on height { NumberAnimation { duration: Theme.motion(Theme.durShort) } }
                    RoundedImage { anchors.fill: parent; radius: parent.radius; visible: tile.thumbnail.toString() !== ""; source: tile.thumbnail; sourceSize: Qt.size(grid.edge * 2, grid.edge * 1.6) }
                    FileMark { anchors.centerIn: parent; visible: tile.thumbnail.toString() === ""; kind: tile.kind; size: Math.min(parent.width, parent.height) * .62; color: "transparent" }
                }
                Rectangle { visible: tile.selected || grid.model.selectedCount > 0 && hover.hovered; x: 2; y: 2; width: 26; height: 26; radius: 13; color: tile.selected ? Theme.primary : Qt.rgba(0, 0, 0, .35); border.width: 2; border.color: tile.selected ? Theme.base : "#ffffff"
                    Glyph { anchors.centerIn: parent; visible: tile.selected; name: "check"; font.pixelSize: 17; color: Theme.onPrimary }
                    TapHandler { onTapped: grid.pane.clickRow(tile.index, Qt.ControlModifier) } }
                Rectangle { visible: tile.starred; anchors.right: parent.right; anchors.bottom: parent.bottom; anchors.margins: 8; width: 26; height: 26; radius: 13; color: Qt.rgba(0, 0, 0, .45)
                    Glyph { anchors.centerIn: parent; name: "star"; fill: 1; font.pixelSize: 16; color: "#ffd86b" } }
            }
            ColumnLayout {
                anchors.top: art.bottom; anchors.topMargin: tile.focused ? 10 : 6; x: 9; width: grid.edge; spacing: 0
                Item { Layout.fillWidth: true; implicitHeight: nameText.implicitHeight
                    MText { id: nameText; visible: grid.pane.renaming !== tile.index; width: parent.width; text: tile.name; role: "label"; font.pixelSize: Theme.couch ? 19 : 13; font.weight: tile.selected || tile.focused ? Font.DemiBold : Font.Medium; elide: Text.ElideMiddle; horizontalAlignment: Text.AlignHCenter; color: tile.selected || tile.focused ? Theme.primary : Theme.text }
                    Loader { active: grid.pane.renaming === tile.index; width: parent.width
                        sourceComponent: RenameField { uri: tile.uri; original: tile.name; onDone: { grid.pane.renaming = -1; grid.pane.forceActiveFocus(); } } } }
                MText { visible: !Theme.couch; Layout.fillWidth: true; text: tile.isDir ? "" : tile.sizeText; role: "labelS"; color: Theme.subtext; horizontalAlignment: Text.AlignHCenter }
            }
            HoverHandler { id: hover }
            MouseArea {
                id: area
                anchors.fill: parent
                acceptedButtons: Qt.LeftButton | Qt.RightButton
                drag.target: proxy
                onPressed: e => { if (!tile.selected) grid.pane.clickRow(tile.index, e.button === Qt.RightButton ? 0 : e.modifiers); }
                onClicked: e => { if (e.button === Qt.RightButton) grid.pane.contextMenu(tile.index, tile, e.x, e.y); else if (tile.selected && !(e.modifiers & (Qt.ShiftModifier | Qt.ControlModifier))) grid.pane.clickRow(tile.index, 0); }
                onDoubleClicked: grid.pane.openRow(tile.index)
                onPressAndHold: e => { if (Posture.touch) grid.pane.clickRow(tile.index, Qt.ControlModifier); else grid.pane.contextMenu(tile.index, tile, e.x, e.y); }
            }
            Item { id: proxy; Drag.active: area.drag.active; Drag.dragType: Drag.Automatic; Drag.supportedActions: Qt.CopyAction | Qt.MoveAction | Qt.LinkAction
                Drag.mimeData: ({"text/uri-list": grid.model.selectedUris().join("\r\n")}); Drag.keys: ["text/uri-list"] }
            Accessible.role: Accessible.ListItem; Accessible.name: name; Accessible.selected: selected
        }
    }
}
