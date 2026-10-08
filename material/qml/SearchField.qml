import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Opal.Material
// M3 search bar: a full pill with a leading glyph, optional trailing actions and a placeholder.
Rectangle {
    id: bar
    property alias text: input.text
    property string placeholder: qsTr("Search")
    property string leadingIcon: "search"
    property alias input: input
    default property alias actions: trail.data
    signal accepted()
    signal leadingClicked()
    implicitHeight: Theme.couch ? 64 : Theme.touch ? 56 : 48
    implicitWidth: 360
    radius: height / 2
    color: Theme.surfaceHigh
    border.width: input.activeFocus ? 2 : 0
    border.color: Theme.primary
    RowLayout {
        anchors.fill: parent; anchors.leftMargin: Theme.px(16); anchors.rightMargin: Theme.px(8); spacing: Theme.px(12)
        Glyph { name: bar.leadingIcon; color: Theme.text; font.pixelSize: Theme.px(22)
            TapHandler { onTapped: bar.leadingClicked() } }
        TextField {
            id: input
            Layout.fillWidth: true
            background: null
            padding: 0
            color: Theme.text
            placeholderText: bar.placeholder
            placeholderTextColor: Theme.subtext
            font.family: Theme.font; font.pixelSize: Theme.px(15)
            selectionColor: Theme.alpha(Theme.primary, .4)
            onAccepted: bar.accepted()
        }
        Row { id: trail; spacing: 0; Layout.alignment: Qt.AlignVCenter }
    }
    Accessible.role: Accessible.EditableText
    Accessible.name: placeholder
}
