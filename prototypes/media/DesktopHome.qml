import QtQuick
import QtQuick.Layouts
import Opal.Material
import "Fixtures.js" as F
// Media direction C, "Home": one page that mixes everything you were in the middle of; the player floats as a bar.
Rectangle {
    color: Theme.base
    component Shelf: ColumnLayout { property string title; property string icon; default property alias items: row.data; spacing: 10; Layout.fillWidth: true
        RowLayout { Glyph { name: icon; color: Theme.primary; font.pixelSize: 20 } MText { text: title; role: "title" } Item { Layout.fillWidth: true } MText { text: "See all"; role: "label"; color: Theme.primary } }
        RowLayout { id: row; spacing: 14 } }
    RowLayout {
        anchors.fill: parent; spacing: 0
        NavRail { Layout.fillHeight: true; Layout.preferredWidth: 96; currentIndex: 0; fabIcon: "search"
            destinations: [{title: "Home", icon: "home"}, {title: "Music", icon: "library_music"}, {title: "Videos", icon: "movie"}, {title: "Radio", icon: "radio"}, {title: "Books", icon: "auto_stories"}] }
        ColumnLayout {
            Layout.fillWidth: true; Layout.fillHeight: true; Layout.topMargin: 20; Layout.rightMargin: 24; spacing: 22
            MText { text: "Good evening"; role: "displayS" }
            Shelf { title: "Continue"; icon: "resume"
                Repeater { model: F.videos.slice(0, 1)
                    Rectangle { required property var modelData; implicitWidth: 360; implicitHeight: 200; radius: 28; color: Theme.surfaceHigh
                        Cover { anchors.fill: parent; r: 28; source: modelData.img }
                        Rectangle { anchors.fill: parent; radius: 28; gradient: Gradient { GradientStop { position: .45; color: "transparent" } GradientStop { position: 1; color: Qt.rgba(0, 0, 0, .75) } } }
                        Column { anchors.left: parent.left; anchors.bottom: parent.bottom; anchors.margins: 16; spacing: 6; width: parent.width - 32
                            MText { text: modelData.t; role: "titleS"; color: "#ffffff" }
                            Rectangle { width: parent.width; height: 4; radius: 2; color: Qt.rgba(1, 1, 1, .3); Rectangle { width: parent.width * modelData.p; height: 4; radius: 2; color: Theme.primary } } } } }
                Repeater { model: F.books.slice(0, 1)
                    Rectangle { required property var modelData; implicitWidth: 360; implicitHeight: 200; radius: 28; color: Theme.surfaceHigh
                        RowLayout { anchors.fill: parent; anchors.margins: 16; spacing: 16
                            Cover { Layout.preferredWidth: 112; Layout.preferredHeight: 168; r: 18; title: modelData.t; tone: modelData.c }
                            ColumnLayout { Layout.fillWidth: true; spacing: 4
                                MText { Layout.fillWidth: true; text: modelData.t; role: "title" }
                                MText { Layout.fillWidth: true; text: modelData.a; role: "labelS"; color: Theme.subtext }
                                Item { Layout.fillHeight: true }
                                MText { text: modelData.ch; role: "label" }
                                WavyProgress { Layout.fillWidth: true; value: modelData.p; animate: false; amplitude: 2; wavelength: 18; thickness: 3 }
                                MText { text: modelData.left; role: "labelS"; color: Theme.subtext } } } } }
                Repeater { model: F.albums.slice(3, 4)
                    Cover { required property var modelData; implicitWidth: 200; implicitHeight: 200; r: 28; source: modelData.img } } }
            Shelf { title: "Recently added"; icon: "new_releases"
                Repeater { model: F.albums.slice(0, 5)
                    ColumnLayout { required property var modelData; spacing: 6
                        Cover { Layout.preferredWidth: 160; Layout.preferredHeight: 160; source: modelData.img }
                        MText { Layout.preferredWidth: 160; text: modelData.t; role: "label" } } } }
            Shelf { title: "Radio"; icon: "radio"
                Repeater { model: F.radio
                    Rectangle { required property var modelData; implicitWidth: 210; implicitHeight: 72; radius: 36; color: Theme.surfaceHigh
                        RowLayout { anchors.fill: parent; anchors.margins: 10; spacing: 12
                            Rectangle { implicitWidth: 52; implicitHeight: 52; radius: 26; color: Theme.alpha(modelData.c, .25); Glyph { anchors.centerIn: parent; name: modelData.i; color: modelData.c } }
                            ColumnLayout { spacing: 0; MText { text: modelData.t; role: "titleS" } MText { text: modelData.a; role: "labelS"; color: Theme.subtext } } } } } }
            Item { Layout.fillHeight: true }
        }
    }
    MiniPlayer { anchors.horizontalCenter: parent.horizontalCenter; anchors.bottom: parent.bottom; anchors.bottomMargin: 20; width: 560; now: F.now }
}
