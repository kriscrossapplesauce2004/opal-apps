import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts
import Opal.Material
import Opal.TerminalUi
// Appearance panel: every control writes terminal.conf, and the change shows at once.
Rectangle {
    id: panel
    property var view
    implicitWidth: 340
    radius: 28
    color: Theme.alpha(Theme.surfaceLow, .97)
    component Swatch: Rectangle {
        property string name; property var colors: []; property bool cur: false; property bool builtin: true
        signal picked()
        implicitWidth: 146; implicitHeight: 64; radius: 18
        color: colors[0] || Theme.surfaceHigh; border.width: cur ? 3 : 0; border.color: Theme.primary
        Row { anchors.left: parent.left; anchors.top: parent.top; anchors.margins: 10; spacing: 4
            Repeater { model: colors.slice(2); Rectangle { required property var modelData; width: 13; height: 13; radius: 7; color: modelData } } }
        Text { anchors.left: parent.left; anchors.bottom: parent.bottom; anchors.margins: 10; width: parent.width - 34; text: name; elide: Text.ElideRight; color: colors[1] || Theme.text; font.family: Theme.font; font.pixelSize: 12; font.weight: Font.DemiBold }
        Glyph { visible: cur; anchors.right: parent.right; anchors.top: parent.top; anchors.margins: 8; name: "check_circle"; fill: 1; font.pixelSize: 18; color: Theme.primary }
        TapHandler { onTapped: parent.picked() }
        Accessible.role: Accessible.RadioButton; Accessible.name: name; Accessible.checked: cur
    }
    component Section: MText { role: "label"; color: Theme.primary; Layout.topMargin: 6 }
    component SliderRow: RowLayout {
        property string label; property real from: 0; property real to: 1; property real value: 0; property real step: 0; property string text: ""
        signal moved(real v)
        Layout.fillWidth: true
        MText { text: label; role: "bodyS"; Layout.preferredWidth: 78 }
        ExpressiveSlider { Layout.fillWidth: true; compact: true; showIcon: false; from: parent.from; to: parent.to; stepSize: parent.step; value: parent.value; onMoved: parent.moved(value) }
        MText { text: parent.text; role: "label"; Layout.preferredWidth: 38; horizontalAlignment: Text.AlignRight }
    }
    FileDialog { id: importDialog; title: qsTr("Import a terminal theme"); nameFilters: [qsTr("Theme files (*.conf *.toml *.ini *.theme *)")]
        onAccepted: { const n = TerminalThemes.importFile(selectedFile.toString()); if (n) TerminalConfig.set("theme", "name", n); } }
    Flickable {
        anchors.fill: parent; anchors.margins: 18; contentHeight: col.height; clip: true
        ColumnLayout {
            id: col; width: parent.width; spacing: 10
            RowLayout { Layout.fillWidth: true
                MText { text: qsTr("Appearance"); role: "headline"; Layout.fillWidth: true }
                MButton { icon: "close"; compact: true; onClicked: panel.view.appearance = false } }
            Section { text: qsTr("Theme") }
            Flow { Layout.fillWidth: true; spacing: 8
                Swatch { name: qsTr("Follow Opal"); cur: TerminalConfig.theme === "opal"; colors: [Theme.surfaceLowest, Theme.text, Theme.primary, Theme.secondary, Theme.tertiary, Theme.error]; onPicked: TerminalConfig.set("theme", "name", "opal") }
                Repeater { model: TerminalThemes.themes
                    Swatch { required property var modelData; name: modelData.name; builtin: modelData.builtin; cur: TerminalConfig.theme === modelData.name
                        colors: [modelData.background, modelData.foreground].concat(modelData.accents.slice(0, 5)); onPicked: TerminalConfig.set("theme", "name", modelData.name) } } }
            MButton { text: qsTr("Import kitty / alacritty / foot / ghostty theme"); icon: "file_open"; compact: true; contained: true; Layout.fillWidth: true; onClicked: importDialog.open() }
            MText { visible: TerminalThemes.lastError !== ""; text: TerminalThemes.lastError; role: "labelS"; color: Theme.error; Layout.fillWidth: true; wrapMode: Text.Wrap }
            Section { text: qsTr("Font") }
            ComboBox {
                id: fontBox
                Layout.fillWidth: true; implicitHeight: 44
                model: TerminalHelpers.monospaceFonts()
                currentIndex: Math.max(0, model.indexOf(TerminalConfig.fontFamily))
                onActivated: i => TerminalConfig.set("font", "family", model[i])
                contentItem: MText { leftPadding: 16; text: fontBox.displayText; verticalAlignment: Text.AlignVCenter; font.family: fontBox.displayText }
                background: Rectangle { radius: 22; color: Theme.surfaceHigh }
            }
            SliderRow { label: qsTr("Size"); from: 8; to: 28; step: 1; value: TerminalConfig.fontSize; text: Math.round(value); onMoved: v => TerminalConfig.set("font", "size", Math.round(v)) }
            SliderRow { label: qsTr("Line height"); from: 1; to: 1.8; step: .05; value: TerminalConfig.lineHeight; text: value.toFixed(2); onMoved: v => TerminalConfig.set("font", "line-height", v.toFixed(2)) }
            Section { text: qsTr("Window") }
            SliderRow { label: qsTr("Opacity"); from: .3; to: 1; value: TerminalConfig.opacity; text: Math.round(value * 100) + "%"; onMoved: v => TerminalConfig.set("window", "opacity", v.toFixed(2)) }
            SliderRow { label: qsTr("Padding"); from: 0; to: 40; step: 1; value: TerminalConfig.padding; text: Math.round(value); onMoved: v => TerminalConfig.set("window", "padding", Math.round(v)) }
            SliderRow { label: qsTr("Blur"); from: 0; to: 1; value: TerminalConfig.blur; text: Math.round(value * 100) + "%"; onMoved: v => TerminalConfig.set("window", "blur", v.toFixed(2)) }
            Section { text: qsTr("Cursor") }
            SegmentedButtons { current: TerminalConfig.cursorStyle; onPicked: id => TerminalConfig.set("cursor", "style", id)
                options: [{id: "block", name: qsTr("Block")}, {id: "bar", name: qsTr("Bar")}, {id: "underline", name: qsTr("Underline")}] }
            SwitchRow { Layout.fillWidth: true; title: qsTr("Cursor trail"); icon: "gesture"; checked: TerminalConfig.cursorTrail; implicitHeight: 52; onToggled: TerminalConfig.set("cursor", "trail", !TerminalConfig.cursorTrail) }
            SwitchRow { Layout.fillWidth: true; title: qsTr("Blink"); icon: "flare"; checked: TerminalConfig.cursorBlink; implicitHeight: 52; onToggled: TerminalConfig.set("cursor", "blink", !TerminalConfig.cursorBlink) }
            Section { text: qsTr("Shader") }
            Flow { Layout.fillWidth: true; spacing: 6
                Repeater { model: [["none", qsTr("None")], ["crt", "CRT"], ["bloom", qsTr("Bloom")], ["edge-glow", qsTr("Edge glow")]]
                    Chip { required property var modelData; text: modelData[1]; selected: TerminalConfig.shader === modelData[0]; glass: false; onClicked: TerminalConfig.set("effects", "shader", modelData[0]) } } }
            Item { implicitHeight: 6 }
            MButton { text: qsTr("Edit terminal.conf"); icon: "code"; tonal: true; Layout.fillWidth: true; onClicked: TerminalHelpers.openFile(TerminalConfig.path) }
        }
    }
}
