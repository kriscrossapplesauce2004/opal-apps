import QtQuick
import QtQuick.Layouts
import Opal.Material
import "Fixtures.js" as F
import "Model.js" as M
// Photos direction A, "Timeline": justified rows grouped by day, a month scrubber on the right, search on top.
Rectangle {
    color: Theme.base
    RowLayout {
        anchors.fill: parent; spacing: 0
        NavRail { Layout.fillHeight: true; Layout.preferredWidth: 220; expanded: true; currentIndex: 0; fabIcon: "add_photo_alternate"; fabText: "Import"
            destinations: [{title: "Photos", icon: "photo_library"}, {title: "Albums", icon: "photo_album"}, {title: "Screenshots", icon: "screenshot_monitor", badge: ""}, {title: "Favourites", icon: "favorite"}, {title: "Trash", icon: "delete"}] }
        ColumnLayout {
            Layout.fillWidth: true; Layout.fillHeight: true; Layout.topMargin: 14; Layout.rightMargin: 12; spacing: 12
            RowLayout { Layout.fillWidth: true; spacing: 12
                SearchField { Layout.preferredWidth: 440; placeholder: "Search photos, places, text in screenshots" }
                Item { Layout.fillWidth: true }
                SegmentedButtons { current: "day"; options: [{id: "year", name: "Years"}, {id: "month", name: "Months"}, {id: "day", name: "Days"}] }
                MButton { icon: "slideshow"; compact: true } }
            Item {
                Layout.fillWidth: true; Layout.fillHeight: true; clip: true
                Flickable { id: flick; anchors.fill: parent; anchors.rightMargin: 56; contentHeight: col.height
                    Column { id: col; width: flick.width; spacing: 10
                        Repeater { model: M.groups(F.photos).slice(0, 5)
                            Column { required property var modelData; width: col.width; spacing: 8
                                RowLayout { width: parent.width
                                    MText { text: modelData.t; role: "title" }
                                    MText { text: modelData.photos.length + (modelData.photos.length === 1 ? " photo" : " photos"); role: "label"; color: Theme.subtext; Layout.leftMargin: 8 }
                                    Item { Layout.fillWidth: true }
                                    Glyph { name: "check_circle"; color: Theme.subtext; font.pixelSize: 20 } }
                                Justified { width: parent.width; photos: modelData.photos; rowHeight: 190 } } } } }
                // Month scrubber.
                Column { anchors.right: parent.right; anchors.top: parent.top; anchors.bottom: parent.bottom; width: 48; spacing: 0
                    Repeater { model: ["Oct", "Sep", "Aug", "Jul", "Jun", "May", "2025"]
                        Item { required property string modelData; required property int index; width: 48; height: 46
                            Rectangle { anchors.centerIn: parent; width: index === 0 ? 44 : 6; height: index === 0 ? 26 : 6; radius: height / 2; color: index === 0 ? Theme.primary : Theme.alpha(Theme.subtext, .4)
                                MText { anchors.centerIn: parent; visible: index === 0; text: modelData; role: "labelS"; font.weight: Font.Bold; color: Theme.onPrimary } }
                            MText { visible: index > 0 && index % 2 === 0; anchors.right: parent.right; anchors.verticalCenter: parent.verticalCenter; anchors.rightMargin: 30; text: modelData; role: "labelS"; color: Theme.subtext } } } }
            }
        }
    }
}
