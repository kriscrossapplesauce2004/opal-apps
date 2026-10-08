import QtQuick
import QtQuick.Layouts
import Opal.Material
// Smart extra-key row above the keyboard. Ctrl is latched (filled); keys are remappable in terminal.conf; swipe the arrow pad for repeat.
Rectangle {
    implicitHeight: 52
    color: Theme.surface
    RowLayout {
        anchors.fill: parent; anchors.leftMargin: 6; anchors.rightMargin: 6; spacing: 4
        Repeater {
            model: [{t: "Esc"}, {t: "Ctrl", on: true}, {t: "Alt"}, {t: "Tab"}, {t: "|"}, {t: "~"}, {t: "/"}, {t: "-"}]
            Rectangle { required property var modelData
                Layout.fillWidth: true; implicitHeight: 40; radius: 12
                color: modelData.on ? Theme.primary : Theme.surfaceHighest
                MText { anchors.centerIn: parent; text: modelData.t; font.family: "CaskaydiaCove Nerd Font Mono"; font.pixelSize: 15; font.weight: Font.DemiBold; color: modelData.on ? Theme.onPrimary : Theme.text } }
        }
        // Arrow pad: one key, swipe in a direction.
        Rectangle { Layout.preferredWidth: 58; implicitHeight: 40; radius: 20; color: Theme.secondaryContainer
            Glyph { anchors.centerIn: parent; name: "open_with"; font.pixelSize: 22; color: Theme.onSecondaryContainer } }
    }
}
