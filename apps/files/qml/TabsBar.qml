import QtQuick
import QtQuick.Layouts
import Opal.Material
// Pill tabs: icon, folder name, close on the current tab; middle-click closes; + opens a new tab.
RowLayout {
    id: bar
    property var tabs: []
    property int current: 0
    signal picked(int index)
    signal closed(int index)
    signal added()
    spacing: 4
    Repeater {
        model: bar.tabs
        Rectangle {
            id: tab
            required property var modelData
            required property int index
            readonly property bool cur: index === bar.current
            implicitHeight: 40; implicitWidth: Math.min(220, tr.implicitWidth + 30); radius: 20
            color: cur ? Theme.surfaceHigh : th.hovered ? Theme.alpha(Theme.text, .05) : "transparent"
            RowLayout { id: tr; anchors.fill: parent; anchors.leftMargin: 14; anchors.rightMargin: tab.cur && bar.tabs.length > 1 ? 6 : 14; spacing: 8
                Glyph { name: tab.modelData.glyph || "folder"; fill: tab.cur ? 1 : 0; font.pixelSize: 18; color: tab.cur ? Theme.primary : Theme.subtext }
                MText { Layout.fillWidth: true; Layout.maximumWidth: 150; text: tab.modelData.name; role: "label"; font.pixelSize: 13; font.weight: tab.cur ? Font.DemiBold : Font.Medium; color: tab.cur ? Theme.text : Theme.subtext }
                MButton { visible: tab.cur && bar.tabs.length > 1; icon: "close"; compact: true; implicitWidth: 28; implicitHeight: 28; onClicked: bar.closed(tab.index) } }
            HoverHandler { id: th }
            TapHandler { acceptedButtons: Qt.LeftButton; onTapped: bar.picked(tab.index) }
            TapHandler { acceptedButtons: Qt.MiddleButton; onTapped: bar.closed(tab.index) }
            Accessible.role: Accessible.PageTab; Accessible.name: modelData.name; Accessible.selected: cur
        }
    }
    MButton { icon: "add"; compact: true; tooltip: qsTr("New tab"); onClicked: bar.added() }
}
