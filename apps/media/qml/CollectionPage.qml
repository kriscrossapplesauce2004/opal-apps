import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Opal.Material
import Opal.Media
// One album, artist, playlist or audiobook: a big header with the art and Play, then the tracks or chapters.
Item {
    id: page
    property var app
    property string kind: "album"
    property string key: ""
    property string name: ""
    signal back()
    readonly property bool phone: width < 640
    readonly property int rev: MediaLibrary.revision + MediaLibrary.likedRevision + MediaLibrary.progressRevision
    readonly property var info: rev >= 0 ? (kind === "album" ? MediaLibrary.album(key) : kind === "artist" ? MediaLibrary.artist(key) : kind === "book" ? MediaLibrary.book(key)
                                            : {title: name, tracks: MediaLibrary.playlist(key)}) : ({})
    readonly property var tracks: info.tracks || []
    readonly property string title: kind === "artist" ? key : info.title || name
    readonly property string cover: kind === "artist" ? ((info.albums || [])[0] || {}).cover || "" : info.cover || (tracks[0] || {}).cover || ""
    readonly property string meta: {
        let total = 0; for (const t of tracks) total += t.duration || 0;
        const parts = [];
        if (kind === "album") { parts.push(info.artist || ""); if (info.year) parts.push(info.year); }
        if (kind === "book") { parts.push(info.author || ""); parts.push(qsTr("%n chapter(s)", "", tracks.length)); }
        else parts.push(qsTr("%n song(s)", "", tracks.length));
        parts.push(MediaLibrary.formatDuration(total));
        return parts.filter(p => p !== "" && p !== 0).join(" · ");
    }
    function playFrom(i) { if (kind === "book") Player.playBook(key, i); else Player.playTracks(tracks, i); }
    Keys.onEscapePressed: back()
    ListView {
        id: list
        anchors.fill: parent; anchors.leftMargin: page.phone ? 8 : 8; anchors.rightMargin: page.phone ? 8 : 16
        clip: true; spacing: 2; boundsBehavior: Flickable.StopAtBounds; bottomMargin: 100
        model: page.tracks
        ScrollBar.vertical: ScrollBar {}
        header: ColumnLayout {
            width: list.width; spacing: 14
            RowLayout { Layout.topMargin: page.phone ? 8 : 16
                MButton { icon: "arrow_back"; compact: true; tooltip: qsTr("Back"); onClicked: page.back() } }
            GridLayout {
                Layout.fillWidth: true; columns: page.phone ? 1 : 2; columnSpacing: 28; rowSpacing: 16
                Cover {
                    Layout.preferredWidth: page.phone ? Math.min(list.width * .7, 280) : Math.min(260, list.width * .3); Layout.preferredHeight: Layout.preferredWidth
                    Layout.alignment: page.phone ? Qt.AlignHCenter : Qt.AlignTop
                    r: page.kind === "artist" ? width / 2 : 28; source: page.cover; title: page.title
                    icon: page.kind === "artist" && !page.cover ? "person" : page.kind === "playlist" && !page.cover ? (page.key === "liked:" ? "favorite" : "queue_music") : ""
                }
                ColumnLayout {
                    Layout.fillWidth: true; Layout.alignment: Qt.AlignBottom; spacing: 6
                    MText { text: ({album: qsTr("Album"), artist: qsTr("Artist"), playlist: qsTr("Playlist"), book: qsTr("Audiobook")})[page.kind]; role: "label"; color: Theme.primary }
                    MText { Layout.fillWidth: true; text: page.title; font.pixelSize: page.phone ? 30 : 44; font.weight: Font.Black; font.variableAxes: ({"wdth": 110, "ROND": 50}); wrapMode: Text.Wrap; maximumLineCount: 3; elide: Text.ElideRight }
                    MText { Layout.fillWidth: true; text: page.meta; role: "body"; color: Theme.subtext; wrapMode: Text.Wrap
                        TapHandler { enabled: page.kind === "album" && !!page.info.artist; onTapped: page.app.openArtist(page.info.artist) } }
                    // Audiobook progress.
                    ColumnLayout { visible: page.kind === "book" && !!page.info.started; Layout.fillWidth: true; spacing: 4; Layout.topMargin: 4
                        WavyProgress { Layout.fillWidth: true; Layout.maximumWidth: 360; value: page.info.progress || 0; animate: false; amplitude: 2; wavelength: 16; thickness: 4 }
                        MText { text: page.info.finished ? qsTr("Finished") : qsTr("%1 left").arg(page.info.left || ""); role: "labelS"; color: Theme.subtext } }
                    RowLayout {
                        Layout.topMargin: 10; spacing: 8
                        PlayButton { size: 56; playing: Player.playing && (page.kind === "book" ? Player.bookKey === page.key : page.tracks.some(t => t.path === Player.current.path))
                            onClicked: { if (playing) Player.pause(); else if (page.kind === "book") Player.playBook(page.key); else { Player.shuffle = false; Player.playTracks(page.tracks, 0); } } }
                        MText { visible: page.kind === "book"; text: page.info.started ? qsTr("Resume") : qsTr("Start listening"); role: "titleS" }
                        MButton { visible: page.kind !== "book"; icon: "shuffle"; tonal: true; tooltip: qsTr("Shuffle"); enabled: page.tracks.length > 0
                            onClicked: { Player.shuffle = true; Player.playTracks(page.tracks, Math.floor(Math.random() * page.tracks.length)); } }
                        MButton { visible: page.kind !== "book"; icon: "queue_music"; tonal: true; tooltip: qsTr("Add to the queue"); enabled: page.tracks.length > 0; onClicked: { Player.enqueue(page.tracks); page.app.toast(qsTr("Added to the queue")); } }
                        MButton { visible: page.kind !== "book"; icon: "playlist_add"; tonal: true; tooltip: qsTr("Add to a playlist"); enabled: page.tracks.length > 0; onClicked: page.app.playlistMenu(page.tracks.map(t => t.path)) }
                        MButton { visible: page.kind === "playlist" && page.key !== "liked:"; icon: "delete"; tonal: true; tooltip: qsTr("Delete playlist")
                            onClicked: { MediaLibrary.deletePlaylist(page.key); page.back(); } }
                    }
                }
            }
            // An artist's albums above the songs.
            ListView {
                visible: page.kind === "artist" && (page.info.albums || []).length > 0
                Layout.fillWidth: true; Layout.preferredHeight: 210; orientation: ListView.Horizontal; spacing: 14; clip: true
                model: page.kind === "artist" ? page.info.albums || [] : []
                delegate: ColumnLayout {
                    required property var modelData
                    width: 150; spacing: 4
                    Cover { Layout.fillWidth: true; Layout.preferredHeight: width; source: modelData.cover || ""; title: modelData.title || ""
                        TapHandler { onTapped: page.app.openAlbum(parent.parent.modelData.key) } HoverHandler { cursorShape: Qt.PointingHandCursor } }
                    MText { Layout.fillWidth: true; text: modelData.title || ""; role: "titleS"; elide: Text.ElideRight }
                    MText { Layout.fillWidth: true; text: modelData.year || ""; role: "labelS"; color: Theme.subtext }
                }
            }
            MText { visible: page.kind === "artist"; text: qsTr("Songs"); role: "title" }
            Item { implicitHeight: 4 }
        }
        delegate: TrackRow {
            required property var modelData
            required property int index
            width: list.width; track: modelData
            number: page.kind === "album" ? (modelData.number || index + 1) : index + 1
            showCover: page.kind !== "album" && page.kind !== "book"
            showAlbum: page.kind === "playlist"
            current: page.kind === "book" ? Player.bookKey === page.key && Player.index === index : Player.hasMedia && Player.current.path === modelData.path
            onActivated: page.playFrom(index)
            onMenuRequested: (t, x, y) => page.app.trackMenu(t, x, y, page.kind === "playlist" ? {playlist: page.key, index: index} : null)
        }
        footer: MText { visible: page.tracks.length === 0; width: list.width; horizontalAlignment: Text.AlignHCenter; topPadding: 40
            text: page.kind === "playlist" ? qsTr("This playlist is empty. Add songs from their menu.") : qsTr("Nothing here"); color: Theme.subtext }
    }
}
