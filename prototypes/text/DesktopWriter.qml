import QtQuick
import QtQuick.Layouts
import Opal.Material
import "Fixtures.js" as F
// Text Editor direction A, "Writer": pill tabs, Markdown source beside a live preview, status chips.
Rectangle {
    color: Theme.base
    component Tab: Rectangle { property string label; property string icon: "description"; property bool cur: false; property bool dirty: false
        implicitHeight: 36; implicitWidth: tr.implicitWidth + 26; radius: 18; color: cur ? Theme.surfaceHigh : "transparent"
        RowLayout { id: tr; anchors.centerIn: parent; spacing: 8
            Glyph { name: icon; font.pixelSize: 17; color: cur ? Theme.primary : Theme.subtext }
            MText { text: label; role: "label"; font.pixelSize: 13; font.weight: cur ? Font.DemiBold : Font.Medium; color: cur ? Theme.text : Theme.subtext }
            Rectangle { visible: dirty; implicitWidth: 8; implicitHeight: 8; radius: 4; color: Theme.primary } } }
    ColumnLayout {
        anchors.fill: parent; anchors.margins: 8; spacing: 8
        RowLayout { Layout.fillWidth: true; spacing: 4
            MButton { icon: "menu"; compact: true }
            Tab { label: "SPEC.md"; cur: true; dirty: true }
            Tab { label: "palette.cpp"; icon: "code" }
            Tab { label: "notes.txt" }
            MButton { icon: "add"; compact: true }
            Item { Layout.fillWidth: true }
            SegmentedButtons { current: "split"; options: [{id: "src", icon: "code"}, {id: "split", icon: "vertical_split"}, {id: "prev", icon: "visibility"}] }
            MButton { icon: "search"; compact: true }
            MButton { icon: "more_vert"; compact: true } }
        RowLayout { Layout.fillWidth: true; Layout.fillHeight: true; spacing: 8
            CodeView { Layout.fillWidth: true; Layout.fillHeight: true; lines: F.md; currentLine: 22 }
            Preview { Layout.fillWidth: true; Layout.fillHeight: true; lines: F.md } }
        RowLayout { Layout.fillWidth: true; Layout.leftMargin: 6
            MText { text: "~/.local/state/opal/apps/docs/SPEC.md"; role: "labelS"; color: Theme.subtext; Layout.fillWidth: true }
            MText { text: "1,284 words"; role: "labelS"; color: Theme.subtext; Layout.rightMargin: 10 }
            StatusChips {} }
    }
}
