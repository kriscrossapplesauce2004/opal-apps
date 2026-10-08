import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Opal.Material

Rectangle {
    color: Theme.base

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 12

        RowLayout {
            Layout.fillWidth: true
            MButton { icon: "menu"; compact: true; Accessible.name: "Open menu" }
            MorphChip { text: "welcome.md"; icon: "description"; selected: true }
            Item { Layout.fillWidth: true }
            MButton { icon: "search"; compact: true; Accessible.name: "Find" }
            MButton { icon: "more_vert"; compact: true; Accessible.name: "More actions" }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            radius: 24
            color: Theme.surfaceLowest

            TextArea {
                anchors.fill: parent
                anchors.margins: 18
                text: "# Welcome to Opal Text Editor\n\nA plain text and Markdown editor built for every Opal posture.\n\n- Edit this document\n- Use Ctrl+F to find text\n- Open more files in tabs"
                color: Theme.text
                selectionColor: Theme.primary
                selectedTextColor: Theme.onPrimary
                wrapMode: TextEdit.Wrap
                font.family: "CaskaydiaCove Nerd Font Mono"
                font.pixelSize: 15
                background: null
                Accessible.name: "Document editor"
            }
        }

        RowLayout {
            Layout.fillWidth: true
            MText { text: "Markdown"; role: "labelS"; color: Theme.subtext }
            Item { Layout.fillWidth: true }
            MText { text: "Ln 1, Col 1"; role: "labelS"; color: Theme.subtext }
        }
    }
}
