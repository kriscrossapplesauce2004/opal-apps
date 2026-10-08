import QtQuick
import QtQuick.Layouts
import Opal.Material
import "Fixtures.js" as F
import "Model.js" as M
// Photos direction B, "Albums": folders as albums with a mosaic cover; device and cloud sources as a row.
Rectangle {
    color: Theme.base
    component Mosaic: Item {
        property var photos: []
        Grid { anchors.fill: parent; columns: 2; spacing: 3
            Repeater { model: 4
                RoundedImage { required property int index; width: (parent.width - 3) / 2; height: (parent.height - 3) / 2; radius: index === 0 ? 0 : 0; source: photos[index] ? photos[index].u : ""; sourceSize: Qt.size(240, 240) } } }
        layer.enabled: true
    }
    RowLayout {
        anchors.fill: parent; spacing: 0
        NavRail { Layout.fillHeight: true; Layout.preferredWidth: 220; expanded: true; currentIndex: 1; fabIcon: "add"; fabText: "New album"
            destinations: [{title: "Photos", icon: "photo_library"}, {title: "Albums", icon: "photo_album"}, {title: "Screenshots", icon: "screenshot_monitor"}, {title: "Favourites", icon: "favorite"}, {title: "Trash", icon: "delete"}] }
        ColumnLayout {
            Layout.fillWidth: true; Layout.fillHeight: true; Layout.topMargin: 18; Layout.rightMargin: 24; spacing: 18
            MText { text: "Albums"; role: "displayS" }
            RowLayout { spacing: 8
                Chip { text: "On this PC"; selected: true }
                Chip { text: "OnePlus 7"; icon: "smartphone"; glass: false }
                Chip { text: "Samsung T7"; icon: "usb"; glass: false }
                Chip { text: "nas.local"; icon: "dns"; glass: false } }
            GridLayout { Layout.fillWidth: true; columns: 4; rowSpacing: 22; columnSpacing: 20; uniformCellWidths: true
                Repeater {
                    model: [{t: "Wallpapers", n: F.counts.Wallpapers, a: "Wallpapers"}, {t: "Screenshots", n: F.counts.Screenshots, a: "Screenshots"}, {t: "Pictures", n: F.counts.Pictures, a: "Pictures"}, {t: "Favourites", n: 12, a: "Wallpapers", fav: true}, {t: "Phone camera", n: 2318, a: "Wallpapers", sync: true}, {t: "Steam artwork", n: 64, a: "Pictures"}]
                    ColumnLayout { required property var modelData; required property int index; Layout.fillWidth: true; spacing: 8
                        Rectangle { Layout.fillWidth: true; Layout.preferredHeight: width; radius: 28; color: Theme.surfaceHigh; clip: true
                            Mosaic { anchors.fill: parent; photos: M.byAlbum(modelData.a).slice(index % 2 * 4, index % 2 * 4 + 4)
                                layer.effect: null }
                            Rectangle { visible: !!modelData.sync; anchors.right: parent.right; anchors.top: parent.top; anchors.margins: 10; width: 34; height: 34; radius: 17; color: Theme.alpha("#000000", .5)
                                Glyph { anchors.centerIn: parent; name: "sync"; font.pixelSize: 18; color: "#ffffff" } } }
                        MText { text: modelData.t; role: "titleS" }
                        MText { text: modelData.n.toLocaleString() + " items"; role: "labelS"; color: Theme.subtext } } } }
            Item { Layout.fillHeight: true }
        }
    }
}
