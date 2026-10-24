import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Opal.Material
import Opal.Media
// Audiobooks: a shelf of books with how far you got. One folder is one book; its files are the chapters.
Item {
    id: page
    property var app
    readonly property bool phone: width < 600
    readonly property var list: MediaLibrary.revision + MediaLibrary.progressRevision >= 0 ? MediaLibrary.books() : []
    ColumnLayout {
        anchors.fill: parent; anchors.topMargin: page.phone ? 8 : 18; spacing: 12
        RowLayout { Layout.fillWidth: true; Layout.leftMargin: page.phone ? 16 : 8; Layout.rightMargin: 16
            ColumnLayout { spacing: 0
                MText { text: qsTr("Audiobooks"); role: page.phone ? "headline" : "displayS" }
                MText { text: qsTr("%n book(s)", "", page.list.length); role: "labelS"; color: Theme.subtext } }
            Item { Layout.fillWidth: true } }
        GridView {
            id: grid
            Layout.fillWidth: true; Layout.fillHeight: true; Layout.leftMargin: page.phone ? 8 : 0; Layout.rightMargin: 8
            clip: true; boundsBehavior: Flickable.StopAtBounds; bottomMargin: 100
            readonly property int cols: Math.max(2, Math.floor(width / (page.phone ? 160 : 190)))
            cellWidth: Math.floor(width / cols); cellHeight: cellWidth * 1.5 + 70
            model: page.list
            ScrollBar.vertical: ScrollBar {}
            delegate: Item {
                required property var modelData
                width: grid.cellWidth; height: grid.cellHeight
                ColumnLayout { anchors.fill: parent; anchors.margins: 9; spacing: 6
                    Cover { Layout.fillWidth: true; Layout.preferredHeight: width * 1.5; r: 18; source: modelData.cover || ""; title: modelData.title || ""; playing: Player.bookKey === modelData.key && Player.playing
                        HoverHandler { cursorShape: Qt.PointingHandCursor }
                        TapHandler { onTapped: page.app.openBook(parent.parent.modelData.key) } }
                    MText { Layout.fillWidth: true; text: modelData.title || ""; role: "titleS"; elide: Text.ElideRight }
                    MText { Layout.fillWidth: true; text: modelData.finished ? qsTr("Finished") : modelData.started ? qsTr("%1 left").arg(modelData.left) : modelData.author; role: "labelS"; color: Theme.subtext; elide: Text.ElideRight }
                    WavyProgress { visible: modelData.started && !modelData.finished; Layout.fillWidth: true; value: modelData.progress; animate: false; amplitude: 1.5; wavelength: 14; thickness: 3 } }
                Accessible.role: Accessible.Button; Accessible.name: modelData.title || ""
            }
        }
    }
    ColumnLayout {
        anchors.centerIn: parent; visible: MediaLibrary.loaded && page.list.length === 0; spacing: 10; width: Math.min(parent.width - 48, 440)
        Glyph { Layout.alignment: Qt.AlignHCenter; name: "auto_stories"; font.pixelSize: 48; color: Theme.subtext }
        MText { Layout.alignment: Qt.AlignHCenter; text: qsTr("No audiobooks yet"); role: "title" }
        MText { Layout.fillWidth: true; horizontalAlignment: Text.AlignHCenter; wrapMode: Text.Wrap; color: Theme.subtext; text: qsTr("Put each book in its own folder under ~/Audiobooks, or add a books folder in Settings.") }
        MButton { Layout.alignment: Qt.AlignHCenter; text: qsTr("Settings"); icon: "settings"; tonal: true; onClicked: page.app.openSettings() }
    }
}
