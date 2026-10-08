import QtQuick
import QtQuick.Layouts
import Opal.Material
import Opal.Files
// Details view: sortable Name / Size / Modified / Kind columns, grouped selection, hover, inline rename and drag out.
Item {
    id: details
    property var pane
    readonly property var model: pane.model
    function positionAt(r) { list.positionViewAtIndex(r, ListView.Contain); }
    readonly property real rowH: Theme.couch ? 64 : Theme.touch ? 52 : 38 + pane.zoom * 10
    component HeaderCell: Item {
        property string label; property int role: -1
        readonly property bool sorted: !details.pane.searching && details.pane.folder.sortRole === role
        implicitHeight: 34
        Row { anchors.verticalCenter: parent.verticalCenter; spacing: 4
            MText { text: label; role: "label"; color: sorted ? Theme.primary : Theme.subtext; font.weight: sorted ? Font.Bold : Font.Medium }
            Glyph { visible: sorted; name: details.pane.folder.sortOrder === Qt.AscendingOrder ? "arrow_upward" : "arrow_downward"; font.pixelSize: 14; color: Theme.primary } }
        TapHandler { enabled: role >= 0; onTapped: { const f = details.pane.folder; if (f.sortRole === role) f.sortOrder = f.sortOrder === Qt.AscendingOrder ? Qt.DescendingOrder : Qt.AscendingOrder; else { f.sortRole = role; f.sortOrder = role === FolderModel.Name || role === FolderModel.Kind ? Qt.AscendingOrder : Qt.DescendingOrder; } } }
        Accessible.role: Accessible.ColumnHeader; Accessible.name: label
    }
    RowLayout {
        id: header
        anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top
        anchors.leftMargin: 52; anchors.rightMargin: 16; height: 34; spacing: 0
        HeaderCell { label: details.pane.searching ? qsTr("Name · folder") : qsTr("Name"); role: FolderModel.Name; Layout.fillWidth: true }
        HeaderCell { label: qsTr("Size"); role: FolderModel.Size; Layout.preferredWidth: 96 }
        HeaderCell { label: qsTr("Modified"); role: FolderModel.Modified; Layout.preferredWidth: 130 }
        HeaderCell { label: qsTr("Kind"); role: FolderModel.Kind; Layout.preferredWidth: 110 }
    }
    ListView {
        id: list
        anchors.fill: parent; anchors.topMargin: header.height
        clip: true; boundsBehavior: Flickable.StopAtBounds
        model: details.model
        reuseItems: true
        cacheBuffer: 400
        onContentYChanged: thumbs.restart()
        onHeightChanged: thumbs.restart()
        onCountChanged: thumbs.restart()
        Timer { id: thumbs; interval: 60; onTriggered: { const f = list.indexAt(10, list.contentY + 2), l = list.indexAt(10, list.contentY + list.height - 2); details.model.requestThumbnails(Math.max(0, f), l < 0 ? details.model.count - 1 : l); } }
        delegate: Rectangle {
            id: row
            required property int index
            required property string name
            required property string uri
            required property bool isDir
            required property bool isSymlink
            required property string sizeText
            required property string modifiedText
            required property string kind
            required property string kindName
            required property url thumbnail
            required property bool selected
            required property bool starred
            required property int childCount
            required property string parentPath
            readonly property bool prevSel: selected && details.pane.isSelected(index - 1)
            readonly property bool nextSel: selected && details.pane.isSelected(index + 1)
            readonly property real r: details.rowH / 2
            width: list.width; height: details.rowH
            topLeftRadius: prevSel ? 4 : r; topRightRadius: topLeftRadius
            bottomLeftRadius: nextSel ? 4 : r; bottomRightRadius: bottomLeftRadius
            color: selected ? (details.pane.active ? Theme.secondaryContainer : Theme.alpha(Theme.secondaryContainer, .55)) : hover.hovered ? Theme.alpha(Theme.text, .05) : "transparent"
            readonly property color ink: selected ? Theme.onSecondaryContainer : Theme.text
            readonly property color sub: selected ? Theme.onSecondaryContainer : Theme.subtext
            RowLayout {
                anchors.fill: parent; anchors.leftMargin: 10; anchors.rightMargin: 16; spacing: 0
                FileMark { kind: row.kind; thumbnail: row.thumbnail; symlink: row.isSymlink; size: details.rowH - 10; Layout.rightMargin: 14 }
                Item { Layout.fillWidth: true; implicitHeight: details.rowH
                    MText { visible: details.pane.renaming !== row.index; anchors.verticalCenter: parent.verticalCenter; width: parent.width - 8; text: details.pane.searching ? row.name + "   " + row.parentPath : row.name; font.pixelSize: 13; font.weight: row.selected ? Font.DemiBold : Font.Normal; color: row.ink; elide: Text.ElideMiddle }
                    Loader { active: details.pane.renaming === row.index; anchors.verticalCenter: parent.verticalCenter; width: Math.min(parent.width - 8, 420)
                        sourceComponent: RenameField { uri: row.uri; original: row.name; onDone: { details.pane.renaming = -1; details.pane.forceActiveFocus(); } } } }
                Glyph { visible: row.starred; name: "star"; fill: 1; font.pixelSize: 15; color: Theme.tertiary; Layout.rightMargin: 12 }
                MText { Layout.preferredWidth: 96; text: row.isDir ? (row.childCount >= 0 ? qsTr("%n item(s)", "", row.childCount) : "") : row.sizeText; role: "bodyS"; color: row.sub }
                MText { Layout.preferredWidth: 130; text: row.modifiedText; role: "bodyS"; color: row.sub }
                MText { Layout.preferredWidth: 110; text: row.kindName; role: "bodyS"; color: row.sub; elide: Text.ElideRight }
            }
            HoverHandler { id: hover }
            MouseArea {
                anchors.fill: parent
                acceptedButtons: Qt.LeftButton | Qt.RightButton | Qt.MiddleButton
                drag.target: dragProxy
                onPressed: e => { if (e.button === Qt.RightButton && !row.selected) details.pane.clickRow(row.index, 0); else if (e.button === Qt.LeftButton && !row.selected) details.pane.clickRow(row.index, e.modifiers); }
                onClicked: e => { if (e.button === Qt.RightButton) details.pane.contextMenu(row.index, row, e.x, e.y); else if (e.button === Qt.LeftButton && row.selected && !(e.modifiers & (Qt.ShiftModifier | Qt.ControlModifier))) details.pane.clickRow(row.index, 0); else if (e.button === Qt.LeftButton && (e.modifiers & Qt.ControlModifier) && row.selected && e.wasHeld === false) {} }
                onDoubleClicked: e => { if (e.button === Qt.LeftButton) details.pane.openRow(row.index); }
                onPressAndHold: e => details.pane.contextMenu(row.index, row, e.x, e.y)
            }
            // Drag proxy: carries the selection as a uri list to other panes and apps.
            Item {
                id: dragProxy
                Drag.active: parent.children[parent.children.length - 2].drag.active
                Drag.dragType: Drag.Automatic
                Drag.supportedActions: Qt.CopyAction | Qt.MoveAction | Qt.LinkAction
                Drag.mimeData: ({"text/uri-list": details.model.selectedUris().join("\r\n")})
                Drag.keys: ["text/uri-list"]
            }
            Accessible.role: Accessible.ListItem; Accessible.name: name; Accessible.selected: selected
        }
    }
}
