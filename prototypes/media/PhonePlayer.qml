import QtQuick
import QtQuick.Effects
import QtQuick.Layouts
import Opal.Material
import "Fixtures.js" as F
// The mini player grown into the full player: art, title, wavy seek, controls, output and the queue peeking from below.
Rectangle {
    color: Theme.base
    Rectangle { anchors.fill: parent; color: Theme.mix(Theme.base, Theme.primaryContainer, .35) }
    ColumnLayout {
        anchors.fill: parent; anchors.margins: 20; anchors.topMargin: 12; spacing: 14
        RowLayout { Layout.fillWidth: true
            MButton { icon: "keyboard_arrow_down"; compact: true }
            MText { Layout.fillWidth: true; horizontalAlignment: Text.AlignHCenter; text: "DELTARUNE Chapter 1"; role: "label"; color: Theme.subtext }
            MButton { icon: "more_vert"; compact: true } }
        Cover { Layout.fillWidth: true; Layout.preferredHeight: width; r: 36; source: F.now.img }
        RowLayout { Layout.fillWidth: true; Layout.topMargin: 8
            ColumnLayout { Layout.fillWidth: true; spacing: 2
                MText { Layout.fillWidth: true; text: F.now.t; role: "headline" }
                MText { text: F.now.a; role: "body"; color: Theme.subtext } }
            Rectangle { implicitWidth: 48; implicitHeight: 48; radius: 24; color: Theme.secondaryContainer
                Glyph { anchors.centerIn: parent; name: "favorite"; fill: 1; color: Theme.onSecondaryContainer } } }
        Transport { Layout.fillWidth: true; now: F.now; size: 76 }
        RowLayout { Layout.fillWidth: true; Layout.topMargin: 4
            Chip { text: "This phone"; icon: "smartphone"; trailingIcon: "expand_more"; glass: false }
            Item { Layout.fillWidth: true }
            MButton { icon: "lyrics"; compact: true }
            MButton { icon: "timer"; compact: true } }
        Item { Layout.fillHeight: true }
    }
    // Queue sheet peeking from the bottom.
    Rectangle { anchors.left: parent.left; anchors.right: parent.right; anchors.bottom: parent.bottom; height: 66; topLeftRadius: 28; topRightRadius: 28; color: Theme.surfaceLow
        Rectangle { anchors.horizontalCenter: parent.horizontalCenter; y: 10; width: 32; height: 4; radius: 2; color: Theme.alpha(Theme.subtext, .5) }
        RowLayout { anchors.fill: parent; anchors.topMargin: 18; anchors.leftMargin: 20; anchors.rightMargin: 20
            MText { text: "Up next"; role: "titleS"; Layout.fillWidth: true }
            MText { text: "Fanfare · Lantern · I'm Very Bad"; role: "labelS"; color: Theme.subtext } } }
}
