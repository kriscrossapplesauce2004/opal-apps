import QtQuick
import QtQuick.Layouts
import Opal.Material
// M3 top app bar: optional navigation glyph, a title with an optional subtitle, and trailing actions. large gives the medium-flexible headline style.
Item {
    id: bar
    property string title: ""
    property string subtitle: ""
    property string navIcon: ""
    property bool large: false
    default property alias actions: trail.data
    signal navClicked()
    implicitHeight: large ? (subtitle !== "" ? Theme.px(136) : Theme.px(112)) : Theme.px(64)
    RowLayout {
        id: top
        anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top
        height: Theme.px(64); anchors.leftMargin: bar.navIcon !== "" ? 4 : Theme.px(16); anchors.rightMargin: 4; spacing: 4
        MButton { visible: bar.navIcon !== ""; icon: bar.navIcon; compact: true; onClicked: bar.navClicked() }
        ColumnLayout { visible: !bar.large; Layout.fillWidth: true; spacing: 0
            MText { Layout.fillWidth: true; text: bar.title; role: "title" }
            MText { visible: bar.subtitle !== ""; Layout.fillWidth: true; text: bar.subtitle; role: "label"; color: Theme.subtext } }
        Item { visible: bar.large; Layout.fillWidth: true }
        Row { id: trail; spacing: 0; Layout.alignment: Qt.AlignVCenter }
    }
    ColumnLayout {
        visible: bar.large
        anchors.left: parent.left; anchors.right: parent.right; anchors.bottom: parent.bottom; anchors.leftMargin: Theme.px(20); anchors.bottomMargin: Theme.px(14); spacing: 2
        MText { Layout.fillWidth: true; text: bar.title; role: "headline" }
        MText { visible: bar.subtitle !== ""; Layout.fillWidth: true; text: bar.subtitle; role: "body"; color: Theme.subtext }
    }
}
