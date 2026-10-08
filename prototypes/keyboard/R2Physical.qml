import QtQuick
import QtQuick.Layouts
import Opal.Material
// A physical keyboard is attached: Borealis shrinks to a slim bar with suggestions, emoji, clipboard and a way back to the full deck.
Rectangle {
    color: Theme.base
    ColumnLayout { anchors.fill: parent; anchors.margins: 40; anchors.bottomMargin: 96; spacing: 16
        MText { text: "Notes"; role: "displayS" }
        Rectangle { Layout.fillWidth: true; Layout.fillHeight: true; radius: 28; color: Theme.surface
            MText { anchors.fill: parent; anchors.margins: 24; wrapMode: Text.Wrap; font.pixelSize: 20; text: "Docked with a keyboard, the deck folds into this bar: suggestions, emoji and clipboard stay one tap away. hel" } } }
    Rectangle {
        anchors.horizontalCenter: parent.horizontalCenter; anchors.bottom: parent.bottom; anchors.bottomMargin: 20
        width: 760; height: 60; radius: 30; color: Theme.surfaceHigh
        RowLayout { anchors.fill: parent; anchors.leftMargin: 10; anchors.rightMargin: 10; spacing: 6
            Rectangle { implicitWidth: 40; implicitHeight: 40; radius: 20; color: Theme.alpha(Theme.primary, .16)
                Glyph { anchors.centerIn: parent; name: "keyboard_external_input"; font.pixelSize: 20; color: Theme.primary } }
            Repeater { model: [{t: "hello"}, {t: "help", m: true}, {t: "helm"}]
                Rectangle { required property var modelData; Layout.fillWidth: true; implicitHeight: 40; radius: 20; color: modelData.m ? Theme.secondaryContainer : "transparent"
                    MText { anchors.centerIn: parent; text: modelData.t; font.pixelSize: 16; font.weight: modelData.m ? Font.DemiBold : Font.Normal; color: modelData.m ? Theme.onSecondaryContainer : Theme.text } } }
            Rectangle { implicitWidth: 1; implicitHeight: 28; color: Theme.outline }
            Repeater { model: ["mood", "content_paste", "keyboard"]
                Rectangle { required property string modelData; implicitWidth: 40; implicitHeight: 40; radius: 20; color: "transparent"
                    Glyph { anchors.centerIn: parent; name: modelData; font.pixelSize: 21; color: Theme.subtext } } }
        }
    }
}
