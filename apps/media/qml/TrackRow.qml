import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Opal.Material
import Opal.Media
// One song in a list: number or cover, title, artist, like, length and a menu.
Rectangle {
    id: row
    property var track: ({})
    property int number: 0
    property bool showCover: false
    property bool showAlbum: false
    property bool current: Player.hasMedia && Player.current.path === track.path
    property bool focused: false
    signal activated()
    signal menuRequested(var track, real x, real y)
    implicitHeight: Posture.couch ? 68 : 56
    radius: Theme.shapeLg
    color: current ? Theme.secondaryContainer : hover.hovered ? Theme.alpha(Theme.text, .05) : "transparent"
    border.width: focused ? 3 : 0; border.color: Theme.primary
    readonly property bool liked: MediaLibrary.likedRevision >= 0 && MediaLibrary.liked(track.path || "")
    RowLayout {
        anchors.fill: parent; anchors.leftMargin: 12; anchors.rightMargin: 6; spacing: 14
        Item {
            Layout.preferredWidth: row.showCover ? 40 : 28; Layout.preferredHeight: 40
            Cover { visible: row.showCover; anchors.fill: parent; r: 10; source: row.track.cover || ""; title: row.track.album || "" }
            MText { visible: !row.showCover && !row.current; anchors.centerIn: parent; text: row.number > 0 ? row.number : ""; role: "labelS"; color: Theme.subtext }
            Glyph { visible: row.current && !row.showCover; anchors.centerIn: parent; name: Player.playing ? "graphic_eq" : "pause"; color: Theme.primary; font.pixelSize: 20 }
        }
        ColumnLayout {
            Layout.fillWidth: true; spacing: 0
            MText { Layout.fillWidth: true; text: row.track.title || ""; role: "titleS"; elide: Text.ElideRight; color: row.current ? Theme.onSecondaryContainer : Theme.text }
            MText { Layout.fillWidth: true; text: (row.track.artist || "") + (row.showAlbum && row.track.album ? " · " + row.track.album : ""); role: "labelS"; elide: Text.ElideRight
                color: row.current ? Theme.alpha(Theme.onSecondaryContainer, .75) : Theme.subtext }
        }
        MButton { visible: row.liked || hover.hovered; icon: "favorite"; compact: true; checked: row.liked; tooltip: row.liked ? qsTr("Remove from liked songs") : qsTr("Like")
            ink: row.liked ? Theme.primary : Theme.subtext; onClicked: MediaLibrary.toggleLiked(row.track.path) }
        MText { text: row.track.length || ""; role: "labelS"; color: Theme.subtext }
        MButton { icon: "more_vert"; compact: true; tooltip: qsTr("More"); onClicked: { const p = mapToItem(null, width / 2, height); row.menuRequested(row.track, p.x, p.y); } }
    }
    HoverHandler { id: hover }
    TapHandler { onTapped: row.activated(); onLongPressed: { const p = row.mapToItem(null, row.width / 2, row.height); row.menuRequested(row.track, p.x, p.y); } }
    TapHandler { acceptedButtons: Qt.RightButton; onTapped: (pt) => { const p = row.mapToItem(null, pt.position.x, pt.position.y); row.menuRequested(row.track, p.x, p.y); } }
    Accessible.role: Accessible.ListItem; Accessible.name: (track.title || "") + ", " + (track.artist || "")
}
