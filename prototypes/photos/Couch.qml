import QtQuick
import QtQuick.Layouts
import Opal.Material
import "Fixtures.js" as F
import "Model.js" as M
// Photos on the couch: big justified rows with a focused photo, tabs on the shoulders.
AppFrame {
    destinations: [{title: "Photos", icon: "photo_library"}, {title: "Albums", icon: "photo_album"}, {title: "Favourites", icon: "favorite"}, {title: "Slideshow", icon: "slideshow"}]
    hints: [{b: "A", t: "View"}, {b: "B", t: "Back"}, {b: "X", t: "Favourite"}, {b: "Y", t: "Slideshow"}]
    Column { anchors.fill: parent; anchors.leftMargin: 72; anchors.rightMargin: 72; spacing: 18
        Repeater { model: M.groups(F.photos).slice(0, 2)
            Column { required property var modelData; required property int index; width: parent.width; spacing: 12
                MText { text: modelData.t; font.pixelSize: 30; font.weight: Font.Bold }
                Justified { width: parent.width; photos: modelData.photos.slice(0, 8); rowHeight: 260; gap: 14; tileRadius: 24; focusIndex: index === 0 ? 1 : -1 } } } }
}
