import QtQuick
import QtQuick.Layouts
import Opal.Material
// Couch and handheld: big keys, a focus ring, and controller hints. Suggestions sit on the shoulder buttons.
Rectangle {
    color: Theme.base
    ColumnLayout {
        anchors.fill: parent; anchors.margins: 60; spacing: 24
        Rectangle { Layout.fillWidth: true; implicitHeight: 80; radius: 40; color: Theme.surfaceHigh
            RowLayout { anchors.fill: parent; anchors.leftMargin: 28; anchors.rightMargin: 28; spacing: 16
                Glyph { name: "search"; font.pixelSize: 34; color: Theme.subtext }
                MText { text: "deltar"; font.pixelSize: 34; Layout.fillWidth: true }
                MText { text: "Steam library"; font.pixelSize: 20; color: Theme.subtext } } }
        RowLayout { Layout.alignment: Qt.AlignHCenter; spacing: 16
            Repeater { model: [{b: "LB", t: "deltarune"}, {b: "", t: "deltarune ch 5", m: true}, {b: "RB", t: "delta"}]
                Rectangle { required property var modelData; implicitHeight: 56; implicitWidth: sr.implicitWidth + 36; radius: 28; color: modelData.m ? Theme.secondaryContainer : Theme.surfaceHigh
                    RowLayout { id: sr; anchors.centerIn: parent; spacing: 10
                        Rectangle { visible: modelData.b !== ""; implicitWidth: 40; implicitHeight: 26; radius: 8; color: Theme.surfaceHighest
                            MText { anchors.centerIn: parent; text: modelData.b; font.pixelSize: 13; font.weight: Font.Bold } }
                        MText { text: modelData.t; font.pixelSize: 22; font.weight: modelData.m ? Font.Bold : Font.Medium; color: modelData.m ? Theme.onSecondaryContainer : Theme.text } } } } }
        Item { Layout.fillHeight: true }
        Rectangle {
            Layout.alignment: Qt.AlignHCenter; implicitWidth: 1240; implicitHeight: 420; radius: 36; color: Theme.surfaceLow
            GridLayout { anchors.fill: parent; anchors.margins: 20; columns: 10; rowSpacing: 10; columnSpacing: 10
                Repeater { model: "1234567890qwertyuiopasdfghjkl'zxcvbnm,.?".split("")
                    Rectangle { required property string modelData; required property int index; readonly property bool f: modelData === "u"
                        Layout.fillWidth: true; Layout.fillHeight: true; radius: f ? 30 : 22; color: f ? Theme.primary : Theme.surfaceHighest
                        border.width: f ? 4 : 0; border.color: Theme.alpha(Theme.onPrimary, .5); scale: f ? 1.08 : 1; z: f ? 1 : 0
                        MText { anchors.centerIn: parent; text: modelData; font.pixelSize: 34; color: parent.f ? Theme.onPrimary : Theme.text } } } } }
        HintBar { Layout.alignment: Qt.AlignHCenter; items: [{b: "A", t: "Type"}, {b: "B", t: "Delete"}, {b: "X", t: "Space"}, {b: "Y", t: "Shift"}, {b: "LT", t: "Symbols"}, {b: "RT", t: "Cursor"}, {b: "Start", t: "Done"}] }
    }
}
