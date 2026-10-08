import QtQuick
import QtQuick.Layouts
import Opal.Material
import Opal.Terminal
import "Fixtures.js" as F
// Text Editor round 2, Code mode with IDE features: symbol breadcrumbs, LSP completion, a diagnostic with hover, git gutter, and a bottom panel whose Terminal tab is the real Opal Terminal engine.
Rectangle {
    color: Theme.base
    component Node: Rectangle { property string name; property int depth: 0; property bool dir: false; property bool open: false; property bool cur: false; property string mark: ""
        Layout.fillWidth: true; implicitHeight: 30; radius: 15; color: cur ? Theme.secondaryContainer : "transparent"
        RowLayout { anchors.fill: parent; anchors.leftMargin: 10 + depth * 16; anchors.rightMargin: 10; spacing: 6
            Glyph { visible: dir; name: open ? "expand_more" : "chevron_right"; font.pixelSize: 16; color: Theme.subtext }
            Glyph { name: dir ? (open ? "folder_open" : "folder") : "code"; fill: dir ? 1 : 0; font.pixelSize: 16; color: dir ? Theme.primary : Theme.subtext }
            MText { Layout.fillWidth: true; text: name; role: "label"; font.pixelSize: 13; color: cur ? Theme.onSecondaryContainer : Theme.text }
            MText { visible: mark !== ""; text: mark; role: "labelS"; font.weight: Font.Bold; color: mark === "M" ? Theme.tertiary : mark === "2" ? Theme.error : Theme.primary } } }
    ColumnLayout {
        anchors.fill: parent; anchors.margins: 8; spacing: 8
        RowLayout { Layout.fillWidth: true; spacing: 6
            SegmentedButtons { current: "code"; options: [{id: "write", name: "Write", icon: "edit_note"}, {id: "code", name: "Code", icon: "code"}] }
            Rectangle { implicitHeight: 36; implicitWidth: tr.implicitWidth + 26; radius: 18; color: Theme.surfaceHigh; Layout.leftMargin: 8
                RowLayout { id: tr; anchors.centerIn: parent; spacing: 8; Glyph { name: "code"; font.pixelSize: 17; color: Theme.primary } MText { text: "palette.cpp"; role: "label"; font.weight: Font.DemiBold } Rectangle { implicitWidth: 8; implicitHeight: 8; radius: 4; color: Theme.primary } } }
            Repeater { model: ["palette.h", "posture.cpp"]
                Rectangle { required property string modelData; implicitHeight: 36; implicitWidth: t2.implicitWidth + 26; radius: 18; color: "transparent"
                    RowLayout { id: t2; anchors.centerIn: parent; spacing: 8; Glyph { name: "code"; font.pixelSize: 17; color: Theme.subtext } MText { text: modelData; role: "label"; color: Theme.subtext } } } }
            Item { Layout.fillWidth: true }
            Rectangle { implicitHeight: 36; implicitWidth: rr.implicitWidth + 24; radius: 18; color: Theme.primaryContainer
                RowLayout { id: rr; anchors.centerIn: parent; spacing: 6; Glyph { name: "play_arrow"; fill: 1; font.pixelSize: 18; color: Theme.onContainer } MText { text: "Build · cmake"; role: "label"; color: Theme.onContainer } Glyph { name: "expand_more"; font.pixelSize: 16; color: Theme.onContainer } } }
            MButton { icon: "terminal"; compact: true; tonal: true }
            MButton { icon: "more_vert"; compact: true } }
        RowLayout { Layout.fillWidth: true; Layout.fillHeight: true; spacing: 8
            Rectangle { Layout.preferredWidth: 250; Layout.maximumWidth: 250; Layout.fillHeight: true; radius: 24; color: Theme.surfaceLow
                ColumnLayout { anchors.fill: parent; anchors.margins: 10; spacing: 1
                    SegmentedButtons { Layout.alignment: Qt.AlignHCenter; Layout.bottomMargin: 8; current: "files"; options: [{id: "files", icon: "folder"}, {id: "sym", icon: "data_object"}, {id: "git", icon: "commit"}, {id: "find", icon: "search"}] }
                    Node { name: "material"; dir: true; open: true }
                    Node { name: "src"; dir: true; depth: 1; open: true }
                    Node { name: "gamepad.cpp"; depth: 2 }
                    Node { name: "palette.cpp"; depth: 2; cur: true; mark: "2" }
                    Node { name: "palette.h"; depth: 2; mark: "M" }
                    Node { name: "posture.cpp"; depth: 2 }
                    Node { name: "tonal.cpp"; depth: 2; mark: "M" }
                    Node { name: "qml"; dir: true; depth: 1 }
                    Node { name: "apps"; dir: true }
                    Node { name: "docs"; dir: true }
                    Node { name: "tests"; dir: true }
                    Item { Layout.fillHeight: true }
                    Rectangle { Layout.fillWidth: true; implicitHeight: 74; radius: 18; color: Theme.surfaceHigh
                        ColumnLayout { anchors.fill: parent; anchors.margins: 12; spacing: 4
                            RowLayout { Glyph { name: "commit"; font.pixelSize: 16; color: Theme.primary } MText { text: "main · 3 changed"; role: "label"; Layout.fillWidth: true } MText { text: "↑2"; role: "labelS"; color: Theme.subtext } }
                            MText { text: "Last: Media design locked"; role: "labelS"; color: Theme.subtext } } } } }
            ColumnLayout { Layout.fillWidth: true; Layout.fillHeight: true; spacing: 8
                Item { Layout.fillWidth: true; Layout.fillHeight: true; Layout.preferredHeight: 5
                    CodeView { id: code; anchors.fill: parent; lines: F.cpp.slice(18, 50); lang: "cpp"; firstLine: 79; currentLine: 95; changed: [94, 95, 96] }
                    // Symbol breadcrumbs.
                    Rectangle { anchors.right: parent.right; anchors.top: parent.top; anchors.margins: 10; implicitHeight: 30; implicitWidth: sb.implicitWidth + 20; radius: 15; color: Theme.surfaceHigh
                        RowLayout { id: sb; anchors.centerIn: parent; spacing: 6; Glyph { name: "data_object"; font.pixelSize: 15; color: Theme.tertiary } MText { text: "Palette  ›  setSeed(const QColor &)"; role: "labelS" } } }
                    // Diagnostic squiggle under a token on line 95 and its hover card.
                    Canvas { x: 64 + 9 * 8.4; y: 14 + (95 - 79) * code.lh + code.lh - 5; width: 120; height: 6
                        onPaint: { const g = getContext("2d"); g.strokeStyle = Theme.error; g.lineWidth = 1.6; g.beginPath(); for (let x = 0; x <= width; x += 1) g.lineTo(x, 3 + Math.sin(x / 2) * 2); g.stroke(); } }
                    Rectangle { x: 120; y: 14 + (96 - 79) * code.lh + 4; width: 470; height: 112; radius: 18; color: Theme.surfaceHighest; border.width: 1; border.color: Theme.outline
                        ColumnLayout { anchors.fill: parent; anchors.margins: 14; spacing: 6
                            RowLayout { Glyph { name: "error"; fill: 1; font.pixelSize: 18; color: Theme.error } MText { text: "clangd · comparison of QColor with QByteArray"; role: "label"; font.weight: Font.DemiBold; Layout.fillWidth: true } }
                            MText { Layout.fillWidth: true; wrapMode: Text.Wrap; text: "No viable operator== between 'const QColor' and 'QByteArray'. Did you mean qgetenv(\"OPAL_SEED\").isEmpty()?"; role: "bodyS"; color: Theme.subtext }
                            RowLayout { spacing: 6; Chip { text: "Quick fix"; icon: "build"; selected: true } Chip { text: "Go to definition"; icon: "arrow_outward"; glass: false } } } }
                    // Completion popup at the cursor.
                    Rectangle { x: 700; y: 14 + (102 - 79) * code.lh + code.lh; width: 330; height: 196; radius: 18; color: Theme.surfaceHighest; border.width: 1; border.color: Theme.outline; z: 5
                        ColumnLayout { anchors.fill: parent; anchors.margins: 6; spacing: 0
                            Repeater { model: [{k: "f", t: "schemeFor(QColor, bool, qreal)", d: "QVariantMap", s: true}, {k: "f", t: "setSeed(QColor)", d: "void"}, {k: "p", t: "seed", d: "QColor"}, {k: "f", t: "source()", d: "QString"}, {k: "p", t: "roles", d: "QVariantMap"}]
                                Rectangle { required property var modelData; Layout.fillWidth: true; implicitHeight: 36; radius: 12; color: modelData.s ? Theme.secondaryContainer : "transparent"
                                    RowLayout { anchors.fill: parent; anchors.leftMargin: 10; anchors.rightMargin: 10; spacing: 10
                                        Rectangle { implicitWidth: 22; implicitHeight: 22; radius: 7; color: Theme.alpha(modelData.k === "f" ? Theme.primary : Theme.tertiary, .2)
                                            MText { anchors.centerIn: parent; text: modelData.k; font.pixelSize: 12; font.weight: Font.Bold; color: modelData.k === "f" ? Theme.primary : Theme.tertiary } }
                                        MText { Layout.fillWidth: true; text: modelData.t; font.family: "CaskaydiaCove Nerd Font Mono"; font.pixelSize: 12; color: modelData.s ? Theme.onSecondaryContainer : Theme.text }
                                        MText { text: modelData.d; role: "labelS"; color: Theme.subtext } } } } } }
                }
                // Bottom panel: Terminal (real engine), Problems, Git, Tasks.
                Rectangle { Layout.fillWidth: true; Layout.fillHeight: true; Layout.preferredHeight: 2.2; radius: 24; color: Theme.surfaceLowest
                    ColumnLayout { anchors.fill: parent; anchors.margins: 8; spacing: 4
                        RowLayout { Layout.fillWidth: true; spacing: 4
                            Repeater { model: [{t: "Terminal", i: "terminal", on: true}, {t: "Problems · 2", i: "error"}, {t: "Git", i: "commit"}, {t: "Tasks", i: "checklist"}]
                                Rectangle { required property var modelData; implicitHeight: 30; implicitWidth: pr.implicitWidth + 22; radius: 15; color: modelData.on ? Theme.secondaryContainer : "transparent"
                                    RowLayout { id: pr; anchors.centerIn: parent; spacing: 6; Glyph { name: modelData.i; font.pixelSize: 15; color: modelData.on ? Theme.onSecondaryContainer : Theme.subtext } MText { text: modelData.t; role: "labelS"; color: modelData.on ? Theme.onSecondaryContainer : Theme.subtext } } } }
                            Item { Layout.fillWidth: true }
                            Glyph { name: "add"; font.pixelSize: 18; color: Theme.subtext } Glyph { name: "expand_less"; font.pixelSize: 18; color: Theme.subtext } }
                        TerminalSession { id: sess; program: "/bin/sh"
                            arguments: ["-c", "printf '\\033[1;32m❯\\033[0m cmake --build build\\n[1/4] Automatic MOC for target OpalMaterial\\n[2/4] Building CXX object material/.../palette.cpp.o\\n\\033[1;31mpalette.cpp:95:43: error:\\033[0m no match for operator== (QColor and QByteArray)\\n\\033[1;31mninja: build stopped: subcommand failed.\\033[0m\\n'; sleep 5"]
                            Component.onCompleted: start() }
                        TerminalItem { Layout.fillWidth: true; Layout.fillHeight: true; session: sess; drawBackground: false; fontFamily: "CaskaydiaCove Nerd Font Mono"; fontPixelSize: 12 } } }
                RowLayout { spacing: 6
                    Rectangle { implicitHeight: 28; implicitWidth: ls.implicitWidth + 20; radius: 14; color: Theme.surfaceHigh
                        RowLayout { id: ls; anchors.centerIn: parent; spacing: 5; Rectangle { implicitWidth: 8; implicitHeight: 8; radius: 4; color: Theme.primary } MText { text: "clangd"; role: "labelS" } } }
                    Rectangle { implicitHeight: 28; implicitWidth: er.implicitWidth + 20; radius: 14; color: Theme.alpha(Theme.error, .18)
                        RowLayout { id: er; anchors.centerIn: parent; spacing: 5; Glyph { name: "error"; fill: 1; font.pixelSize: 14; color: Theme.error } MText { text: "2 errors"; role: "labelS"; color: Theme.error } } }
                    Item { Layout.fillWidth: true }
                    StatusChips { lang: "C++"; pos: "Ln 95, Col 43" } } }
        }
    }
}
