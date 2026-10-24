import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Opal.Material
import Opal.Media
// The queue: the playing track and what comes after it. Drag a row's handle to reorder; the close button removes it.
ListView {
    id: q
    property var s: null
    readonly property color ink: s ? s.on_surface : Theme.text
    readonly property color sub: s ? s.on_surface_variant : Theme.subtext
    readonly property color hi: s ? s.secondary_container : Theme.secondaryContainer
    clip: true; spacing: 2
    boundsBehavior: Flickable.StopAtBounds
    model: Player.queue
    ScrollBar.vertical: ScrollBar {}
    Component.onCompleted: positionViewAtIndex(Math.max(0, Player.index), ListView.Beginning)
    delegate: Rectangle {
        id: qr
        required property var modelData
        required property int index
        readonly property bool cur: index === Player.index
        width: q.width; height: 52; radius: 16
        color: cur ? q.hi : qHover.hovered ? Theme.alpha(q.ink, .05) : "transparent"
        opacity: index < Player.index ? .55 : 1
        RowLayout {
            anchors.fill: parent; anchors.leftMargin: 10; anchors.rightMargin: 4; spacing: 12
            Glyph { name: qr.cur ? (Player.playing ? "graphic_eq" : "pause") : "drag_indicator"; font.pixelSize: 20; color: qr.cur ? (q.s ? q.s.primary : Theme.primary) : q.sub
                DragHandler { id: dh; target: null; yAxis.enabled: true; xAxis.enabled: false
                    onActiveChanged: if (!active) { const to = Math.max(0, Math.min(Player.queue.length - 1, qr.index + Math.round(translation.y / qr.height))); Player.move(qr.index, to); } }
                HoverHandler { cursorShape: Qt.SizeVerCursor } }
            ColumnLayout { Layout.fillWidth: true; spacing: 0
                MText { Layout.fillWidth: true; text: qr.modelData.title || ""; role: "bodyS"; font.weight: qr.cur ? Font.Bold : Font.Normal; elide: Text.ElideRight; color: q.ink }
                MText { Layout.fillWidth: true; text: qr.modelData.artist || ""; role: "labelS"; elide: Text.ElideRight; color: q.sub } }
            MText { text: qr.modelData.length || ""; role: "labelS"; color: q.sub }
            MButton { visible: qHover.hovered && !qr.cur; icon: "close"; compact: true; tooltip: qsTr("Remove from the queue"); onClicked: Player.removeAt(qr.index) }
        }
        transform: Translate { y: dh.active ? dh.translation.y : 0 }
        z: dh.active ? 5 : 0
        HoverHandler { id: qHover }
        TapHandler { onTapped: Player.jump(qr.index) }
        Accessible.role: Accessible.ListItem; Accessible.name: modelData.title || ""
    }
    MText { anchors.centerIn: parent; visible: q.count === 0; text: qsTr("Nothing queued"); color: q.sub }
}
