import QtQuick
import QtQuick.Layouts
import Opal.Material
import "Fixtures.js" as F
// Audiobook player: cover, chapter, skip by 30 s, speed and sleep timer chips, and the chapter list.
Rectangle {
    color: Theme.base
    ColumnLayout {
        anchors.fill: parent; anchors.margins: 20; anchors.topMargin: 12; spacing: 12
        RowLayout { Layout.fillWidth: true
            MButton { icon: "keyboard_arrow_down"; compact: true }
            Item { Layout.fillWidth: true }
            MButton { icon: "bookmark_add"; compact: true }
            MButton { icon: "more_vert"; compact: true } }
        Cover { Layout.alignment: Qt.AlignHCenter; Layout.preferredWidth: 220; Layout.preferredHeight: 330; r: 28; title: F.books[0].t; tone: F.books[0].c }
        MText { Layout.alignment: Qt.AlignHCenter; text: F.books[0].t; role: "headline" }
        MText { Layout.alignment: Qt.AlignHCenter; text: "Chapter 9 · Letter 4 continued"; role: "body"; color: Theme.subtext }
        WavyProgress { Layout.fillWidth: true; value: .62; animate: false; phase: .4; amplitude: 3; thickness: 5 }
        RowLayout { Layout.fillWidth: true
            MText { text: "14:02"; role: "labelS"; color: Theme.subtext }
            Item { Layout.fillWidth: true }
            MText { text: "-8:31 · 5 h 12 m left in book"; role: "labelS"; color: Theme.subtext } }
        RowLayout { Layout.alignment: Qt.AlignHCenter; spacing: 18
            Rectangle { implicitWidth: 60; implicitHeight: 60; radius: 30; color: Theme.secondaryContainer; Glyph { anchors.centerIn: parent; name: "replay_30"; font.pixelSize: 28; color: Theme.onSecondaryContainer } }
            PlayButton { size: 80; playing: false }
            Rectangle { implicitWidth: 60; implicitHeight: 60; radius: 30; color: Theme.secondaryContainer; Glyph { anchors.centerIn: parent; name: "forward_30"; font.pixelSize: 28; color: Theme.onSecondaryContainer } } }
        RowLayout { Layout.alignment: Qt.AlignHCenter; spacing: 8
            Chip { text: "1.25×"; icon: "speed"; glass: false }
            Chip { text: "Sleep · end of chapter"; icon: "bedtime"; selected: true }
            Chip { text: "Chapters"; icon: "list"; glass: false } }
        Item { Layout.fillHeight: true }
    }
}
