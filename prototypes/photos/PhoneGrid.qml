import QtQuick
import QtQuick.Layouts
import Opal.Material
import "Fixtures.js" as F
import "Model.js" as M
// Photos on a phone: search, day groups with justified rows, bottom navigation.
AppFrame {
    destinations: [{title: "Photos", icon: "photo_library"}, {title: "Albums", icon: "photo_album"}, {title: "Search", icon: "search"}, {title: "Library", icon: "folder_special"}]
    Flickable { id: flick; anchors.fill: parent; contentHeight: col.height; clip: true
        Column { id: col; width: flick.width; spacing: 10
            Item { width: 1; height: 6 }
            RowLayout { x: 16; width: parent.width - 32
                MText { text: "Photos"; role: "headline"; Layout.fillWidth: true }
                MButton { icon: "cloud_done"; compact: true }
                MButton { icon: "more_vert"; compact: true } }
            Repeater { model: M.groups(F.photos).slice(0, 4)
                Column { required property var modelData; x: 4; width: col.width - 8; spacing: 6
                    MText { x: 12; text: modelData.t; role: "titleS" }
                    Justified { width: parent.width; photos: modelData.photos; rowHeight: 120; gap: 3; tileRadius: 8 } } } } }
}
