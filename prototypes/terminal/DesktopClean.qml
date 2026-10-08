import QtQuick
import QtQuick.Layouts
import Opal.Material
import "Fixtures.js" as F
// Terminal direction A, "Clean": slim pill tabs, tiled panes with gaps, shell-integration marks and a pill for the hovered command.
Rectangle {
    color: Theme.base
    component Tab: Rectangle {
        property string label; property string icon: "terminal"; property bool cur: false; property string status: ""
        implicitHeight: 36; implicitWidth: r.implicitWidth + 26; radius: 18
        color: cur ? Theme.surfaceHigh : "transparent"
        RowLayout { id: r; anchors.centerIn: parent; spacing: 8
            Glyph { name: icon; font.pixelSize: 17; color: cur ? Theme.primary : Theme.subtext }
            MText { text: label; role: "label"; font.pixelSize: 13; font.weight: cur ? Font.DemiBold : Font.Medium; color: cur ? Theme.text : Theme.subtext }
            Rectangle { visible: status !== ""; implicitWidth: 8; implicitHeight: 8; radius: 4; color: status === "fail" ? Theme.error : status === "run" ? Theme.tertiary : Theme.primary } }
    }
    ColumnLayout {
        anchors.fill: parent; anchors.margins: 8; spacing: 8
        RowLayout { Layout.fillWidth: true; spacing: 4
            Tab { label: "opal-apps"; cur: true; status: "fail" }
            Tab { label: "nas.local"; icon: "dns" }
            Tab { label: "fastfetch"; icon: "auto_awesome" }
            MButton { icon: "add"; compact: true }
            Item { Layout.fillWidth: true }
            MButton { icon: "vertical_split"; compact: true; tooltip: "Split right" }
            MButton { icon: "horizontal_split"; compact: true; tooltip: "Split down" }
            MButton { icon: "search"; compact: true }
            MButton { icon: "palette"; compact: true; tooltip: "Appearance" }
            MButton { icon: "more_vert"; compact: true } }
        RowLayout {
            Layout.fillWidth: true; Layout.fillHeight: true; spacing: 8
            Item {
                Layout.fillWidth: true; Layout.preferredWidth: 6; Layout.fillHeight: true
                TermView { id: main; anchors.fill: parent; lines: F.build; marks: ({1: "ok", 6: "ok", 11: "fail", 17: "run"}) }
                Rectangle { anchors.fill: parent; radius: 20; color: "transparent"; border.width: 2; border.color: Theme.alpha(Theme.primary, .5) }
                CommandPill { anchors.right: parent.right; anchors.rightMargin: 16; y: 11 * main.lineHeight + main.padding - 4; command: "ctest --test-dir build"; detail: "exit 8 · 1.4 s · 1 failed" }
                // Pane title chip: cwd and running program, from OSC 7 and the foreground process.
                Rectangle { anchors.right: parent.right; anchors.top: parent.top; anchors.margins: 10; implicitHeight: 28; implicitWidth: pr.implicitWidth + 20; radius: 14; color: Theme.surfaceHigh
                    RowLayout { id: pr; anchors.centerIn: parent; spacing: 6
                        Glyph { name: "edit_note"; font.pixelSize: 15; color: Theme.tertiary }
                        MText { text: "nvim · posture.cpp"; role: "labelS"; color: Theme.text } } }
            }
            ColumnLayout {
                Layout.fillWidth: true; Layout.preferredWidth: 5; Layout.fillHeight: true; spacing: 8
                TermView { Layout.fillWidth: true; Layout.preferredHeight: 3; Layout.fillHeight: true; lines: F.ssh; active: false; fontSize: 13 }
                TermView { Layout.fillWidth: true; Layout.preferredHeight: 5; Layout.fillHeight: true; lines: F.fastfetch(); active: false; fontSize: 11; cursor: false }
            }
        }
    }
}
