import QtQuick
import QtQuick.Layouts
import Opal.Material
// Floating toolbar for the selection; the count chip clears it. Icons only on narrow widths.
Rectangle {
    id: bar
    property int count: 0
    property bool showLabels: true
    property bool showCount: true
    property var actions: []
    signal triggered(string id)
    signal cleared()
    implicitHeight: Theme.couch ? 80 : 64
    implicitWidth: row.implicitWidth + 16
    radius: height / 2
    color: Theme.primaryContainer
    RowLayout {
        id: row; anchors.centerIn: parent; spacing: 4
        Rectangle { visible: bar.showCount; implicitHeight: bar.height - 16; implicitWidth: countRow.implicitWidth + 28; radius: height / 2; color: Theme.primary
            RowLayout { id: countRow; anchors.centerIn: parent; spacing: 6
                Glyph { name: "close"; font.pixelSize: Theme.px(18); color: Theme.onPrimary }
                MText { text: qsTr("%1 selected").arg(bar.count); role: "label"; font.pixelSize: Theme.px(13); font.weight: Font.Bold; color: Theme.onPrimary } }
            TapHandler { onTapped: bar.cleared() } }
        Repeater {
            model: bar.actions
            Rectangle {
                id: act
                required property var modelData
                implicitHeight: bar.height - 16; implicitWidth: actRow.implicitWidth + (bar.showLabels ? 24 : 16); radius: height / 2
                color: ah.hovered ? Theme.alpha(Theme.onContainer, .08) : "transparent"
                RowLayout { id: actRow; anchors.centerIn: parent; spacing: 6
                    Glyph { name: act.modelData.i; font.pixelSize: Theme.px(20); color: act.modelData.id === "trash" ? Theme.error : Theme.onContainer }
                    MText { visible: bar.showLabels; text: act.modelData.t; role: "label"; font.pixelSize: Theme.px(13); color: Theme.onContainer } }
                HoverHandler { id: ah }
                TapHandler { onTapped: bar.triggered(act.modelData.id) }
                Accessible.role: Accessible.Button; Accessible.name: modelData.t
            }
        }
    }
}
