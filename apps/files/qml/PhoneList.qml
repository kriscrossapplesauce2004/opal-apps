import QtQuick
import QtQuick.Layouts
import Opal.Material
import Opal.Files
// Phone list: 72 px rows with 52 px thumbnails; tap opens, long press starts selection; in selection mode taps toggle.
Item {
    id: phone
    property var pane
    readonly property var model: pane.model
    readonly property bool selecting: model.selectedCount > 0
    function positionAt(r) { list.positionViewAtIndex(r, ListView.Contain); }
    ListView {
        id: list
        anchors.fill: parent; leftMargin: 8; rightMargin: 8; bottomMargin: 96
        clip: true; spacing: 2
        model: phone.model
        onContentYChanged: thumbs.restart()
        onCountChanged: thumbs.restart()
        Timer { id: thumbs; interval: 60; onTriggered: { const f = list.indexAt(20, list.contentY + 2), l = list.indexAt(20, list.contentY + list.height - 2); phone.model.requestThumbnails(Math.max(0, f), l < 0 ? phone.model.count - 1 : l); } }
        delegate: Rectangle {
            id: row
            required property int index
            required property string name
            required property bool isDir
            required property string kind
            required property string sizeText
            required property string modifiedText
            required property url thumbnail
            required property bool selected
            required property bool starred
            required property int childCount
            width: list.width - 16; height: 72; radius: 24
            color: selected ? Theme.secondaryContainer : "transparent"
            RowLayout { anchors.fill: parent; anchors.leftMargin: 8; anchors.rightMargin: 4; spacing: 14
                Item { implicitWidth: 52; implicitHeight: 52
                    FileMark { anchors.fill: parent; kind: row.kind; thumbnail: row.thumbnail; size: 52 }
                    Rectangle { visible: row.selected; anchors.fill: parent; radius: 15; color: Theme.alpha(Theme.primary, .85)
                        Glyph { anchors.centerIn: parent; name: "check"; font.pixelSize: 26; color: Theme.onPrimary } } }
                ColumnLayout { Layout.fillWidth: true; spacing: 2
                    MText { Layout.fillWidth: true; text: row.name; role: "titleS"; font.weight: Font.Medium; elide: Text.ElideMiddle; color: row.selected ? Theme.onSecondaryContainer : Theme.text }
                    MText { Layout.fillWidth: true; text: (row.isDir ? (row.childCount >= 0 ? qsTr("%n item(s)", "", row.childCount) : qsTr("Folder")) : row.sizeText) + " · " + row.modifiedText; role: "bodyS"; color: Theme.subtext } }
                Glyph { visible: row.starred; name: "star"; fill: 1; font.pixelSize: 18; color: Theme.tertiary }
                MButton { visible: !phone.selecting; icon: "more_vert"; compact: true; onClicked: phone.pane.contextMenu(row.index, row, row.width - 40, 36) } }
            TapHandler { onTapped: phone.selecting ? phone.pane.clickRow(row.index, Qt.ControlModifier) : phone.pane.openRow(row.index)
                onLongPressed: phone.pane.clickRow(row.index, Qt.ControlModifier) }
            Accessible.role: Accessible.ListItem; Accessible.name: name; Accessible.selected: selected
        }
    }
}
