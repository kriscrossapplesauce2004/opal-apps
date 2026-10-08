import QtQuick
import QtQuick.Layouts
import Opal.Material
import "Fixtures.js" as F
// Terminal on a phone, pinched out: desktop splits become a pager of pane cards, tabs become chips. Tap a card to go back in.
Rectangle {
    color: Theme.base
    ColumnLayout {
        anchors.fill: parent; spacing: 14
        RowLayout { Layout.fillWidth: true; Layout.topMargin: 14; Layout.leftMargin: 16; Layout.rightMargin: 8
            MText { text: "Sessions"; role: "headline"; Layout.fillWidth: true }
            MButton { icon: "search"; compact: true } }
        Flickable { Layout.fillWidth: true; Layout.preferredHeight: 40; contentWidth: chips.width + 32; clip: true
            Row { id: chips; x: 16; spacing: 8
                Chip { text: "opal-apps · 2"; icon: "terminal"; selected: true }
                Chip { text: "nas.local"; icon: "dns"; glass: false }
                Chip { text: "cargo watch"; icon: "autorenew"; glass: false }
                Chip { text: "New"; icon: "add"; glass: false } } }
        // Pane cards: the focused pane is big, its neighbour peeks from the side.
        Item {
            Layout.fillWidth: true; Layout.fillHeight: true
            Rectangle { x: 28; y: 0; width: parent.width - 80; height: parent.height - 120; radius: 30; color: Theme.surface; border.width: 2; border.color: Theme.primary
                TermView { anchors.fill: parent; anchors.margins: 6; radius: 24; lines: F.build.slice(6, 17); fontSize: 10; padding: 10; cursor: false }
                Rectangle { anchors.horizontalCenter: parent.horizontalCenter; anchors.bottom: parent.bottom; anchors.bottomMargin: -16; implicitHeight: 32; implicitWidth: lr.implicitWidth + 24; radius: 16; color: Theme.primary
                    RowLayout { id: lr; anchors.centerIn: parent; spacing: 6
                        Glyph { name: "edit_note"; font.pixelSize: 16; color: Theme.onPrimary }
                        MText { text: "nvim · posture.cpp"; role: "label"; color: Theme.onPrimary } } } }
            Rectangle { x: parent.width - 40; y: 30; width: parent.width - 80; height: parent.height - 180; radius: 30; color: Theme.surface; opacity: .7
                TermView { anchors.fill: parent; anchors.margins: 6; radius: 24; lines: F.ssh; fontSize: 10; padding: 10; active: false } }
            Row { anchors.horizontalCenter: parent.horizontalCenter; anchors.bottom: parent.bottom; anchors.bottomMargin: 66; spacing: 6
                Rectangle { width: 20; height: 6; radius: 3; color: Theme.primary } Rectangle { width: 6; height: 6; radius: 3; color: Theme.alpha(Theme.subtext, .5) } }
            RowLayout { anchors.horizontalCenter: parent.horizontalCenter; anchors.bottom: parent.bottom; anchors.bottomMargin: 8; spacing: 10
                MButton { text: "Split"; icon: "splitscreen_add"; tonal: true }
                MButton { text: "Close pane"; icon: "close"; contained: true } }
        }
    }
}
