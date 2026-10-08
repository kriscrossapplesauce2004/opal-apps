import QtQuick
import QtQuick.Layouts
import Opal.Material
import "Fixtures.js" as F
import "../keyboard" as Kb
// Text Editor on a phone: opens straight into the document; a Markdown tool row rides above the keyboard.
Rectangle {
    color: Theme.base
    ColumnLayout {
        anchors.fill: parent; spacing: 0
        RowLayout { Layout.fillWidth: true; Layout.preferredHeight: 56; Layout.leftMargin: 4; Layout.rightMargin: 4
            MButton { icon: "arrow_back"; compact: true }
            ColumnLayout { Layout.fillWidth: true; spacing: 0; MText { text: "SPEC.md"; role: "titleS" } MText { text: "Edited · docs"; role: "labelS"; color: Theme.subtext } }
            MButton { icon: "undo"; compact: true } MButton { icon: "visibility"; compact: true } MButton { icon: "more_vert"; compact: true } }
        CodeView { Layout.fillWidth: true; Layout.fillHeight: true; Layout.margins: 6; lines: F.md.slice(0, 16); gutter: false; fontSize: 13; currentLine: 12 }
        Rectangle { Layout.fillWidth: true; implicitHeight: 52; color: Theme.surface
            RowLayout { anchors.fill: parent; anchors.leftMargin: 6; anchors.rightMargin: 6; spacing: 2
                Repeater { model: ["format_h1", "format_bold", "format_italic", "format_list_bulleted", "checklist", "link", "code", "format_quote"]
                    Rectangle { required property string modelData; Layout.fillWidth: true; implicitHeight: 40; radius: 12; color: modelData === "format_bold" ? Theme.secondaryContainer : "transparent"
                        Glyph { anchors.centerIn: parent; name: modelData; font.pixelSize: 21; color: modelData === "format_bold" ? Theme.onSecondaryContainer : Theme.text } } } } }
        Kb.Kbd { Layout.fillWidth: true; keyH: 48 }
    }
}
