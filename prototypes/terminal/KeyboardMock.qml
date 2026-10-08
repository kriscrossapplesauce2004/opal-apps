import QtQuick
import QtQuick.Layouts
import Opal.Material
// Stand-in for the on-screen keyboard in phone renders.
Rectangle {
    color: Theme.surfaceLow
    implicitHeight: 236
    readonly property var rows: [["q","w","e","r","t","y","u","i","o","p"], ["a","s","d","f","g","h","j","k","l"], ["⇧","z","x","c","v","b","n","m","⌫"], ["?123","/","space",".","↵"]]
    ColumnLayout {
        anchors.fill: parent; anchors.margins: 6; spacing: 8
        Repeater {
            model: parent.parent.rows
            RowLayout {
                required property var modelData
                required property int index
                Layout.fillWidth: true; spacing: 5
                Layout.leftMargin: index === 1 ? 18 : 0; Layout.rightMargin: index === 1 ? 18 : 0
                Repeater {
                    model: modelData
                    Rectangle {
                        required property string modelData
                        readonly property bool special: modelData.length > 1 || "⇧⌫↵".indexOf(modelData) >= 0
                        Layout.fillWidth: true; Layout.preferredWidth: modelData === "space" ? 5 : special ? 1.5 : 1; implicitHeight: 46; radius: 12
                        color: modelData === "↵" ? Theme.primary : special ? Theme.secondaryContainer : Theme.surfaceHighest
                        MText { anchors.centerIn: parent; text: modelData === "space" ? "" : modelData; font.pixelSize: 19; color: modelData === "↵" ? Theme.onPrimary : Theme.text }
                    }
                }
            }
        }
    }
}
