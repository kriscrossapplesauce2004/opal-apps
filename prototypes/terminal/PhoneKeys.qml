import QtQuick
import QtQuick.Layouts
import Opal.Material
import "Fixtures.js" as F
// Terminal on a phone: tab chip and pane dots on top, the session, the smart key row, then the keyboard.
Rectangle {
    color: Theme.base
    ColumnLayout {
        anchors.fill: parent; spacing: 0
        RowLayout { Layout.fillWidth: true; Layout.preferredHeight: 60; Layout.leftMargin: 8; Layout.rightMargin: 4; spacing: 6
            Rectangle { implicitHeight: 40; implicitWidth: tabRow.implicitWidth + 28; radius: 20; color: Theme.surfaceHigh
                RowLayout { id: tabRow; anchors.centerIn: parent; spacing: 8
                    Glyph { name: "terminal"; font.pixelSize: 18; color: Theme.primary }
                    MText { text: "opal-apps"; role: "titleS" }
                    Rectangle { implicitWidth: 22; implicitHeight: 18; radius: 9; color: Theme.secondaryContainer
                        MText { anchors.centerIn: parent; text: "3"; font.pixelSize: 11; font.weight: Font.Bold; color: Theme.onSecondaryContainer } }
                    Glyph { name: "expand_more"; font.pixelSize: 18; color: Theme.subtext } } }
            Item { Layout.fillWidth: true }
            // Pane dots: this tab has two panes; swipe sideways to change pane.
            Row { spacing: 5; Rectangle { width: 18; height: 6; radius: 3; color: Theme.primary } Rectangle { width: 6; height: 6; radius: 3; color: Theme.alpha(Theme.subtext, .5) } }
            MButton { icon: "add"; compact: true }
            MButton { icon: "more_vert"; compact: true } }
        TermView { Layout.fillWidth: true; Layout.fillHeight: true; Layout.leftMargin: 6; Layout.rightMargin: 6; radius: 22; lines: F.build.slice(6); fontSize: 12; padding: 12; marks: ({0: "ok", 5: "fail", 11: "run"}) }
        // Zoom feedback after a pinch.
        Item { Layout.fillWidth: true; Layout.preferredHeight: 0
            Rectangle { anchors.horizontalCenter: parent.horizontalCenter; y: -60; implicitHeight: 32; implicitWidth: zr.implicitWidth + 24; radius: 16; color: Theme.inverseSurface
                RowLayout { id: zr; anchors.centerIn: parent; spacing: 6
                    Glyph { name: "pinch_zoom_in"; font.pixelSize: 16; color: Theme.inverseInk }
                    MText { text: "12 pt"; role: "label"; color: Theme.inverseInk } } } }
        KeyRow { Layout.fillWidth: true; Layout.topMargin: 6 }
        KeyboardMock { Layout.fillWidth: true }
    }
}
