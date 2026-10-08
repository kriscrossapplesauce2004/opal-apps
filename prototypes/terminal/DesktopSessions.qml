import QtQuick
import QtQuick.Layouts
import Opal.Material
import "Fixtures.js" as F
// Terminal direction C, "Sessions": persistent sessions in a side rail, and a command palette with snippets and SSH hosts.
Rectangle {
    color: Theme.base
    component Session: Rectangle {
        property string title; property string detail; property string icon: "terminal"; property bool cur: false; property string state_: ""
        Layout.fillWidth: true; implicitHeight: 58; radius: 20
        color: cur ? Theme.secondaryContainer : "transparent"
        RowLayout { anchors.fill: parent; anchors.leftMargin: 12; anchors.rightMargin: 12; spacing: 12
            Rectangle { implicitWidth: 34; implicitHeight: 34; radius: 12; color: Theme.alpha(cur ? Theme.primary : Theme.subtext, .16)
                Glyph { anchors.centerIn: parent; name: icon; font.pixelSize: 19; color: cur ? Theme.primary : Theme.subtext } }
            ColumnLayout { Layout.fillWidth: true; spacing: 0
                MText { Layout.fillWidth: true; text: title; role: "titleS"; font.pixelSize: 14; color: cur ? Theme.onSecondaryContainer : Theme.text }
                MText { Layout.fillWidth: true; text: detail; role: "labelS"; font.family: "CaskaydiaCove Nerd Font Mono"; color: Theme.subtext } }
            Rectangle { visible: state_ !== ""; implicitWidth: 8; implicitHeight: 8; radius: 4; color: state_ === "fail" ? Theme.error : state_ === "run" ? Theme.tertiary : Theme.primary } }
    }
    RowLayout {
        anchors.fill: parent; spacing: 0
        ColumnLayout {
            Layout.preferredWidth: 270; Layout.maximumWidth: 270; Layout.fillHeight: true; Layout.margins: 12; spacing: 4
            Fab { size: "small"; icon: "add"; text: "New session"; Layout.bottomMargin: 12 }
            MText { text: "Running"; role: "label"; color: Theme.subtext; Layout.leftMargin: 12 }
            Session { title: "opal-apps"; detail: "nvim posture.cpp"; cur: true; state_: "fail" }
            Session { title: "cargo watch"; detail: "jevcomputeruse · 3 m"; icon: "autorenew"; state_: "run" }
            Session { title: "nas.local"; detail: "ssh · zfs"; icon: "dns" }
            MText { text: "Saved"; role: "label"; color: Theme.subtext; Layout.leftMargin: 12; Layout.topMargin: 12 }
            Session { title: "achroot device"; detail: "adb shell"; icon: "smartphone" }
            Session { title: "pmbootstrap"; detail: "~/pmos · build"; icon: "build" }
            Item { Layout.fillHeight: true }
            Rectangle { Layout.fillWidth: true; implicitHeight: 64; radius: 20; color: Theme.surface
                RowLayout { anchors.fill: parent; anchors.margins: 12; spacing: 10
                    Glyph { name: "restore"; color: Theme.primary }
                    MText { Layout.fillWidth: true; text: "Sessions survive closing the window"; role: "labelS"; wrapMode: Text.Wrap; color: Theme.subtext } } }
        }
        Item {
            Layout.fillWidth: true; Layout.fillHeight: true
            TermView { anchors.fill: parent; anchors.margins: 12; anchors.leftMargin: 0; lines: F.build; marks: ({1: "ok", 6: "ok", 11: "fail", 17: "run"}) }
            Rectangle { anchors.fill: parent; anchors.margins: 12; anchors.leftMargin: 0; radius: 20; color: Theme.alpha("#000000", .35) }
            // Command palette: snippets, hosts and actions in one search.
            Rectangle {
                anchors.horizontalCenter: parent.horizontalCenter; y: 90
                width: 620; height: 470; radius: 28; color: Theme.surfaceHigh
                ColumnLayout {
                    anchors.fill: parent; anchors.margins: 12; spacing: 6
                    SearchField { Layout.fillWidth: true; leadingIcon: "terminal"; placeholder: "Run a snippet, open a host, or type a command"; text: "bu"; color: Theme.surfaceHighest }
                    MText { text: "Snippets"; role: "label"; color: Theme.primary; Layout.leftMargin: 14; Layout.topMargin: 6 }
                    ListRow { Layout.fillWidth: true; icon: "build"; iconBadge: true; iconTint: Theme.primary; title: "Build and test"; subtitle: "cmake --build build && ctest --test-dir build"; selected: true; trailing: "↵" }
                    ListRow { Layout.fillWidth: true; icon: "deployed_code"; iconBadge: true; iconTint: Theme.secondary; title: "Build pmOS image"; subtitle: "pmbootstrap install --android-fastboot-img" }
                    MText { text: "Hosts"; role: "label"; color: Theme.primary; Layout.leftMargin: 14; Layout.topMargin: 6 }
                    ListRow { Layout.fillWidth: true; icon: "dns"; iconBadge: true; iconTint: Theme.tertiary; title: "build-box"; subtitle: "ssh melissa@build-box.local" }
                    MText { text: "Actions"; role: "label"; color: Theme.primary; Layout.leftMargin: 14; Layout.topMargin: 6 }
                    ListRow { Layout.fillWidth: true; icon: "vertical_split"; title: "Split right"; trailing: "Ctrl Shift D" }
                    Item { Layout.fillHeight: true }
                }
            }
        }
    }
}
