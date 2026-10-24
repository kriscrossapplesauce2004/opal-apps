import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Opal.Material
import Opal.Photos
// Photos settings: which folders it watches, what it leaves out, videos and text search in screenshots.
Flickable {
    id: settings
    signal addFolder()
    contentHeight: col.implicitHeight + 32; clip: true; boundsBehavior: Flickable.StopAtBounds
    ScrollBar.vertical: ScrollBar {}
    function home(p) { const h = PhotoLibrary.picturesPath.replace(/\/Pictures$/, ""); return p.indexOf(h) === 0 ? "~" + p.slice(h.length) : p; }
    ColumnLayout {
        id: col
        x: 20; width: parent.width - 40; spacing: 10
        MText { text: qsTr("Photos settings"); role: "title" }
        MText { text: qsTr("Folders"); role: "label"; color: Theme.primary; Layout.topMargin: 6 }
        Repeater {
            model: PhotoLibrary.roots
            ListRow {
                required property string modelData
                required property int index
                Layout.fillWidth: true; icon: modelData === PhotoLibrary.picturesPath ? "photo_library" : "folder"
                title: settings.home(modelData); grouped: true; first: index === 0; last: index === PhotoLibrary.roots.length - 1
                MButton { visible: PhotoLibrary.roots.length > 1; icon: "remove_circle_outline"; compact: true; tooltip: qsTr("Stop watching this folder"); onClicked: PhotoLibrary.removeRoot(modelData) }
            }
        }
        MButton { text: qsTr("Add folder"); icon: "create_new_folder"; tonal: true; onClicked: settings.addFolder() }
        MText { text: qsTr("What to show"); role: "label"; color: Theme.primary; Layout.topMargin: 10 }
        SwitchRow { Layout.fillWidth: true; icon: "movie"; title: qsTr("Videos"); description: qsTr("Show videos next to photos"); checked: PhotoLibrary.includeVideos; onToggled: PhotoLibrary.includeVideos = !PhotoLibrary.includeVideos; first: true; last: false }
        SwitchRow { Layout.fillWidth: true; icon: "sports_esports"; title: qsTr("Leave out game and app folders")
            description: PhotoLibrary.skipped.length ? qsTr("%n folder(s) with textures and icons left out", "", PhotoLibrary.skipped.length) : qsTr("Folders that hold programs, games or code")
            checked: PhotoLibrary.skipSoftware; onToggled: PhotoLibrary.skipSoftware = !PhotoLibrary.skipSoftware; first: false; last: false }
        SwitchRow { Layout.fillWidth: true; icon: "document_scanner"; title: qsTr("Search text in screenshots")
            description: !PhotoLibrary.ocrAvailable ? qsTr("Install tesseract to turn this on") : PhotoLibrary.ocrPending ? qsTr("Reading %n screenshot(s)…", "", PhotoLibrary.ocrPending) : qsTr("Reads text on this computer only")
            enabled: PhotoLibrary.ocrAvailable; checked: PhotoLibrary.ocrEnabled; onToggled: PhotoLibrary.ocrEnabled = !PhotoLibrary.ocrEnabled; first: false; last: true }
        MText { visible: PhotoLibrary.excluded.length > 0; text: qsTr("Hidden folders"); role: "label"; color: Theme.primary; Layout.topMargin: 10 }
        Repeater {
            model: PhotoLibrary.excluded
            ListRow { required property string modelData; Layout.fillWidth: true; icon: "visibility_off"; title: settings.home(modelData)
                MButton { text: qsTr("Show"); compact: true; onClicked: PhotoLibrary.include(modelData) } }
        }
        RowLayout { Layout.topMargin: 10; spacing: 10
            MButton { text: qsTr("Look again now"); icon: "refresh"; enabled: !PhotoLibrary.scanning; onClicked: PhotoLibrary.rescan() }
            MText { text: qsTr("%n item(s)", "", PhotoLibrary.count); role: "labelS"; color: Theme.subtext } }
    }
}
