import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Opal.Material
// Context menu in Opal style: a rounded card of rows with glyphs and shortcuts; submenus are not used.
Popup {
    id: menu
    property var entries: []
    signal triggered(string id)
    padding: 6
    modal: false
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    background: Rectangle { radius: 20; color: Theme.surfaceHigh; border.width: 1; border.color: Theme.outline }
    function openAt(item, x, y) { const p = item.mapToItem(menu.parent, x, y); menu.x = Math.min(p.x, menu.parent.width - implicitWidth - 8); menu.y = Math.min(p.y, menu.parent.height - implicitHeight - 8); open(); }
    contentItem: ColumnLayout {
        spacing: 0
        Repeater {
            model: menu.entries
            Loader {
                required property var modelData
                Layout.fillWidth: true
                sourceComponent: modelData.sep ? sep : item
                Component { id: sep; Rectangle { implicitHeight: 9; color: "transparent"; Rectangle { anchors.centerIn: parent; width: parent.width - 20; height: 1; color: Theme.outline } } }
                Component { id: item
                    Rectangle {
                        implicitWidth: Math.max(240, r.implicitWidth + 28); implicitHeight: Theme.touch ? 48 : 38; radius: height / 2
                        color: h.hovered ? Theme.alpha(Theme.text, .08) : "transparent"
                        opacity: modelData.disabled ? .4 : 1
                        RowLayout { id: r; anchors.fill: parent; anchors.leftMargin: 14; anchors.rightMargin: 14; spacing: 12
                            Glyph { name: modelData.i || ""; font.pixelSize: 19; color: modelData.danger ? Theme.error : Theme.subtext }
                            MText { Layout.fillWidth: true; text: modelData.t; role: "body"; color: modelData.danger ? Theme.error : Theme.text }
                            MText { visible: !!modelData.k; text: modelData.k || ""; role: "labelS"; color: Theme.muted } }
                        HoverHandler { id: h }
                        TapHandler { enabled: !modelData.disabled; onTapped: { menu.close(); menu.triggered(modelData.id); } }
                        Accessible.role: Accessible.MenuItem; Accessible.name: modelData.t
                    } }
            }
        }
    }
}
