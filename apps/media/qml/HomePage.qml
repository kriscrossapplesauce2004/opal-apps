import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Opal.Material
import Opal.Media
// Home: pick up where you left off, albums played lately, new arrivals and favourite stations.
Flickable {
    id: home
    property var app
    readonly property bool phone: width < 600
    readonly property int rev: MediaLibrary.revision + MediaLibrary.progressRevision
    readonly property var books: rev >= 0 ? MediaLibrary.books().filter(b => b.started && !b.finished) : []
    readonly property var videos: rev >= 0 ? MediaLibrary.videos().filter(v => v.position > 0).slice(0, 6) : []
    readonly property var recent: rev >= 0 ? MediaLibrary.recentAlbums.map(k => MediaLibrary.album(k)).filter(a => !!a.key).slice(0, 12) : []
    readonly property var added: rev >= 0 ? MediaLibrary.albums("", "", "added").slice(0, 14) : []
    readonly property string greeting: { const h = new Date().getHours(); return h < 5 ? qsTr("Good night") : h < 12 ? qsTr("Good morning") : h < 18 ? qsTr("Good afternoon") : qsTr("Good evening"); }
    contentHeight: col.implicitHeight + 120; clip: true; boundsBehavior: Flickable.StopAtBounds
    ScrollBar.vertical: ScrollBar {}
    ColumnLayout {
        id: col
        width: parent.width; spacing: 18
        RowLayout { Layout.fillWidth: true; Layout.leftMargin: home.phone ? 16 : 8; Layout.rightMargin: 16; Layout.topMargin: home.phone ? 8 : 18
            MText { text: home.greeting; role: home.phone ? "headline" : "displayS"; Layout.fillWidth: true }
            MButton { icon: "search"; compact: true; tooltip: qsTr("Search"); onClicked: home.app.openSearch() } }
        // Continue: an audiobook or a video in progress, as wide cards.
        Flow {
            visible: home.books.length + home.videos.length > 0
            Layout.fillWidth: true; Layout.leftMargin: home.phone ? 16 : 8; Layout.rightMargin: 16; spacing: 12
            Repeater { model: home.books.slice(0, 2)
                ContinueCard { required property var modelData; cover: modelData.cover; title: modelData.title; detail: qsTr("Chapter %1 · %2 left").arg(modelData.chapter + 1).arg(modelData.left); progress: modelData.progress
                    onActivated: Player.playBook(modelData.key) } }
            Repeater { model: home.videos.slice(0, 2)
                ContinueCard { required property var modelData; cover: modelData.thumb; title: modelData.title; detail: qsTr("%1 of %2").arg(MediaLibrary.formatDuration(modelData.position)).arg(modelData.length); progress: modelData.progress; wideArt: true
                    onActivated: home.app.playVideo(modelData) } }
        }
        Shelf { title: qsTr("Jump back in"); albums: home.recent }
        Shelf { title: qsTr("Recently added"); albums: home.added }
        MText { visible: Radio.favourites.length > 0; text: qsTr("Your stations"); role: "title"; Layout.leftMargin: home.phone ? 16 : 8 }
        GridLayout {
            visible: Radio.favourites.length > 0
            Layout.fillWidth: true; Layout.leftMargin: home.phone ? 16 : 8; Layout.rightMargin: 16; columns: home.phone ? 2 : 4; rowSpacing: 10; columnSpacing: 10
            Repeater { model: Radio.favourites.slice(0, 8)
                Rectangle { required property var modelData; Layout.fillWidth: true; implicitHeight: 64; radius: 32
                    color: Player.current.url === modelData.url ? Theme.secondaryContainer : Theme.surfaceHigh
                    RowLayout { anchors.fill: parent; anchors.margins: 8; spacing: 10
                        Cover { Layout.preferredWidth: 48; Layout.preferredHeight: 48; r: 24; source: parent.parent.modelData.cover || ""; title: parent.parent.modelData.name || ""; icon: "radio" }
                        ColumnLayout { spacing: 0; Layout.fillWidth: true
                            MText { Layout.fillWidth: true; text: parent.parent.parent.modelData.name || ""; role: "titleS"; elide: Text.ElideRight }
                            MText { Layout.fillWidth: true; text: parent.parent.parent.modelData.artist || ""; role: "labelS"; color: Theme.subtext; elide: Text.ElideRight } } }
                    HoverHandler { cursorShape: Qt.PointingHandCursor }
                    TapHandler { onTapped: Player.playRadio(parent.modelData) }
                    Accessible.role: Accessible.Button; Accessible.name: modelData.name || "" } } }
        // A first start with nothing found.
        ColumnLayout {
            visible: MediaLibrary.loaded && MediaLibrary.count === 0 && Radio.favourites.length === 0
            Layout.alignment: Qt.AlignHCenter; Layout.topMargin: 60; spacing: 12; Layout.maximumWidth: 460
            Rectangle { Layout.alignment: Qt.AlignHCenter; implicitWidth: 96; implicitHeight: 96; radius: 34; color: Theme.secondaryContainer
                Glyph { anchors.centerIn: parent; name: "library_music"; font.pixelSize: 48; color: Theme.onSecondaryContainer } }
            MText { Layout.alignment: Qt.AlignHCenter; text: qsTr("Your library is empty"); role: "title" }
            MText { Layout.fillWidth: true; horizontalAlignment: Text.AlignHCenter; wrapMode: Text.Wrap; color: Theme.subtext
                text: qsTr("Media plays what is in your Music, Videos and Audiobooks folders. Add another folder, or listen to the radio.") }
            RowLayout { Layout.alignment: Qt.AlignHCenter; spacing: 8
                MButton { text: qsTr("Add a folder"); icon: "create_new_folder"; filled: true; onClicked: home.app.openSettings() }
                MButton { text: qsTr("Radio"); icon: "radio"; tonal: true; onClicked: home.app.go("radio") } }
        }
    }
    component Shelf: ColumnLayout {
        id: shelf
        property string title
        property var albums: []
        visible: albums.length > 0
        Layout.fillWidth: true; spacing: 10
        MText { text: shelf.title; role: "title"; Layout.leftMargin: home.phone ? 16 : 8 }
        ListView {
            Layout.fillWidth: true; Layout.preferredHeight: (home.phone ? 150 : 180) + 52
            orientation: ListView.Horizontal; spacing: 14; clip: true; leftMargin: home.phone ? 16 : 8; rightMargin: 16
            boundsBehavior: Flickable.StopAtBounds
            model: shelf.albums
            delegate: ColumnLayout {
                required property var modelData
                width: home.phone ? 150 : 180; spacing: 6
                Cover { Layout.fillWidth: true; Layout.preferredHeight: width; source: modelData.cover || ""; title: modelData.title || ""; playing: Player.current.albumKey === modelData.key
                    scale: shelfTap.pressed ? .97 : 1
                    Behavior on scale { NumberAnimation { duration: Theme.motion(Theme.durShort) } }
                    TapHandler { id: shelfTap; onTapped: home.app.openAlbum(parent.parent.modelData.key) }
                    HoverHandler { id: shelfHover; cursorShape: Qt.PointingHandCursor }
                    PlayButton { visible: shelfHover.hovered; anchors.left: parent.left; anchors.bottom: parent.bottom; anchors.margins: 10; size: 44; playing: false
                        onClicked: Player.playTracks(MediaLibrary.album(parent.parent.modelData.key).tracks, 0) } }
                MText { Layout.fillWidth: true; text: modelData.title || ""; role: "titleS"; elide: Text.ElideRight }
                MText { Layout.fillWidth: true; text: modelData.artist || ""; role: "labelS"; color: Theme.subtext; elide: Text.ElideRight }
            }
        }
    }
    component ContinueCard: Rectangle {
        id: cc
        property string cover
        property string title
        property string detail
        property real progress: 0
        property bool wideArt: false
        signal activated()
        width: home.phone ? home.width - 32 : 420; height: 120; radius: 28; color: Theme.surfaceHigh
        RowLayout { anchors.fill: parent; anchors.margins: 12; spacing: 14
            Cover { Layout.preferredWidth: cc.wideArt ? 152 : 64; Layout.preferredHeight: cc.wideArt ? 86 : 96; r: 14; source: cc.cover || ""; title: cc.title; icon: cc.wideArt ? "movie" : "" }
            ColumnLayout { Layout.fillWidth: true; spacing: 3
                MText { Layout.fillWidth: true; text: cc.title; role: "titleS"; elide: Text.ElideRight; maximumLineCount: 2; wrapMode: Text.Wrap }
                MText { Layout.fillWidth: true; text: cc.detail; role: "labelS"; color: Theme.subtext }
                Item { Layout.fillHeight: true }
                WavyProgress { Layout.fillWidth: true; value: cc.progress; animate: false; amplitude: 2; wavelength: 16; thickness: 3 } }
            PlayButton { size: 48; playing: false; onClicked: cc.activated() } }
        TapHandler { onTapped: cc.activated() }
        Accessible.role: Accessible.Button; Accessible.name: qsTr("Continue %1").arg(title)
    }
}
