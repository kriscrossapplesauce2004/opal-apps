import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Opal.Material
import Opal.Media
// Music: albums, artists, songs and playlists, with genre chips and a sort.
Item {
    id: page
    property var app
    property string view: "albums"
    property string genre: ""
    property string sort: "added"
    property string query: ""
    readonly property bool phone: width < 600
    readonly property int rev: MediaLibrary.revision + MediaLibrary.likedRevision
    readonly property var albumList: rev >= 0 && view === "albums" ? MediaLibrary.albums(query, genre, sort) : []
    readonly property var artistList: rev >= 0 && view === "artists" ? MediaLibrary.artists(query) : []
    readonly property var songList: rev >= 0 && view === "songs" ? MediaLibrary.songs(query, sort === "added" ? "added" : sort === "artist" ? "artist" : "title") : []
    readonly property var playlistList: rev >= 0 && view === "lists" ? MediaLibrary.playlists() : []
    ColumnLayout {
        anchors.fill: parent; anchors.topMargin: page.phone ? 8 : 18; spacing: 12
        RowLayout {
            Layout.fillWidth: true; Layout.leftMargin: page.phone ? 16 : 8; Layout.rightMargin: 16; spacing: 10
            ColumnLayout { spacing: 0
                MText { text: qsTr("Music"); role: page.phone ? "headline" : "displayS" }
                MText { text: qsTr("%n song(s)", "", MediaLibrary.songCount) + " · " + qsTr("%n album(s)", "", MediaLibrary.albumCount); role: "labelS"; color: Theme.subtext } }
            Item { Layout.fillWidth: true }
            SearchField { visible: !page.phone; Layout.preferredWidth: Math.min(300, page.width * .28); placeholder: qsTr("Filter"); onTextChanged: page.query = text }
            MButton { visible: page.view !== "artists" && page.view !== "lists"; icon: "sort"; compact: true; tooltip: qsTr("Sort"); onClicked: sortMenu.popup() }
        }
        Menu {
            id: sortMenu
            Repeater { model: [{id: "added", t: qsTr("Recently added")}, {id: "title", t: qsTr("Title")}, {id: "artist", t: qsTr("Artist")}, {id: "year", t: qsTr("Year")}]
                MenuItem { required property var modelData; text: modelData.t; checkable: true; checked: page.sort === modelData.id; onTriggered: page.sort = modelData.id } }
        }
        SegmentedButtons {
            Layout.leftMargin: page.phone ? 16 : 8
            current: page.view
            options: [{id: "albums", name: qsTr("Albums")}, {id: "artists", name: qsTr("Artists")}, {id: "songs", name: qsTr("Songs")}, {id: "lists", name: qsTr("Playlists")}]
            onPicked: id => page.view = id
        }
        Flickable {
            visible: page.view === "albums" && MediaLibrary.genres.length > 0
            Layout.fillWidth: true; Layout.preferredHeight: 36; contentWidth: genreRow.implicitWidth + 24; clip: true; boundsBehavior: Flickable.StopAtBounds
            Row { id: genreRow; x: page.phone ? 16 : 8; spacing: 8
                Chip { text: qsTr("All"); selected: page.genre === ""; glass: false; onClicked: page.genre = "" }
                Repeater { model: MediaLibrary.genres.slice(0, 12)
                    Chip { required property string modelData; text: modelData; selected: page.genre === modelData; glass: false; onClicked: page.genre = page.genre === modelData ? "" : modelData } } }
        }
        // ---------- albums ----------
        GridView {
            id: albumGrid
            visible: page.view === "albums"
            Layout.fillWidth: true; Layout.fillHeight: true; Layout.leftMargin: page.phone ? 8 : 0; Layout.rightMargin: page.phone ? 8 : 8
            clip: true; boundsBehavior: Flickable.StopAtBounds
            readonly property int cols: Math.max(2, Math.floor(width / (page.phone ? 170 : 210)))
            cellWidth: Math.floor(width / cols); cellHeight: cellWidth + 58
            model: page.albumList
            bottomMargin: 100
            ScrollBar.vertical: ScrollBar {}
            delegate: Item {
                required property var modelData
                width: albumGrid.cellWidth; height: albumGrid.cellHeight
                ColumnLayout {
                    anchors.fill: parent; anchors.margins: page.phone ? 6 : 9; spacing: 6
                    Cover {
                        id: cov
                        Layout.fillWidth: true; Layout.preferredHeight: width
                        source: modelData.cover || ""; title: modelData.title || ""; playing: Player.current.albumKey === modelData.key
                        scale: albumTap.pressed ? .97 : 1
                        Behavior on scale { NumberAnimation { duration: Theme.motion(Theme.durShort) } }
                        HoverHandler { id: albumHover; cursorShape: Qt.PointingHandCursor }
                        TapHandler { id: albumTap; onTapped: page.app.openAlbum(cov.parent.parent.modelData.key) }
                        PlayButton { visible: albumHover.hovered && !cov.playing; anchors.left: parent.left; anchors.bottom: parent.bottom; anchors.margins: 10; size: 46; playing: false
                            onClicked: Player.playTracks(MediaLibrary.album(cov.parent.parent.modelData.key).tracks, 0) }
                    }
                    MText { Layout.fillWidth: true; text: modelData.title || ""; role: "titleS"; elide: Text.ElideRight; color: Player.current.albumKey === modelData.key ? Theme.primary : Theme.text }
                    MText { Layout.fillWidth: true; text: (modelData.artist || "") + (modelData.year ? " · " + modelData.year : ""); role: "labelS"; color: Theme.subtext; elide: Text.ElideRight }
                }
                Accessible.role: Accessible.Button; Accessible.name: (modelData.title || "") + ", " + (modelData.artist || "")
            }
        }
        // ---------- artists ----------
        GridView {
            id: artistGrid
            visible: page.view === "artists"
            Layout.fillWidth: true; Layout.fillHeight: true; Layout.leftMargin: page.phone ? 8 : 0; Layout.rightMargin: 8
            clip: true; boundsBehavior: Flickable.StopAtBounds; bottomMargin: 100
            readonly property int cols: Math.max(2, Math.floor(width / (page.phone ? 150 : 180)))
            cellWidth: Math.floor(width / cols); cellHeight: cellWidth + 46
            model: page.artistList
            ScrollBar.vertical: ScrollBar {}
            delegate: Item {
                required property var modelData
                width: artistGrid.cellWidth; height: artistGrid.cellHeight
                ColumnLayout { anchors.fill: parent; anchors.margins: 12; spacing: 6
                    Cover { Layout.fillWidth: true; Layout.preferredHeight: width; r: width / 2; source: modelData.cover || ""; title: modelData.name || ""; icon: modelData.cover ? "" : "person" }
                    MText { Layout.fillWidth: true; horizontalAlignment: Text.AlignHCenter; text: modelData.name || ""; role: "titleS"; elide: Text.ElideRight }
                    MText { Layout.fillWidth: true; horizontalAlignment: Text.AlignHCenter; text: qsTr("%n song(s)", "", modelData.songs); role: "labelS"; color: Theme.subtext } }
                HoverHandler { cursorShape: Qt.PointingHandCursor }
                TapHandler { onTapped: page.app.openArtist(parent.modelData.name) }
                Accessible.role: Accessible.Button; Accessible.name: modelData.name || ""
            }
        }
        // ---------- songs ----------
        ListView {
            id: songListView
            visible: page.view === "songs"
            Layout.fillWidth: true; Layout.fillHeight: true; Layout.leftMargin: page.phone ? 8 : 0; Layout.rightMargin: 12
            clip: true; spacing: 2; boundsBehavior: Flickable.StopAtBounds; bottomMargin: 100
            model: page.songList
            ScrollBar.vertical: ScrollBar {}
            header: RowLayout { width: songListView.width; height: 56; spacing: 8
                MButton { text: qsTr("Play all"); icon: "play_arrow"; filled: true; enabled: page.songList.length > 0; onClicked: { Player.shuffle = false; Player.playTracks(page.songList, 0); } }
                MButton { text: qsTr("Shuffle"); icon: "shuffle"; tonal: true; enabled: page.songList.length > 0; onClicked: { Player.shuffle = true; Player.playTracks(page.songList, Math.floor(Math.random() * page.songList.length)); } } }
            delegate: TrackRow {
                required property var modelData
                required property int index
                width: songListView.width; track: modelData; showCover: true; showAlbum: true
                onActivated: Player.playTracks(page.songList, index)
                onMenuRequested: (t, x, y) => page.app.trackMenu(t, x, y)
            }
        }
        // ---------- playlists ----------
        ListView {
            id: listsView
            visible: page.view === "lists"
            Layout.fillWidth: true; Layout.fillHeight: true; Layout.leftMargin: page.phone ? 8 : 0; Layout.rightMargin: 12
            clip: true; spacing: 4; boundsBehavior: Flickable.StopAtBounds; bottomMargin: 100
            model: page.playlistList
            header: RowLayout { width: listsView.width; height: 56
                MButton { text: qsTr("New playlist"); icon: "playlist_add"; tonal: true; onClicked: page.app.newPlaylist([]) } }
            delegate: ListRow {
                required property var modelData
                width: listsView.width
                icon: modelData.path === "liked:" ? "favorite" : "queue_music"; iconBadge: true
                title: modelData.name; subtitle: qsTr("%n song(s)", "", modelData.count)
                onClicked: page.app.openPlaylist(modelData.path, modelData.name)
            }
        }
    }
    MText { anchors.centerIn: parent; visible: MediaLibrary.loaded && MediaLibrary.songCount === 0; text: qsTr("No music found yet"); role: "title"; color: Theme.subtext }
}
