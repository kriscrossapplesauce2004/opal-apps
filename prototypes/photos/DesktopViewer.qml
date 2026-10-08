import QtQuick
import QtQuick.Layouts
import Opal.Material
import "Fixtures.js" as F
// Photos direction C, "Viewer": the photo on a dark stage, a floating action toolbar, a filmstrip, and the info panel.
Rectangle {
    color: "#0b0b0a"
    readonly property var cur: F.photos.filter(p => p.n.indexOf("dr5-keyart") === 0)[0] || F.photos[0]
    RowLayout {
        anchors.fill: parent; spacing: 0
        Item {
            Layout.fillWidth: true; Layout.fillHeight: true
            RowLayout { anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top; anchors.margins: 14
                Rectangle { implicitWidth: 44; implicitHeight: 44; radius: 22; color: Qt.rgba(1, 1, 1, .1); Glyph { anchors.centerIn: parent; name: "arrow_back"; color: "#ffffff" } }
                MText { text: parent.parent.parent.parent.cur.n; role: "titleS"; color: "#ffffff"; Layout.leftMargin: 8 }
                Item { Layout.fillWidth: true }
                MText { text: "4 of 47"; role: "label"; color: Qt.rgba(1, 1, 1, .7) } }
            RoundedImage { id: photo; anchors.centerIn: parent; anchors.verticalCenterOffset: -30; width: Math.min(parent.width - 120, (parent.height - 230) * cur.w / cur.h); height: width * cur.h / cur.w; radius: 16; source: cur.u; sourceSize: Qt.size(1600, 900)
                readonly property var cur: parent.parent.parent.cur }
            // Floating toolbar.
            Rectangle { anchors.horizontalCenter: parent.horizontalCenter; anchors.bottom: strip.top; anchors.bottomMargin: 18; implicitHeight: 60; implicitWidth: tb.implicitWidth + 16; radius: 30; color: Theme.surfaceHigh
                RowLayout { id: tb; anchors.centerIn: parent; spacing: 2
                    Repeater { model: [{i: "favorite", on: true}, {i: "edit"}, {i: "crop_rotate"}, {i: "share"}, {i: "wallpaper"}, {i: "info", on: true}, {i: "delete"}]
                        Rectangle { required property var modelData; implicitWidth: 48; implicitHeight: 44; radius: 22; color: modelData.on ? Theme.secondaryContainer : "transparent"
                            Glyph { anchors.centerIn: parent; name: modelData.i; fill: modelData.on ? 1 : 0; color: modelData.on ? Theme.onSecondaryContainer : Theme.text } } } } }
            // Filmstrip.
            Row { id: strip; anchors.horizontalCenter: parent.horizontalCenter; anchors.bottom: parent.bottom; anchors.bottomMargin: 16; spacing: 6
                Repeater { model: F.photos.slice(0, 11)
                    RoundedImage { required property var modelData; required property int index; width: index === 3 ? 84 : 54; height: 54; radius: 10; source: modelData.u; sourceSize: Qt.size(170, 110); opacity: index === 3 ? 1 : .6 } } }
        }
        // Info panel.
        Rectangle {
            Layout.preferredWidth: 340; Layout.fillHeight: true; color: Theme.surfaceLow
            ColumnLayout { anchors.fill: parent; anchors.margins: 20; spacing: 14
                RowLayout { MText { text: "Details"; role: "title"; Layout.fillWidth: true } MButton { icon: "close"; compact: true } }
                Rectangle { Layout.fillWidth: true; implicitHeight: 54; radius: 18; color: Theme.surfaceHigh
                    MText { anchors.fill: parent; anchors.margins: 14; verticalAlignment: Text.AlignVCenter; text: "Add a caption"; color: Theme.subtext } }
                Repeater { model: [["calendar_today", "8 Oct 2026, 13:58", "Wednesday"], ["image", cur.n, cur.w + " × " + cur.h + " · " + (cur.s / 1048576).toFixed(1) + " MB"], ["folder", "Wallpapers", "~/Pictures/Wallpapers"], ["auto_awesome", "Dominant colours", ""]]
                    RowLayout { required property var modelData; Layout.fillWidth: true; spacing: 14
                        Glyph { name: modelData[0]; color: Theme.subtext; Layout.alignment: Qt.AlignTop }
                        ColumnLayout { Layout.fillWidth: true; spacing: 1
                            MText { Layout.fillWidth: true; text: modelData[1]; role: "titleS"; font.pixelSize: 14 }
                            MText { visible: modelData[2] !== ""; Layout.fillWidth: true; text: modelData[2]; role: "labelS"; color: Theme.subtext } } } }
                Row { spacing: 8; Layout.leftMargin: 38
                    Repeater { model: ["#f2a65a", "#d9606e", "#6b3f5e", "#2d2440", "#f5d6a1"]; Rectangle { required property string modelData; width: 30; height: 30; radius: 15; color: modelData } } }
                MText { text: "Albums"; role: "label"; color: Theme.primary; Layout.topMargin: 6 }
                Flow { Layout.fillWidth: true; spacing: 6
                    Chip { text: "Wallpapers"; icon: "photo_album"; glass: false }
                    Chip { text: "Favourites"; icon: "favorite"; selected: true }
                    Chip { text: "Add"; icon: "add"; glass: false } }
                Item { Layout.fillHeight: true }
                MButton { text: "Show in Files"; icon: "folder_open"; tonal: true; Layout.fillWidth: true } }
        }
    }
}
