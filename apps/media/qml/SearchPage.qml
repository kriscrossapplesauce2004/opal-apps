import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Opal.Material
import Opal.Media
// Search across music, videos and audiobooks as you type.
Item {
    id: page
    property var app
    signal back()
    readonly property bool phone: width < 600
    readonly property var r: MediaLibrary.revision >= 0 && field.text.trim().length > 0 ? MediaLibrary.search(field.text) : ({})
    function focusField() { field.input.forceActiveFocus(); }
    ColumnLayout {
        anchors.fill: parent; anchors.topMargin: page.phone ? 8 : 18; spacing: 12
        RowLayout { Layout.fillWidth: true; Layout.leftMargin: page.phone ? 8 : 8; Layout.rightMargin: 16; spacing: 8
            MButton { icon: "arrow_back"; compact: true; tooltip: qsTr("Back"); onClicked: page.back() }
            SearchField { id: field; Layout.fillWidth: true; placeholder: qsTr("Songs, albums, artists, videos, books"); Keys.onEscapePressed: page.back() } }
        Flickable {
            Layout.fillWidth: true; Layout.fillHeight: true; contentHeight: col.implicitHeight + 120; clip: true; boundsBehavior: Flickable.StopAtBounds
            ScrollBar.vertical: ScrollBar {}
            ColumnLayout {
                id: col
                width: parent.width - (page.phone ? 16 : 24); x: page.phone ? 8 : 8; spacing: 10
                Section { title: qsTr("Albums"); visible: (page.r.albums || []).length > 0 }
                Flow { Layout.fillWidth: true; spacing: 12
                    Repeater { model: page.r.albums || []
                        Column { required property var modelData; width: 150; spacing: 4
                            Cover { width: 150; height: 150; source: modelData.cover || ""; title: modelData.title || ""; TapHandler { onTapped: page.app.openAlbum(parent.parent.modelData.key) } }
                            MText { width: 150; text: modelData.title || ""; role: "titleS"; elide: Text.ElideRight }
                            MText { width: 150; text: modelData.artist || ""; role: "labelS"; color: Theme.subtext; elide: Text.ElideRight } } } }
                Section { title: qsTr("Artists"); visible: (page.r.artists || []).length > 0 }
                Flow { Layout.fillWidth: true; spacing: 8
                    Repeater { model: page.r.artists || []
                        Chip { required property var modelData; text: modelData.name; icon: "person"; glass: false; onClicked: page.app.openArtist(modelData.name) } } }
                Section { title: qsTr("Songs"); visible: (page.r.songs || []).length > 0 }
                Repeater { model: page.r.songs || []
                    TrackRow { required property var modelData; required property int index; Layout.fillWidth: true; track: modelData; showCover: true; showAlbum: true
                        onActivated: Player.playTracks(page.r.songs, index); onMenuRequested: (t, x, y) => page.app.trackMenu(t, x, y) } }
                Section { title: qsTr("Videos"); visible: (page.r.videos || []).length > 0 }
                Repeater { model: page.r.videos || []
                    ListRow { required property var modelData; Layout.fillWidth: true; image: modelData.thumb || ""; icon: "movie"; title: modelData.title; subtitle: modelData.length; onClicked: page.app.playVideo(modelData) } }
                Section { title: qsTr("Audiobooks"); visible: (page.r.books || []).length > 0 }
                Repeater { model: page.r.books || []
                    ListRow { required property var modelData; Layout.fillWidth: true; image: modelData.cover || ""; icon: "auto_stories"; title: modelData.title; subtitle: modelData.author; onClicked: page.app.openBook(modelData.key) } }
                MText { visible: field.text.trim() !== "" && Object.keys(page.r).every(k => (page.r[k] || []).length === 0); text: qsTr("Nothing matches “%1”").arg(field.text); color: Theme.subtext; Layout.topMargin: 30; Layout.alignment: Qt.AlignHCenter }
            }
        }
    }
    component Section: MText { property string title; text: title; role: "title"; Layout.topMargin: 8 }
}
