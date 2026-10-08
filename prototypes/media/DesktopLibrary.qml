import QtQuick
import QtQuick.Layouts
import Opal.Material
import "Fixtures.js" as F
// Media direction A, "Library": rail, filter chips, album grid, and a now-playing panel with art, wavy seek and the queue.
Rectangle {
    color: Theme.base
    RowLayout {
        anchors.fill: parent; spacing: 0
        NavRail { Layout.fillHeight: true; expanded: true; Layout.preferredWidth: 220; currentIndex: 1; fabIcon: "search"; fabText: "Search"
            destinations: [{title: "Home", icon: "home"}, {title: "Music", icon: "library_music"}, {title: "Videos", icon: "movie"}, {title: "Radio", icon: "radio"}, {title: "Audiobooks", icon: "auto_stories"}, {title: "nas.local", icon: "dns", badge: ""}] }
        ColumnLayout {
            Layout.fillWidth: true; Layout.fillHeight: true; Layout.topMargin: 18; Layout.rightMargin: 12; spacing: 14
            RowLayout { Layout.fillWidth: true
                MText { text: "Music"; role: "displayS"; Layout.fillWidth: true }
                SegmentedButtons { current: "albums"; options: [{id: "albums", name: "Albums"}, {id: "artists", name: "Artists"}, {id: "songs", name: "Songs"}, {id: "lists", name: "Playlists"}] } }
            RowLayout { spacing: 8
                Chip { text: "All"; selected: true; glass: false }
                Chip { text: "Soundtracks"; icon: "sports_esports"; glass: false }
                Chip { text: "Recently added"; icon: "schedule"; glass: false }
                Chip { text: "Downloaded"; icon: "download_done"; glass: false }
                Chip { text: "nas.local"; icon: "dns"; glass: false } }
            GridLayout {
                Layout.fillWidth: true; columns: 4; rowSpacing: 18; columnSpacing: 18; uniformCellWidths: true
                Repeater {
                    model: F.albums.concat(F.albums.slice(0, 2))
                    ColumnLayout { required property var modelData; required property int index; spacing: 6; Layout.fillWidth: true
                        Cover { Layout.fillWidth: true; Layout.preferredHeight: width; source: modelData.img; title: modelData.t
                            Rectangle { visible: index === 0; anchors.right: parent.right; anchors.bottom: parent.bottom; anchors.margins: 10; width: 44; height: 44; radius: 16; color: Theme.primary
                                Glyph { anchors.centerIn: parent; name: "graphic_eq"; font.pixelSize: 24; color: Theme.onPrimary } } }
                        MText { Layout.fillWidth: true; text: modelData.t; role: "titleS"; font.weight: index === 0 ? Font.Bold : Font.DemiBold; color: index === 0 ? Theme.primary : Theme.text }
                        MText { Layout.fillWidth: true; text: modelData.a + " · " + modelData.y; role: "labelS"; color: Theme.subtext } } } }
            Item { Layout.fillHeight: true }
        }
        // Now playing panel.
        Rectangle {
            Layout.preferredWidth: 380; Layout.fillHeight: true; Layout.margins: 12; radius: 32; color: Theme.surface
            ColumnLayout {
                anchors.fill: parent; anchors.margins: 20; spacing: 14
                RowLayout { Layout.fillWidth: true
                    MText { text: "Now playing"; role: "label"; color: Theme.primary; Layout.fillWidth: true }
                    Chip { text: "This PC"; icon: "speaker"; trailingIcon: "expand_more"; glass: false } }
                Cover { Layout.fillWidth: true; Layout.preferredHeight: width; r: 28; source: F.now.img }
                ColumnLayout { spacing: 2; Layout.fillWidth: true
                    MText { Layout.fillWidth: true; text: F.now.t; role: "headline" }
                    MText { Layout.fillWidth: true; text: F.now.a + " · " + F.now.al; role: "body"; color: Theme.subtext } }
                Transport { Layout.fillWidth: true; now: F.now; size: 60 }
                MText { text: "Up next"; role: "label"; color: Theme.subtext; Layout.topMargin: 6 }
                Repeater {
                    model: F.queue.slice(1, 5)
                    RowLayout { required property var modelData; Layout.fillWidth: true; spacing: 12
                        MText { text: modelData.n; role: "labelS"; color: Theme.muted; Layout.preferredWidth: 20 }
                        MText { Layout.fillWidth: true; text: modelData.t; role: "bodyS" }
                        MText { text: modelData.d; role: "labelS"; color: Theme.subtext } } }
                Item { Layout.fillHeight: true }
            }
        }
    }
}
