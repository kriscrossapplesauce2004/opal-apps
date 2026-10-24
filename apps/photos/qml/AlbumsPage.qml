import QtQuick
import QtQuick.Controls
import QtQuick.Effects
import QtQuick.Layouts
import Opal.Material
import Opal.Photos
// Albums: every folder with photos as a mosaic card, and a chip per source (this PC, a phone, a camera card).
Item {
    id: page
    signal openAlbum(var album)
    signal addFolder()
    readonly property bool phone: width < 600
    readonly property bool couch: Posture.couch
    property string source: ""
    property int current: -1
    AlbumModel { id: albums; source: page.source }
    readonly property int columns: couch ? 4 : Math.max(2, Math.floor((grid.width + 20) / (phone ? 170 : 230)))
    Keys.onPressed: e => {
        if (e.key === Qt.Key_Right) { current = Math.min(albums.count - 1, current + 1); grid.positionViewAtIndex(current, GridView.Contain); e.accepted = true; }
        else if (e.key === Qt.Key_Left) { current = Math.max(0, current - 1); grid.positionViewAtIndex(current, GridView.Contain); e.accepted = true; }
        else if (e.key === Qt.Key_Down) { current = Math.min(albums.count - 1, current < 0 ? 0 : current + columns); grid.positionViewAtIndex(current, GridView.Contain); e.accepted = true; }
        else if (e.key === Qt.Key_Up) { current = Math.max(0, current - columns); grid.positionViewAtIndex(current, GridView.Contain); e.accepted = true; }
        else if ((e.key === Qt.Key_Return || e.key === Qt.Key_Enter) && current >= 0) { page.openAlbum(albums.get(current)); e.accepted = true; }
    }
    ColumnLayout {
        anchors.fill: parent; anchors.topMargin: phone ? 8 : 14; spacing: phone ? 10 : 16
        RowLayout {
            Layout.fillWidth: true; Layout.leftMargin: page.phone ? 16 : 4; Layout.rightMargin: page.phone ? 8 : 16; spacing: 10
            ColumnLayout { spacing: 0
                MText { text: qsTr("Albums"); role: page.couch ? "displayS" : "headline" }
                MText { text: qsTr("%n album(s)", "", albums.count); role: "labelS"; color: Theme.subtext } }
            Item { Layout.fillWidth: true }
            MButton { text: page.phone ? "" : qsTr("Add folder"); icon: "create_new_folder"; tonal: true; tooltip: qsTr("Add a folder to Photos"); onClicked: page.addFolder() }
        }
        // Sources: this PC first, then phones and cards that are plugged in.
        Flickable {
            visible: PhotoLibrary.sources.length > 1
            Layout.fillWidth: true; Layout.preferredHeight: 40; Layout.leftMargin: page.phone ? 12 : 4
            contentWidth: srcRow.implicitWidth; clip: true; boundsBehavior: Flickable.StopAtBounds
            Row { id: srcRow; spacing: 8
                Repeater { model: PhotoLibrary.sources
                    Chip { required property var modelData; text: modelData.name + " · " + modelData.count; icon: modelData.icon; selected: page.source === modelData.id; glass: false
                        onClicked: page.source = modelData.id } } }
        }
        GridView {
            id: grid
            Layout.fillWidth: true; Layout.fillHeight: true; Layout.leftMargin: page.phone ? 8 : 0; Layout.rightMargin: page.phone ? 8 : 16
            clip: true; boundsBehavior: Flickable.StopAtBounds
            model: albums
            cellWidth: Math.floor(width / page.columns); cellHeight: cellWidth + (page.couch ? 76 : 62)
            ScrollBar.vertical: ScrollBar {}
            delegate: Item {
                id: card
                required property int index
                required property string kind
                required property string path
                required property string name
                required property string parentName
                required property int count
                required property var covers
                width: grid.cellWidth; height: grid.cellHeight
                readonly property bool focused: page.current === index && (page.activeFocus || Posture.controllerActive)
                ColumnLayout {
                    anchors.fill: parent; anchors.margins: page.phone ? 6 : 10; spacing: 8
                    Item {
                        Layout.fillWidth: true; Layout.preferredHeight: width
                        Rectangle { anchors.fill: parent; anchors.margins: -5; radius: mosaic.radius + 5; color: "transparent"; border.width: 3; border.color: Theme.primary; visible: card.focused }
                        // Four covers in a 2 × 2 mosaic; fewer photos use fewer, bigger tiles.
                        Rectangle {
                            id: mosaic
                            anchors.fill: parent; radius: page.couch ? 32 : 26; color: Theme.surfaceHigh
                            scale: tap.pressed ? .97 : 1
                            Behavior on scale { NumberAnimation { duration: Theme.motion(Theme.durShort) } }
                            readonly property int n: Math.min(4, card.covers.length)
                            // Square tiles drawn into a layer, then masked to the rounded card.
                            Item {
                                id: tiles
                                anchors.fill: parent; visible: false; layer.enabled: true
                                Repeater {
                                    model: mosaic.n
                                    Image {
                                        required property int index
                                        readonly property real half: (mosaic.width - 3) / 2
                                        x: mosaic.n === 1 ? 0 : index % 2 ? half + 3 : 0
                                        y: mosaic.n <= 2 ? 0 : index >= 2 ? half + 3 : 0
                                        width: mosaic.n === 1 ? mosaic.width : mosaic.n === 3 && index === 2 ? mosaic.width : half
                                        height: mosaic.n <= 2 ? mosaic.height : half
                                        fillMode: Image.PreserveAspectCrop; asynchronous: true
                                        source: card.covers[index]
                                        sourceSize: Qt.size(320, 320)
                                    }
                                }
                            }
                            Rectangle { id: coverMask; anchors.fill: parent; radius: mosaic.radius; visible: false; layer.enabled: true; layer.smooth: true }
                            MultiEffect { anchors.fill: parent; source: tiles; maskEnabled: true; maskSource: coverMask; maskThresholdMin: .5; maskSpreadAtMin: 1 }
                            Rectangle {
                                visible: card.kind === "favourites"
                                anchors { left: parent.left; bottom: parent.bottom; margins: 12 }
                                width: 40; height: 40; radius: 20; color: Theme.primary
                                Glyph { anchors.centerIn: parent; name: "favorite"; fill: 1; color: Theme.onPrimary }
                            }
                        }
                    }
                    MText { Layout.fillWidth: true; text: card.name; role: page.couch ? "title" : "titleS"; elide: Text.ElideRight }
                    MText { Layout.fillWidth: true; text: qsTr("%n item(s)", "", card.count) + (card.parentName ? " · " + card.parentName : ""); role: "labelS"; color: Theme.subtext; elide: Text.ElideRight }
                }
                HoverHandler { cursorShape: Qt.PointingHandCursor }
                TapHandler { id: tap; onTapped: page.openAlbum(albums.get(card.index)) }
                Accessible.role: Accessible.Button; Accessible.name: card.name + ", " + qsTr("%n item(s)", "", card.count)
            }
            MText {
                anchors.centerIn: parent; visible: albums.count === 0 && PhotoLibrary.loaded
                text: page.source ? qsTr("No photos on this device") : qsTr("No albums yet"); role: "title"; color: Theme.subtext
            }
        }
    }
}
