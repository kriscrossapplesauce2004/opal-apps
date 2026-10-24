import QtQuick
import QtQuick.Layouts
import Opal.Material
import Opal.Files
// Miller columns: the parent folders of the current one on the left, the current folder active, and a preview of the selection.
Item {
    id: cols
    property var pane
    readonly property var crumbs: pane.folder.location && pane.folder.breadcrumbs ? pane.folder.breadcrumbs() : []
    readonly property var parents: crumbs.slice(Math.max(0, crumbs.length - 3), crumbs.length - 1)
    function positionAt(r) { current.positionViewAtIndex(r, ListView.Contain); }
    component ColumnList: ListView {
        id: lv
        property string highlightName: ""
        clip: true; boundsBehavior: Flickable.StopAtBounds; spacing: 1
        delegate: Rectangle {
            id: it
            required property int index
            required property string name
            required property string uri
            required property bool isDir
            required property string kind
            required property url thumbnail
            readonly property bool sel: name === lv.highlightName
            width: lv.width; height: 36; radius: 18
            color: sel ? Theme.secondaryContainer : hov.hovered ? Theme.alpha(Theme.text, .05) : "transparent"
            RowLayout { anchors.fill: parent; anchors.leftMargin: 8; anchors.rightMargin: 8; spacing: 10
                FileMark { kind: it.kind; thumbnail: it.thumbnail; size: 26 }
                MText { Layout.fillWidth: true; text: it.name; font.pixelSize: 13; font.weight: it.sel ? Font.DemiBold : Font.Normal; color: it.sel ? Theme.onSecondaryContainer : Theme.text }
                Glyph { visible: it.isDir; name: "chevron_right"; font.pixelSize: 18; color: Theme.muted } }
            HoverHandler { id: hov }
            TapHandler { onTapped: if (it.isDir) cols.pane.navigate(it.uri) }
        }
    }
    RowLayout {
        anchors.fill: parent; spacing: 6
        Repeater {
            model: cols.parents
            Rectangle {
                id: pc
                required property var modelData
                required property int index
                Layout.fillHeight: true; Layout.preferredWidth: 230; radius: 22; color: Theme.alpha(Theme.surfaceLow, .6)
                FolderModel { id: pm; location: pc.modelData.uri }
                ColumnList { anchors.fill: parent; anchors.margins: 6; model: pm; highlightName: cols.crumbs[cols.crumbs.length - cols.parents.length + pc.index] ? cols.crumbs[cols.crumbs.length - cols.parents.length + pc.index].name : "" }
            }
        }
        Rectangle {
            Layout.fillHeight: true; Layout.preferredWidth: 260; radius: 22; color: Theme.surfaceLow
            border.width: 2; border.color: Theme.alpha(Theme.primary, .35)
            ListView {
                id: current
                anchors.fill: parent; anchors.margins: 6; clip: true; spacing: 1
                model: cols.pane.model
                delegate: Rectangle {
                    id: c
                    required property int index
                    required property string name
                    required property string uri
                    required property bool isDir
                    required property string kind
                    required property url thumbnail
                    required property bool selected
                    width: current.width; height: 36; radius: 18
                    color: selected ? Theme.primary : hc.hovered ? Theme.alpha(Theme.text, .05) : "transparent"
                    RowLayout { anchors.fill: parent; anchors.leftMargin: 8; anchors.rightMargin: 8; spacing: 10
                        FileMark { kind: c.kind; thumbnail: c.thumbnail; size: 26 }
                        MText { Layout.fillWidth: true; text: c.name; font.pixelSize: 13; font.weight: c.selected ? Font.DemiBold : Font.Normal; color: c.selected ? Theme.onPrimary : Theme.text }
                        Glyph { visible: c.isDir; name: "chevron_right"; font.pixelSize: 18; color: c.selected ? Theme.onPrimary : Theme.muted } }
                    HoverHandler { id: hc }
                    MouseArea { anchors.fill: parent; acceptedButtons: Qt.LeftButton | Qt.RightButton
                        onClicked: e => { cols.pane.clickRow(c.index, e.modifiers); if (e.button === Qt.RightButton) cols.pane.contextMenu(c.index, c, e.x, e.y); }
                        onDoubleClicked: cols.pane.openRow(c.index) }
                }
            }
        }
        // Preview of the single selected item.
        Rectangle {
            Layout.fillWidth: true; Layout.fillHeight: true; radius: 26; color: Theme.surface
            readonly property var sel: cols.pane.selTick >= 0 && cols.pane.model.selectedCount === 1 ? cols.pane.model.get(cols.pane.model.indexOfUri(cols.pane.model.selectedUris()[0])) : null
            FileInfo { id: info; uri: parent.sel ? parent.sel.uri : "" }
            ColumnLayout {
                anchors.fill: parent; anchors.margins: 18; spacing: 12
                visible: parent.sel !== null
                Item { Layout.fillWidth: true; Layout.preferredHeight: Math.min(width * .62, 360)
                    RoundedImage { anchors.fill: parent; radius: 22; visible: !!parent.parent.parent.sel && parent.parent.parent.sel.kind === "image"; source: parent.parent.parent.sel && parent.parent.parent.sel.kind === "image" ? parent.parent.parent.sel.uri : ""; fillMode: Image.PreserveAspectFit; sourceSize: Qt.size(900, 600) }
                    FileMark { anchors.centerIn: parent; visible: !parent.parent.parent.sel || parent.parent.parent.sel.kind !== "image"; kind: parent.parent.parent.sel ? parent.parent.parent.sel.kind : "other"; size: 120 } }
                MText { Layout.fillWidth: true; text: info.name; role: "headline"; elide: Text.ElideMiddle }
                GridLayout { columns: 2; columnSpacing: 20; rowSpacing: 8; Layout.fillWidth: true
                    Repeater { model: [[qsTr("Kind"), info.kindName], [qsTr("Size"), info.sizeText], [qsTr("Dimensions"), info.dimensionsText], [qsTr("Modified"), info.modifiedText], [qsTr("Permissions"), info.permissionsText], [qsTr("Where"), info.path]]
                        RowLayout { required property var modelData; visible: modelData[1] !== ""; Layout.columnSpan: 2; spacing: 0
                            MText { Layout.preferredWidth: 100; text: modelData[0]; role: "bodyS"; color: Theme.subtext }
                            MText { Layout.fillWidth: true; text: modelData[1]; role: "bodyS"; elide: Text.ElideMiddle } } } }
                Item { Layout.fillHeight: true }
                RowLayout { spacing: 8
                    MButton { text: qsTr("Open"); icon: "open_in_new"; filled: true; onClicked: cols.pane.openSelection() }
                    MButton { icon: "content_copy"; compact: true; tooltip: qsTr("Copy"); enabled: !!info.uri; onClicked: SystemBridge.setClipboard([info.uri], false) } }
            }
        }
    }
}
