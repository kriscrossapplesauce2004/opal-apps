import QtQuick
import QtQuick.Layouts
import Opal.Material
// Slim pill tabs with a status dot: amber while a command runs, red after a failure, accent after success.
RowLayout {
    id: tabs
    property var model: []
    property int current: 0
    signal picked(int index)
    signal closed(int index)
    signal added()
    spacing: 4
    Repeater {
        model: tabs.model
        Rectangle {
            id: tab
            required property var modelData
            required property int index
            readonly property bool cur: index === tabs.current
            implicitHeight: 36; implicitWidth: Math.min(240, r.implicitWidth + 28); radius: 18
            color: cur ? Theme.surfaceHigh : th.hovered ? Theme.alpha(Theme.text, .05) : "transparent"
            RowLayout { id: r; anchors.fill: parent; anchors.leftMargin: 13; anchors.rightMargin: tab.cur && tabs.model.length > 1 ? 6 : 13; spacing: 8
                Glyph { name: tab.modelData.icon; font.pixelSize: 17; color: tab.cur ? Theme.primary : Theme.subtext }
                MText { Layout.maximumWidth: 160; text: tab.modelData.title; role: "label"; font.pixelSize: 13; font.weight: tab.cur ? Font.DemiBold : Font.Medium; color: tab.cur ? Theme.text : Theme.subtext }
                Rectangle { visible: tab.modelData.status !== ""; implicitWidth: 8; implicitHeight: 8; radius: 4; color: tab.modelData.status === "fail" ? Theme.error : tab.modelData.status === "running" ? Theme.tertiary : Theme.primary }
                MButton { visible: tab.cur && tabs.model.length > 1; icon: "close"; compact: true; implicitWidth: 26; implicitHeight: 26; onClicked: tabs.closed(tab.index) } }
            HoverHandler { id: th }
            TapHandler { acceptedButtons: Qt.LeftButton; onTapped: tabs.picked(tab.index) }
            TapHandler { acceptedButtons: Qt.MiddleButton; onTapped: tabs.closed(tab.index) }
            Accessible.role: Accessible.PageTab; Accessible.name: modelData.title; Accessible.selected: cur
        }
    }
    MButton { icon: "add"; compact: true; tooltip: qsTr("New tab (Ctrl+Shift+T)"); onClicked: tabs.added() }
}
