import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts
import Opal.Material
import Opal.Media
// Media settings: the folders for music, audiobooks and videos.
Flickable {
    id: settings
    contentHeight: col.implicitHeight + 32; clip: true; boundsBehavior: Flickable.StopAtBounds
    ScrollBar.vertical: ScrollBar {}
    property string addKind: "music"
    FolderDialog { id: pick; title: qsTr("Choose a folder"); onAccepted: MediaLibrary.addRoot(settings.addKind, selectedFolder.toString()) }
    function short(p) { const h = (MediaLibrary.musicRoots[0] || "").replace(/\/Music$/, ""); return h && p.indexOf(h) === 0 ? "~" + p.slice(h.length) : p; }
    ColumnLayout {
        id: col
        x: 20; width: parent.width - 40; spacing: 10
        MText { text: qsTr("Media settings"); role: "title" }
        Repeater {
            model: [{kind: "music", t: qsTr("Music folders"), roots: MediaLibrary.musicRoots, icon: "library_music"}, {kind: "books", t: qsTr("Audiobook folders"), roots: MediaLibrary.bookRoots, icon: "auto_stories"},
                    {kind: "videos", t: qsTr("Video folders"), roots: MediaLibrary.videoRoots, icon: "movie"}]
            ColumnLayout {
                id: folderGroup
                required property var modelData
                Layout.fillWidth: true; spacing: 6
                MText { text: modelData.t; role: "label"; color: Theme.primary; Layout.topMargin: 8 }
                Repeater { model: parent.modelData.roots
                    ListRow { id: rootRow; required property string modelData; Layout.fillWidth: true; icon: folderGroup.modelData.icon; title: settings.short(modelData)
                        MButton { icon: "remove_circle_outline"; compact: true; tooltip: qsTr("Remove"); onClicked: MediaLibrary.removeRoot(folderGroup.modelData.kind, rootRow.modelData) } } }
                MButton { text: qsTr("Add folder"); icon: "create_new_folder"; tonal: true; onClicked: { settings.addKind = folderGroup.modelData.kind; pick.open(); } }
            }
        }
        MText { visible: MediaLibrary.skipped.length > 0; Layout.fillWidth: true; Layout.topMargin: 8; wrapMode: Text.Wrap; role: "bodyS"; color: Theme.subtext
            text: qsTr("%n folder(s) with programs, games or code were left out, so their sound effects stay out of your library.", "", MediaLibrary.skipped.length) }
        RowLayout { Layout.topMargin: 10; spacing: 10
            MButton { text: qsTr("Look again now"); icon: "refresh"; enabled: !MediaLibrary.scanning; onClicked: MediaLibrary.rescan() }
            WavyProgress { visible: MediaLibrary.scanning; Layout.preferredWidth: 100; indeterminate: true } }
    }
}
