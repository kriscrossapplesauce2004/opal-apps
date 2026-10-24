import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts
import Opal.Material
import Opal.Photos
// The whole Photos interface: navigation, the timeline or albums, and the viewer, editor and slideshow over them.
Item {
    id: root
    property string openPath: ""
    property var album: null
    property var viewerPaths: []
    property int viewerIndex: 0
    property bool viewerOpen: false
    property string editPath: ""
    property var showPaths: []
    property int showIndex: 0
    property bool showOpen: false
    property bool settingsOpen: false
    property bool viewerDetails: false
    function timelineView() { return timeline.view; }
    function go(i) { frame.currentIndex = i; }
    readonly property var dests: {
        const d = [{title: qsTr("Photos"), icon: "photo_library", filter: "all"}, {title: qsTr("Albums"), icon: "photo_album", filter: "albums"},
                   {title: qsTr("Favourites"), icon: "favorite", filter: "favourites"}];
        if (PhotoLibrary.screenshotCount > 0) d.push({title: qsTr("Screenshots"), icon: "screenshot_monitor", filter: "screenshots"});
        if (PhotoLibrary.videoCount > 0) d.push({title: qsTr("Videos"), icon: "movie", filter: "videos"});
        return d;
    }
    readonly property var dest: dests[Math.min(frame.currentIndex, dests.length - 1)]
    readonly property string title: viewerOpen ? qsTr("Photos") : album ? album.name : dest.title
    function openViewer(paths, index) { viewerPaths = paths; viewerIndex = index; viewerOpen = true; }
    function startShow(paths, index) { showPaths = paths.filter(p => !(PhotoLibrary.photo(p).video)); showIndex = Math.max(0, showPaths.indexOf(paths[index])); showOpen = showPaths.length > 0; }
    Component.onCompleted: if (openPath) openOne.start()
    // A photo from a file manager: its folder's photos, in the library's order when it knows them.
    Timer {
        id: openOne
        interval: 50; repeat: true
        onTriggered: {
            if (!PhotoLibrary.loaded && PhotoLibrary.scanning) return;
            stop();
            const folder = root.openPath.substring(0, root.openPath.lastIndexOf("/"));
            siblings.album = folder;
            siblingWait.start();
        }
    }
    PhotoView { id: siblings; filter: "album"; group: "none"; width: 1000 }
    Timer {
        id: siblingWait; interval: 120
        onTriggered: { const list = siblings.paths(); const i = list.indexOf(root.openPath); root.openViewer(i >= 0 ? list : [root.openPath], Math.max(0, i)); }
    }
    FolderDialog {
        id: folderPick
        title: qsTr("Add a folder to Photos")
        onAccepted: { PhotoLibrary.addRoot(selectedFolder.toString()); toast.show(qsTr("Looking for photos in %1").arg(decodeURIComponent(selectedFolder.toString()).split("/").pop())); }
    }
    Connections {
        target: PhotoActions
        function onDone(m) { toast.show(m); }
        function onFailed(m) { toast.show(m); }
    }
    Connections { target: PhotoLibrary; function onCaptionFailed(path) { toast.show(qsTr("This file can't hold a caption")); } }

    AppFrame {
        id: frame
        anchors.fill: parent
        visible: !root.viewerOpen && root.editPath === "" && !root.showOpen
        destinations: root.dests
        fabIcon: "create_new_folder"; fabText: qsTr("Add folder")
        onFabClicked: folderPick.open()
        hints: [{b: "A", t: qsTr("View")}, {b: "B", t: qsTr("Back")}, {b: "X", t: qsTr("Favourite")}, {b: "Y", t: qsTr("Slideshow")}]
        onCurrentIndexChanged: { root.album = null; (root.dest.filter === "albums" ? albums : timeline).forceActiveFocus(); }
        Keys.onEscapePressed: e => { if (root.album) { root.album = null; e.accepted = true; } else e.accepted = false; }
        // Library status and settings under the rail.
        railFooter: ColumnLayout {
            width: parent ? parent.width : 200; spacing: 4
            MText { visible: frame.navStyle === "railExpanded"; Layout.leftMargin: 18; text: qsTr("%n item(s)", "", PhotoLibrary.count); role: "labelS"; color: Theme.subtext }
            MButton { Layout.alignment: frame.navStyle === "railExpanded" ? Qt.AlignLeft : Qt.AlignHCenter; Layout.leftMargin: frame.navStyle === "railExpanded" ? 8 : 0
                icon: "settings"; text: frame.navStyle === "railExpanded" ? qsTr("Settings") : ""; tooltip: qsTr("Settings"); onClicked: root.settingsOpen = true }
        }
        Timeline {
            id: timeline
            anchors.fill: parent
            visible: root.dest.filter !== "albums" || root.album !== null
            focus: visible
            filter: root.album ? "album" : root.dest.filter === "albums" ? "all" : root.dest.filter
            album: root.album ? root.album.path : ""
            heading: root.album ? root.album.name : root.dest.title
            showBack: root.album !== null
            emptyIcon: root.dest.icon
            emptyTitle: root.dest.filter === "favourites" ? qsTr("No favourites yet") : root.dest.filter === "screenshots" ? qsTr("No screenshots") : root.dest.filter === "videos" ? qsTr("No videos") : qsTr("No photos yet")
            emptyText: root.dest.filter === "favourites" ? qsTr("Tap the heart on a photo to keep it here. Favourites are shared with Starred in Files.")
                : root.dest.filter === "all" ? qsTr("Photos looks in your Pictures folder and every folder you add.") : ""
            onBack: { root.album = null; albums.forceActiveFocus(); }
            onOpenPhoto: i => root.openViewer(view.paths(), i)
            onSlideshow: root.startShow(view.paths(), Math.max(0, current))
            onAddFolder: folderPick.open()
        }
        AlbumsPage {
            id: albums
            anchors.fill: parent
            visible: root.dest.filter === "albums" && root.album === null
            focus: visible
            onOpenAlbum: a => { root.album = a.kind === "favourites" ? null : a; if (a.kind === "favourites") frame.currentIndex = 2; }
            onAddFolder: folderPick.open()
        }
        SelectionBar { view: timeline.view; visible: timeline.visible && count > 0; onMessage: t => toast.show(t) }
    }

    Loader {
        anchors.fill: parent
        active: root.viewerOpen && root.editPath === "" && !root.showOpen
        sourceComponent: Viewer {
            paths: root.viewerPaths; index: root.viewerIndex; detailsOpen: root.viewerDetails
            onDetailsOpenChanged: root.viewerDetails = detailsOpen
            onIndexChanged: root.viewerIndex = index
            onClosed: { root.viewerOpen = false; timeline.current = timeline.view.indexOfPath(root.viewerPaths[root.viewerIndex] || ""); timeline.reveal(timeline.current); timeline.forceActiveFocus(); }
            onEdit: p => root.editPath = p
            onSlideshow: i => root.startShow(root.viewerPaths, i)
            onMessage: t => toast.show(t)
        }
    }
    Loader {
        anchors.fill: parent
        active: root.editPath !== ""
        sourceComponent: Editor {
            path: root.editPath
            onClosed: root.editPath = ""
            onSaved: (p, asCopy) => {
                toast.show(asCopy ? qsTr("Saved a copy") : qsTr("Saved. The original is in the trash"));
                if (asCopy && root.viewerOpen) { const l = root.viewerPaths.slice(); l.splice(root.viewerIndex + 1, 0, p); root.viewerPaths = l; root.viewerIndex++; }
            }
            onMessage: t => toast.show(t)
        }
    }
    Loader {
        anchors.fill: parent
        active: root.showOpen
        sourceComponent: Slideshow {
            paths: root.showPaths; index: root.showIndex
            onClosed: i => root.showOpen = false
        }
    }
    Sheet {
        open: root.settingsOpen
        sheetHeight: Math.min(root.height * .8, 640)
        onDismissed: root.settingsOpen = false
        PhotosSettings { anchors.fill: parent; onAddFolder: folderPick.open() }
    }
    Toast { id: toast }
}
