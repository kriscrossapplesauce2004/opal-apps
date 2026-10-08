import QtQuick
import QtQuick.Layouts
import Opal.Material
import "Fixtures.js" as F
// Text Editor direction B, "Folder": open a folder, a file tree on the left, find-in-files results, a find bar over the editor.
Rectangle {
    color: Theme.base
    component Node: Rectangle { property string name; property int depth: 0; property bool dir: false; property bool open: false; property bool cur: false; property string mark: ""
        Layout.fillWidth: true; implicitHeight: 32; radius: 16; color: cur ? Theme.secondaryContainer : "transparent"
        RowLayout { anchors.fill: parent; anchors.leftMargin: 10 + depth * 16; anchors.rightMargin: 10; spacing: 6
            Glyph { visible: dir; name: open ? "expand_more" : "chevron_right"; font.pixelSize: 16; color: Theme.subtext }
            Glyph { name: dir ? (open ? "folder_open" : "folder") : name.endsWith(".md") ? "description" : name.endsWith(".qml") ? "deployed_code" : "code"; fill: dir ? 1 : 0; font.pixelSize: 17; color: dir ? Theme.primary : Theme.subtext }
            MText { Layout.fillWidth: true; text: name; role: "label"; font.pixelSize: 13; color: cur ? Theme.onSecondaryContainer : Theme.text }
            MText { visible: mark !== ""; text: mark; role: "labelS"; font.weight: Font.Bold; color: mark === "M" ? Theme.tertiary : Theme.primary } } }
    RowLayout {
        anchors.fill: parent; anchors.margins: 8; spacing: 8
        Rectangle { Layout.preferredWidth: 270; Layout.fillHeight: true; radius: 24; color: Theme.surfaceLow
            ColumnLayout { anchors.fill: parent; anchors.margins: 10; spacing: 1
                RowLayout { Layout.fillWidth: true; Layout.bottomMargin: 6
                    MText { text: "opal-apps"; role: "titleS"; Layout.fillWidth: true; Layout.leftMargin: 8 }
                    MButton { icon: "note_add"; compact: true }
                    MButton { icon: "search"; compact: true; tonal: true } }
                Node { name: "docs"; dir: true; open: true }
                Node { name: "SPEC.md"; depth: 1 }
                Node { name: "files-design.md"; depth: 1 }
                Node { name: "media-design.md"; depth: 1; mark: "M" }
                Node { name: "material"; dir: true; open: true }
                Node { name: "qml"; dir: true; depth: 1 }
                Node { name: "src"; dir: true; depth: 1; open: true }
                Node { name: "gamepad.cpp"; depth: 2 }
                Node { name: "palette.cpp"; depth: 2; cur: true; mark: "M" }
                Node { name: "posture.cpp"; depth: 2 }
                Node { name: "tonal.cpp"; depth: 2; mark: "M" }
                Node { name: "prototypes"; dir: true }
                Node { name: "tools"; dir: true }
                Item { Layout.fillHeight: true }
                // Find in files.
                Rectangle { Layout.fillWidth: true; implicitHeight: 170; radius: 18; color: Theme.surfaceHigh
                    ColumnLayout { anchors.fill: parent; anchors.margins: 12; spacing: 4
                        MText { text: "schemeFor · 6 results in 3 files"; role: "label"; color: Theme.primary }
                        Repeater { model: [["palette.cpp", "QVariantMap Palette::schemeFor(…)"], ["palette.h", "Q_INVOKABLE QVariantMap schemeFor"], ["PlayerBleed3.qml", "Palette.schemeFor(seed, Theme.light, .5)"]]
                            ColumnLayout { required property var modelData; spacing: 0; Layout.fillWidth: true
                                MText { text: modelData[0]; role: "labelS"; font.weight: Font.Bold }
                                MText { Layout.fillWidth: true; text: modelData[1]; role: "labelS"; font.family: "CaskaydiaCove Nerd Font Mono"; color: Theme.subtext } } } } } } }
        ColumnLayout { Layout.fillWidth: true; Layout.fillHeight: true; spacing: 8
            Item { Layout.fillWidth: true; Layout.fillHeight: true
                CodeView { anchors.fill: parent; lines: F.cpp; lang: "cpp"; firstLine: 61; currentLine: 101; changed: [100, 101, 102] }
                // Find and replace bar floats at the top right of the editor.
                Rectangle { anchors.right: parent.right; anchors.top: parent.top; anchors.margins: 12; width: 420; height: 104; radius: 22; color: Theme.surfaceHigh
                    ColumnLayout { anchors.fill: parent; anchors.margins: 10; spacing: 6
                        RowLayout { Layout.fillWidth: true; spacing: 6
                            SearchField { Layout.fillWidth: true; implicitHeight: 38; text: "schemeFor"; color: Theme.surfaceHighest }
                            MText { text: "2 of 2"; role: "labelS"; color: Theme.subtext }
                            MButton { icon: "keyboard_arrow_up"; compact: true } MButton { icon: "keyboard_arrow_down"; compact: true } }
                        RowLayout { Layout.fillWidth: true; spacing: 6
                            SearchField { Layout.fillWidth: true; implicitHeight: 38; leadingIcon: "find_replace"; placeholder: "Replace"; color: Theme.surfaceHighest }
                            Chip { text: "Aa"; glass: false } Chip { text: ".*"; selected: true } } } } }
            StatusChips { lang: "C++"; pos: "Ln 101, Col 22" } }
    }
}
