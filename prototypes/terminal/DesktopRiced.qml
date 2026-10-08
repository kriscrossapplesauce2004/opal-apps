import QtQuick
import QtQuick.Effects
import QtQuick.Layouts
import Opal.Material
import "Fixtures.js" as F
// Terminal direction B, "Riced": the same app pushed by a config. Wallpaper with blur behind translucent panes, a Catppuccin theme, Fantasque font, cursor trail, and the live Appearance panel.
Rectangle {
    id: root
    color: "#11111b"
    readonly property var mocha: ({bg: Qt.rgba(0.07, 0.07, 0.11, .72), fg: "#cdd6f4", dim: "#7f849c", red: "#f38ba8", green: "#a6e3a1", yellow: "#f9e2af", blue: "#89b4fa", magenta: "#cba6f7", cyan: "#94e2d5", cursor: "#f5c2e7"})
    Image { id: wall; anchors.fill: parent; source: "file:///home/melissa/Pictures/Wallpapers/3kg8qd.jpeg"; fillMode: Image.PreserveAspectCrop; visible: false; sourceSize: Qt.size(1440, 900) }
    MultiEffect { anchors.fill: parent; source: wall; blurEnabled: true; blur: .7; blurMax: 48; saturation: .1; brightness: -.15 }
    component Swatch: Rectangle {
        property string name; property var colors: []; property bool cur: false
        implicitWidth: 132; implicitHeight: 64; radius: 18
        color: colors[0]; border.width: cur ? 3 : 0; border.color: Theme.primary
        Row { anchors.left: parent.left; anchors.top: parent.top; anchors.margins: 10; spacing: 4
            Repeater { model: colors.slice(1); Rectangle { required property string modelData; width: 14; height: 14; radius: 7; color: modelData } } }
        Text { anchors.left: parent.left; anchors.bottom: parent.bottom; anchors.margins: 10; text: name; color: colors[1]; font.family: Theme.font; font.pixelSize: 12; font.weight: Font.DemiBold }
        Glyph { visible: cur; anchors.right: parent.right; anchors.top: parent.top; anchors.margins: 8; name: "check_circle"; fill: 1; font.pixelSize: 18; color: Theme.primary }
    }
    RowLayout {
        anchors.fill: parent; anchors.margins: 14; spacing: 14
        ColumnLayout {
            Layout.fillWidth: true; Layout.fillHeight: true; spacing: 10
            // Tabs as a floating powerline-style strip.
            RowLayout { spacing: 6
                Repeater {
                    model: [{t: "1  opal-apps", c: "#cba6f7", cur: true}, {t: "2  nas", c: "#89b4fa"}, {t: "3  btop", c: "#a6e3a1"}]
                    Rectangle { required property var modelData
                        implicitHeight: 30; implicitWidth: tt.implicitWidth + 28; radius: 15
                        color: modelData.cur ? modelData.c : Qt.rgba(0.07, 0.07, 0.11, .6)
                        Text { id: tt; anchors.centerIn: parent; text: modelData.t; font.family: "FantasqueSansM Nerd Font Mono"; font.pixelSize: 15; font.bold: !!modelData.cur; color: modelData.cur ? "#11111b" : modelData.c } } } }
            RowLayout {
                Layout.fillWidth: true; Layout.fillHeight: true; spacing: 12
                TermView { Layout.fillWidth: true; Layout.fillHeight: true; Layout.preferredWidth: 6; theme: root.mocha; lines: F.build; fontFamily: "FantasqueSansM Nerd Font Mono"; fontSize: 16; padding: 22; radius: 26; trail: true; cursorStyle: "block"
                    // CRT shader preview: scanlines and a soft vignette.
                    Column { anchors.fill: parent; Repeater { model: Math.ceil(parent.height / 3); Rectangle { width: parent.width; height: 3; color: index % 2 ? "transparent" : Qt.rgba(0, 0, 0, .16); required property int index } } }
                    Rectangle { anchors.fill: parent; radius: 26; gradient: Gradient { orientation: Gradient.Horizontal; GradientStop { position: 0; color: Qt.rgba(0, 0, 0, .25) } GradientStop { position: .15; color: "transparent" } GradientStop { position: .85; color: "transparent" } GradientStop { position: 1; color: Qt.rgba(0, 0, 0, .25) } } } }
                ColumnLayout { Layout.fillWidth: true; Layout.fillHeight: true; Layout.preferredWidth: 4; spacing: 12
                    TermView { Layout.fillWidth: true; Layout.fillHeight: true; Layout.preferredHeight: 6; theme: root.mocha; lines: F.fastfetch(); fontFamily: "FantasqueSansM Nerd Font Mono"; fontSize: 10; padding: 16; radius: 26; cursor: false }
                    TermView { Layout.fillWidth: true; Layout.fillHeight: true; Layout.preferredHeight: 3; theme: root.mocha; lines: F.ssh; fontFamily: "FantasqueSansM Nerd Font Mono"; fontSize: 13; padding: 16; radius: 26; active: false; cursorStyle: "underline" } }
            }
        }
        // Appearance panel: every control writes terminal.conf, and the change shows live.
        Rectangle {
            Layout.preferredWidth: 340; Layout.fillHeight: true; radius: 28; color: Theme.alpha(Theme.surfaceLow, .96)
            ColumnLayout {
                anchors.fill: parent; anchors.margins: 18; spacing: 12
                RowLayout { Layout.fillWidth: true
                    MText { text: "Appearance"; role: "headline"; Layout.fillWidth: true }
                    MButton { icon: "close"; compact: true } }
                MText { text: "Theme"; role: "label"; color: Theme.primary }
                GridLayout { columns: 2; rowSpacing: 8; columnSpacing: 8
                    Swatch { name: "Follow Opal"; colors: [Theme.surfaceLowest, Theme.text, Theme.primary, Theme.secondary, Theme.tertiary] }
                    Swatch { name: "Catppuccin Mocha"; cur: true; colors: ["#1e1e2e", "#cdd6f4", "#f38ba8", "#a6e3a1", "#89b4fa", "#cba6f7"] }
                    Swatch { name: "Gruvbox"; colors: ["#282828", "#ebdbb2", "#fb4934", "#b8bb26", "#fabd2f", "#83a598"] }
                    Swatch { name: "Tokyo Night"; colors: ["#1a1b26", "#c0caf5", "#f7768e", "#9ece6a", "#7aa2f7", "#bb9af7"] } }
                MButton { text: "Import kitty / alacritty / foot / ghostty theme"; icon: "file_open"; compact: true; Layout.fillWidth: true; contained: true }
                MText { text: "Font"; role: "label"; color: Theme.primary; Layout.topMargin: 4 }
                RowLayout { Layout.fillWidth: true; spacing: 8
                    Chip { text: "FantasqueSansM Nerd"; icon: "match_case"; trailingIcon: "expand_more"; glass: false }
                    Chip { text: "Ligatures"; selected: true } }
                RowLayout { Layout.fillWidth: true
                    MText { text: "Size"; role: "bodyS"; Layout.preferredWidth: 70 }
                    ExpressiveSlider { Layout.fillWidth: true; compact: true; showIcon: false; value: 45 }
                    MText { text: "16"; role: "label" } }
                RowLayout { Layout.fillWidth: true
                    MText { text: "Opacity"; role: "bodyS"; Layout.preferredWidth: 70 }
                    ExpressiveSlider { Layout.fillWidth: true; compact: true; showIcon: false; value: 72 }
                    MText { text: "72%"; role: "label" } }
                MText { text: "Cursor"; role: "label"; color: Theme.primary; Layout.topMargin: 4 }
                SegmentedButtons { current: "block"; options: [{id: "block", name: "Block"}, {id: "bar", name: "Bar"}, {id: "under", name: "Underline"}] }
                SwitchRow { Layout.fillWidth: true; title: "Cursor trail"; icon: "gesture"; checked: true; implicitHeight: 52 }
                MText { text: "Shader"; role: "label"; color: Theme.primary; Layout.topMargin: 4 }
                Flow { Layout.fillWidth: true; spacing: 6
                    Chip { text: "None"; glass: false }
                    Chip { text: "CRT"; selected: true }
                    Chip { text: "Bloom"; glass: false }
                    Chip { text: "Edge glow"; glass: false }
                    Chip { text: "Custom .glsl"; icon: "add"; glass: false } }
                Item { Layout.fillHeight: true }
                MButton { text: "Edit terminal.conf"; icon: "code"; tonal: true; Layout.fillWidth: true }
            }
        }
    }
}
