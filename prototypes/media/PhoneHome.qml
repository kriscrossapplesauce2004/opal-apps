import QtQuick
import QtQuick.Layouts
import Opal.Material
import "Fixtures.js" as F
// Media on a phone: home shelves, the mini player pill above the bottom bar.
AppFrame {
    destinations: [{title: "Home", icon: "home"}, {title: "Music", icon: "library_music"}, {title: "Videos", icon: "movie"}, {title: "Radio", icon: "radio"}, {title: "Books", icon: "auto_stories"}]
    Flickable {
        anchors.fill: parent; contentHeight: col.height + 100; clip: true
        ColumnLayout {
            id: col; width: parent.width; spacing: 18
            RowLayout { Layout.fillWidth: true; Layout.margins: 16; Layout.bottomMargin: 0
                MText { text: "Good evening"; role: "headline"; Layout.fillWidth: true }
                MButton { icon: "search"; compact: true }
                MButton { icon: "cast"; compact: true } }
            Flickable { Layout.fillWidth: true; Layout.preferredHeight: 40; contentWidth: chips.width + 32
                Row { id: chips; x: 16; spacing: 8
                    Chip { text: "Continue"; selected: true }
                    Chip { text: "Soundtracks"; glass: false }
                    Chip { text: "Downloaded"; glass: false }
                    Chip { text: "nas.local"; icon: "dns"; glass: false } } }
            // Continue: the book you are listening to as a wide card.
            Rectangle { Layout.fillWidth: true; Layout.leftMargin: 16; Layout.rightMargin: 16; implicitHeight: 132; radius: 28; color: Theme.surfaceHigh
                RowLayout { anchors.fill: parent; anchors.margins: 12; spacing: 14
                    Cover { Layout.preferredWidth: 72; Layout.preferredHeight: 108; r: 14; title: F.books[0].t; tone: F.books[0].c }
                    ColumnLayout { Layout.fillWidth: true; spacing: 3
                        MText { text: F.books[0].t; role: "title" }
                        MText { text: F.books[0].ch + " · " + F.books[0].left; role: "labelS"; color: Theme.subtext }
                        Item { Layout.fillHeight: true }
                        WavyProgress { Layout.fillWidth: true; value: F.books[0].p; animate: false; amplitude: 2; wavelength: 16; thickness: 3 } }
                    PlayButton { size: 48; playing: false } } }
            MText { text: "Recently added"; role: "title"; Layout.leftMargin: 16 }
            Flickable { Layout.fillWidth: true; Layout.preferredHeight: 190; contentWidth: rec.width + 32
                Row { id: rec; x: 16; spacing: 12
                    Repeater { model: F.albums.slice(0, 5)
                        Column { required property var modelData; spacing: 6
                            Cover { width: 150; height: 150; source: modelData.img }
                            MText { width: 150; text: modelData.t; role: "label" } } } } }
            MText { text: "Radio"; role: "title"; Layout.leftMargin: 16 }
            GridLayout { Layout.fillWidth: true; Layout.leftMargin: 16; Layout.rightMargin: 16; columns: 2; rowSpacing: 10; columnSpacing: 10
                Repeater { model: F.radio
                    Rectangle { required property var modelData; Layout.fillWidth: true; implicitHeight: 64; radius: 32; color: Theme.surfaceHigh
                        RowLayout { anchors.fill: parent; anchors.margins: 8; spacing: 10
                            Rectangle { implicitWidth: 48; implicitHeight: 48; radius: 24; color: Theme.alpha(modelData.c, .25); Glyph { anchors.centerIn: parent; name: modelData.i; color: modelData.c } }
                            ColumnLayout { spacing: 0; Layout.fillWidth: true; MText { Layout.fillWidth: true; text: modelData.t; role: "titleS" } MText { Layout.fillWidth: true; text: modelData.a; role: "labelS"; color: Theme.subtext } } } } } }
        }
    }
    MiniPlayer { anchors.left: parent.left; anchors.right: parent.right; anchors.bottom: parent.bottom; anchors.margins: 10; now: F.now }
}
