import QtQuick
import QtQuick.Layouts
import Opal.Material
import "Fixtures.js" as F
// Media on the couch: a hero for what you were watching, then shelves; the focused tile grows.
AppFrame {
    destinations: [{title: "Home", icon: "home"}, {title: "Music", icon: "library_music"}, {title: "Videos", icon: "movie"}, {title: "Radio", icon: "radio"}, {title: "Books", icon: "auto_stories"}]
    hints: [{b: "A", t: "Play"}, {b: "B", t: "Back"}, {b: "X", t: "Queue"}, {b: "Y", t: "Search"}]
    ColumnLayout {
        anchors.fill: parent; anchors.leftMargin: 72; anchors.rightMargin: 72; spacing: 28
        Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 380; radius: 40; color: Theme.surfaceHigh; clip: true
            Cover { anchors.fill: parent; r: 40; source: F.albums[2].img }
            Rectangle { anchors.fill: parent; radius: 40; gradient: Gradient { orientation: Gradient.Horizontal; GradientStop { position: 0; color: Qt.rgba(0, 0, 0, .8) } GradientStop { position: .6; color: "transparent" } } }
            ColumnLayout { anchors.left: parent.left; anchors.verticalCenter: parent.verticalCenter; anchors.leftMargin: 48; spacing: 14
                MText { text: "Continue listening"; font.pixelSize: 22; color: Qt.rgba(1, 1, 1, .8) }
                MText { text: "DELTARUNE Chapters 3+4"; font.pixelSize: 52; font.weight: Font.Bold; font.variableAxes: ({"wdth": 115, "ROND": 80}); color: "#ffffff" }
                MText { text: "Track 23 of 78 · Toby Fox"; font.pixelSize: 22; color: Qt.rgba(1, 1, 1, .8) }
                RowLayout { spacing: 16; Layout.topMargin: 10
                    Rectangle { implicitHeight: 72; implicitWidth: pr.implicitWidth + 56; radius: 36; color: Theme.primary; border.width: 4; border.color: Qt.rgba(1, 1, 1, .7)
                        RowLayout { id: pr; anchors.centerIn: parent; spacing: 10; Glyph { name: "play_arrow"; fill: 1; font.pixelSize: 36; color: Theme.onPrimary } MText { text: "Resume"; font.pixelSize: 26; font.weight: Font.Bold; color: Theme.onPrimary } } }
                    Rectangle { implicitHeight: 72; implicitWidth: 72; radius: 36; color: Qt.rgba(1, 1, 1, .2); Glyph { anchors.centerIn: parent; name: "shuffle"; font.pixelSize: 32; color: "#ffffff" } } } } }
        MText { text: "Albums"; font.pixelSize: 30; font.weight: Font.Bold }
        RowLayout { spacing: 26
            Repeater { model: F.albums.slice(0, 6)
                ColumnLayout { required property var modelData; spacing: 10
                    Cover { Layout.preferredWidth: 220; Layout.preferredHeight: 220; r: 30; source: modelData.img }
                    MText { Layout.preferredWidth: 220; text: modelData.t; font.pixelSize: 19; font.weight: Font.Medium } } } }
        Item { Layout.fillHeight: true }
    }
}
