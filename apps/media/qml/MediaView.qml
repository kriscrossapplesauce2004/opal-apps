import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Opal.Material
import Opal.Media
// The whole Media interface: navigation, the pages, the now-playing panel or docked mini player, and the full player and video over them.
Item {
    id: root
    readonly property var dests: [{id: "home", title: qsTr("Home"), icon: "home"}, {id: "music", title: qsTr("Music"), icon: "library_music"},
                                  {id: "videos", title: qsTr("Videos"), icon: "movie"}, {id: "radio", title: qsTr("Radio"), icon: "radio"},
                                  {id: "books", title: Posture.touch ? qsTr("Books") : qsTr("Audiobooks"), icon: "auto_stories"}]
    readonly property string page: dests[frame.currentIndex].id
    property var detail: null
    property bool searching: false
    property bool playerOpen: false
    property var video: null
    property bool settingsOpen: false
    readonly property bool panelShown: frame.navStyle === "railExpanded" && width >= 1180 && Player.hasMedia && !detail && !searching
    function go(id) { detail = null; searching = false; frame.currentIndex = Math.max(0, dests.findIndex(d => d.id === id)); }
    function openAlbum(key) { searching = false; detail = {kind: "album", key: key}; }
    function openArtist(name) { searching = false; detail = {kind: "artist", key: name}; }
    function openPlaylist(path, name) { searching = false; detail = {kind: "playlist", key: path, name: name}; }
    function openBook(key) { searching = false; detail = {kind: "book", key: key}; }
    function openSearch() { searching = true; detail = null; searchLoader.item && searchLoader.item.focusField(); }
    function openSettings() { settingsOpen = true; }
    function playVideo(v) { video = v; }
    function toast(m) { toastItem.show(m); }
    function openFiles(tracks) { Player.playTracks(tracks, 0); playerOpen = true; }
    // ---------- track menu ----------
    property var menuTrack: ({})
    property var menuContext: null
    property var pendingPaths: []
    function trackMenu(t, x, y, context) { menuTrack = t; menuContext = context || null; trackMenuItem.popup(root, x, y); }
    function playlistMenu(paths) { pendingPaths = paths; listMenu.popup(); }
    function newPlaylist(paths) { pendingPaths = paths; nameSheet.open = true; nameField.text = ""; nameField.forceActiveFocus(); }
    Menu {
        id: trackMenuItem
        MenuItem { text: qsTr("Play next"); onTriggered: { Player.playNext([root.menuTrack]); root.toast(qsTr("Plays next")); } }
        MenuItem { text: qsTr("Add to the queue"); onTriggered: { Player.enqueue([root.menuTrack]); root.toast(qsTr("Added to the queue")); } }
        MenuItem { text: qsTr("Add to a playlist…"); onTriggered: root.playlistMenu([root.menuTrack.path]) }
        MenuItem { text: MediaLibrary.likedRevision >= 0 && MediaLibrary.liked(root.menuTrack.path || "") ? qsTr("Remove from liked songs") : qsTr("Like"); onTriggered: MediaLibrary.toggleLiked(root.menuTrack.path) }
        MenuSeparator {}
        MenuItem { text: qsTr("Go to album"); enabled: !!root.menuTrack.albumKey; onTriggered: root.openAlbum(root.menuTrack.albumKey) }
        MenuItem { text: qsTr("Go to artist"); enabled: !!root.menuTrack.artist; onTriggered: root.openArtist(root.menuTrack.artist) }
        MenuItem { visible: !!root.menuContext; height: visible ? implicitHeight : 0; text: qsTr("Remove from this playlist")
            onTriggered: MediaLibrary.removeFromPlaylist(root.menuContext.playlist, root.menuContext.index) }
    }
    Menu {
        id: listMenu
        MenuItem { text: qsTr("New playlist…"); onTriggered: root.newPlaylist(root.pendingPaths) }
        MenuSeparator {}
        Repeater {
            model: MediaLibrary.likedRevision >= 0 ? MediaLibrary.playlists() : []
            MenuItem { required property var modelData; text: modelData.name
                onTriggered: { MediaLibrary.addToPlaylist(modelData.path, root.pendingPaths); root.toast(qsTr("Added to %1").arg(modelData.name)); } }
        }
    }
    Keys.onPressed: e => {
        if (e.key === Qt.Key_Space && !(activeFocusItem instanceof TextInput)) { Player.toggle(); e.accepted = true; }
        else if (e.key === Qt.Key_F && e.modifiers & Qt.ControlModifier) { openSearch(); e.accepted = true; }
        else if (e.key === Qt.Key_MediaNext) { Player.next(); e.accepted = true; }
        else if (e.key === Qt.Key_MediaPrevious) { Player.previous(); e.accepted = true; }
    }
    Connections { target: Player; function onErrorChanged() { if (Player.error) root.toast(Player.error); } }

    AppFrame {
        id: frame
        anchors.fill: parent
        focus: true
        visible: !root.video
        destinations: root.dests
        fabIcon: "search"; fabText: qsTr("Search")
        onFabClicked: root.openSearch()
        onCurrentIndexChanged: { root.detail = null; root.searching = false; }
        hints: [{b: "A", t: qsTr("Play")}, {b: "B", t: qsTr("Back")}, {b: "X", t: qsTr("Queue")}, {b: "Y", t: qsTr("Search")}]
        railFooter: ColumnLayout {
            width: parent ? parent.width : 200; spacing: 4
            MButton { Layout.alignment: frame.navStyle === "railExpanded" ? Qt.AlignLeft : Qt.AlignHCenter; Layout.leftMargin: frame.navStyle === "railExpanded" ? 8 : 0
                icon: "settings"; text: frame.navStyle === "railExpanded" ? qsTr("Settings") : ""; tooltip: qsTr("Settings"); onClicked: root.settingsOpen = true }
        }
        RowLayout {
            anchors.fill: parent; spacing: 0
            Item {
                Layout.fillWidth: true; Layout.fillHeight: true
                Loader { anchors.fill: parent; active: root.page === "home" && !root.detail && !root.searching; sourceComponent: HomePage { app: root } }
                Loader { anchors.fill: parent; active: root.page === "music"; visible: !root.detail && !root.searching; sourceComponent: MusicPage { app: root } }
                Loader { anchors.fill: parent; active: root.page === "videos" && !root.detail && !root.searching; sourceComponent: VideosPage { app: root } }
                Loader { anchors.fill: parent; active: root.page === "radio" && !root.detail && !root.searching; sourceComponent: RadioPage { app: root } }
                Loader { anchors.fill: parent; active: root.page === "books" && !root.detail && !root.searching; sourceComponent: BooksPage { app: root } }
                Loader {
                    anchors.fill: parent; active: !!root.detail && !root.searching
                    sourceComponent: CollectionPage { app: root; kind: root.detail.kind; key: root.detail.key; name: root.detail.name || ""; onBack: root.detail = null; focus: true }
                }
                Loader { id: searchLoader; anchors.fill: parent; active: root.searching
                    sourceComponent: SearchPage { app: root; onBack: root.searching = false; Component.onCompleted: focusField() } }
                // Docked mini player when there is no side panel.
                MiniPlayer {
                    visible: Player.hasMedia && !root.panelShown
                    anchors { left: parent.left; right: parent.right; bottom: parent.bottom; leftMargin: 8; rightMargin: 8; bottomMargin: 6 }
                    onExpand: root.playerOpen = true
                }
            }
            NowPlayingPanel {
                visible: root.panelShown
                Layout.preferredWidth: 380; Layout.fillHeight: true; Layout.margins: 12
                onExpand: root.playerOpen = true
                onOpenAlbum: k => root.openAlbum(k)
            }
        }
    }
    Loader {
        anchors.fill: parent
        active: root.playerOpen && Player.hasMedia && !root.video
        sourceComponent: FullPlayer { onClose: root.playerOpen = false; onOpenAlbum: k => root.openAlbum(k) }
        // The player rises from the bottom.
        y: active ? 0 : height
    }
    Loader {
        anchors.fill: parent
        active: !!root.video
        sourceComponent: VideoPlayer {
            video: root.video
            onClose: { root.video = null; frame.forceActiveFocus(); if (Window.window) Window.window.visibility = Window.AutomaticVisibility; }
            onFullScreenRequested: on => { if (Window.window) Window.window.visibility = Window.window.visibility === Window.FullScreen ? Window.AutomaticVisibility : Window.FullScreen; }
        }
    }
    Sheet {
        open: root.settingsOpen
        sheetHeight: Math.min(root.height * .8, 640)
        onDismissed: root.settingsOpen = false
        MediaSettings { anchors.fill: parent }
    }
    Sheet {
        id: nameSheet
        sheetHeight: 240
        ColumnLayout {
            anchors.fill: parent; anchors.margins: 24; spacing: 12
            MText { text: qsTr("New playlist"); role: "title" }
            TextField { id: nameField; Layout.fillWidth: true; placeholderText: qsTr("Name"); color: Theme.text; font.family: Theme.font; onAccepted: create.clicked() }
            Item { Layout.fillHeight: true }
            RowLayout { Layout.alignment: Qt.AlignRight; spacing: 8
                MButton { text: qsTr("Cancel"); onClicked: nameSheet.open = false }
                MButton { id: create; text: qsTr("Create"); filled: true; enabled: nameField.text.trim() !== ""
                    onClicked: { const p = MediaLibrary.createPlaylist(nameField.text, root.pendingPaths); nameSheet.open = false; root.toast(p ? qsTr("Made %1").arg(nameField.text) : qsTr("A playlist with that name already exists")); } } }
        }
    }
    Toast { id: toastItem }
}
