import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Opal.Material
import Opal.Media
// Videos: a poster grid, newest first, with how far you watched.
Item {
    id: page
    property var app
    property string query: ""
    readonly property bool phone: width < 600
    readonly property var list: MediaLibrary.revision + MediaLibrary.progressRevision >= 0 ? MediaLibrary.videos(query) : []
    ColumnLayout {
        anchors.fill: parent; anchors.topMargin: page.phone ? 8 : 18; spacing: 12
        RowLayout {
            Layout.fillWidth: true; Layout.leftMargin: page.phone ? 16 : 8; Layout.rightMargin: 16
            ColumnLayout { spacing: 0
                MText { text: qsTr("Videos"); role: page.phone ? "headline" : "displayS" }
                MText { text: qsTr("%n video(s)", "", MediaLibrary.videoCount); role: "labelS"; color: Theme.subtext } }
            Item { Layout.fillWidth: true }
            SearchField { visible: !page.phone; Layout.preferredWidth: Math.min(300, page.width * .3); placeholder: qsTr("Filter"); onTextChanged: page.query = text }
        }
        GridView {
            id: grid
            Layout.fillWidth: true; Layout.fillHeight: true; Layout.leftMargin: page.phone ? 8 : 0; Layout.rightMargin: 8
            clip: true; boundsBehavior: Flickable.StopAtBounds; bottomMargin: 100
            readonly property int cols: Math.max(1, Math.floor(width / (page.phone ? 360 : 300)))
            cellWidth: Math.floor(width / cols); cellHeight: cellWidth * 9 / 16 + 64
            model: page.list
            ScrollBar.vertical: ScrollBar {}
            delegate: Item {
                required property var modelData
                width: grid.cellWidth; height: grid.cellHeight
                ColumnLayout {
                    anchors.fill: parent; anchors.margins: 9; spacing: 6
                    Item {
                        Layout.fillWidth: true; Layout.preferredHeight: width * 9 / 16
                        Cover { anchors.fill: parent; r: 20; source: modelData.thumb || ""; title: modelData.title || ""; icon: "movie"
                            scale: vTap.pressed ? .97 : 1
                            Behavior on scale { NumberAnimation { duration: Theme.motion(Theme.durShort) } } }
                        Rectangle { anchors.right: parent.right; anchors.bottom: parent.bottom; anchors.margins: 10; implicitWidth: lenText.implicitWidth + 14; implicitHeight: 24; radius: 12; color: Qt.rgba(0, 0, 0, .65)
                            MText { id: lenText; anchors.centerIn: parent; text: modelData.length || ""; role: "labelS"; color: "#ffffff" } }
                        Rectangle { visible: modelData.progress > 0; anchors.left: parent.left; anchors.right: parent.right; anchors.bottom: parent.bottom; anchors.margins: 12; anchors.rightMargin: 70; height: 4; radius: 2; color: Qt.rgba(1, 1, 1, .35)
                            Rectangle { width: parent.width * modelData.progress; height: parent.height; radius: 2; color: Theme.primary } }
                        Rectangle { visible: vHover.hovered; anchors.centerIn: parent; width: 56; height: 56; radius: 28; color: Theme.primary
                            Glyph { anchors.centerIn: parent; name: "play_arrow"; fill: 1; font.pixelSize: 32; color: Theme.onPrimary } }
                        HoverHandler { id: vHover; cursorShape: Qt.PointingHandCursor }
                        TapHandler { id: vTap; onTapped: page.app.playVideo(parent.parent.parent.modelData) }
                    }
                    MText { Layout.fillWidth: true; text: modelData.title || ""; role: "titleS"; elide: Text.ElideRight }
                    MText { Layout.fillWidth: true; text: (modelData.folder || "") + (modelData.height ? " · " + modelData.height + "p" : ""); role: "labelS"; color: Theme.subtext; elide: Text.ElideRight }
                }
                Accessible.role: Accessible.Button; Accessible.name: modelData.title || ""
            }
        }
    }
    MText { anchors.centerIn: parent; visible: MediaLibrary.loaded && page.list.length === 0; text: page.query ? qsTr("No videos match") : qsTr("No videos in your Videos folder"); role: "title"; color: Theme.subtext }
}
