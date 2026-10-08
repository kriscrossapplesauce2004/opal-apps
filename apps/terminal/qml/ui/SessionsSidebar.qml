import QtQuick
import QtQuick.Layouts
import Opal.Material
import Opal.TerminalUi
// The pop-out sessions sidebar: running sessions across tabs, and saved sessions from terminal.conf profiles.
Rectangle {
    id: side
    property var view
    implicitWidth: 280
    radius: 24
    color: Theme.surfaceLow
    component Row_: Rectangle {
        property string title; property string detail; property string icon: "terminal"; property bool cur: false; property string status: ""
        signal activated()
        Layout.fillWidth: true; implicitHeight: 58; radius: 20
        color: cur ? Theme.secondaryContainer : rh.hovered ? Theme.alpha(Theme.text, .05) : "transparent"
        RowLayout { anchors.fill: parent; anchors.leftMargin: 12; anchors.rightMargin: 12; spacing: 12
            Rectangle { implicitWidth: 34; implicitHeight: 34; radius: 12; color: Theme.alpha(cur ? Theme.primary : Theme.subtext, .16)
                Glyph { anchors.centerIn: parent; name: icon; font.pixelSize: 19; color: cur ? Theme.primary : Theme.subtext } }
            ColumnLayout { Layout.fillWidth: true; spacing: 0
                MText { Layout.fillWidth: true; text: title; role: "titleS"; font.pixelSize: 14; color: cur ? Theme.onSecondaryContainer : Theme.text }
                MText { Layout.fillWidth: true; visible: detail !== ""; text: detail; role: "labelS"; font.family: TerminalConfig.fontFamily; color: Theme.subtext } }
            Rectangle { visible: status !== ""; implicitWidth: 8; implicitHeight: 8; radius: 4; color: status === "fail" ? Theme.error : status === "running" ? Theme.tertiary : Theme.primary } }
        HoverHandler { id: rh }
        TapHandler { onTapped: activated() }
        Accessible.role: Accessible.ListItem; Accessible.name: title
    }
    ColumnLayout {
        anchors.fill: parent; anchors.margins: 12; spacing: 4
        RowLayout { Layout.fillWidth: true; Layout.bottomMargin: 8
            Fab { size: "small"; icon: "add"; text: qsTr("New session"); onClicked: side.view.newTab() }
            Item { Layout.fillWidth: true }
            MButton { icon: "close"; compact: true; onClicked: side.view.sidebar = false } }
        MText { text: qsTr("Running"); role: "label"; color: Theme.subtext; Layout.leftMargin: 12 }
        Repeater {
            model: side.view.paneList
            Row_ { required property var modelData
                title: modelData.title; detail: modelData.detail; icon: modelData.icon; cur: modelData.focused; status: modelData.status
                onActivated: side.view.focusPaneAnywhere(modelData.id) }
        }
        MText { visible: TerminalConfig.profiles.length > 0; text: qsTr("Saved"); role: "label"; color: Theme.subtext; Layout.leftMargin: 12; Layout.topMargin: 12 }
        Repeater {
            model: TerminalConfig.profiles
            Row_ { required property var modelData
                title: modelData.name; detail: modelData.command; icon: modelData.icon
                onActivated: side.view.newTab(modelData.command, modelData.cwd) }
        }
        Item { Layout.fillHeight: true }
        Rectangle { Layout.fillWidth: true; implicitHeight: 58; radius: 18; color: Theme.surface
            RowLayout { anchors.fill: parent; anchors.margins: 12; spacing: 10
                Glyph { name: "edit_note"; color: Theme.primary }
                MText { Layout.fillWidth: true; text: qsTr("Add saved sessions as [profile NAME] in terminal.conf"); role: "labelS"; wrapMode: Text.Wrap; color: Theme.subtext } } }
    }
}
