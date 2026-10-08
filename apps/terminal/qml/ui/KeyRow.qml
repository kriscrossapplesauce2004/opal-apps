import QtQuick
import QtQuick.Layouts
import Opal.Material
import Opal.TerminalUi
// Extra key row for touch screens, from terminal.conf [keys] key-row. Ctrl and Alt latch for the next key.
Rectangle {
    id: row
    property var session
    property bool ctrl: false
    property bool alt: false
    implicitHeight: 52
    color: Theme.surface
    readonly property var keyMap: ({"Esc": Qt.Key_Escape, "Tab": Qt.Key_Tab, "Up": Qt.Key_Up, "Down": Qt.Key_Down, "Left": Qt.Key_Left, "Right": Qt.Key_Right, "Home": Qt.Key_Home, "End": Qt.Key_End, "PgUp": Qt.Key_PageUp, "PgDn": Qt.Key_PageDown})
    function press(k) {
        if (!session) return;
        if (k === "Ctrl") { ctrl = !ctrl; return; }
        if (k === "Alt") { alt = !alt; return; }
        const mods = (ctrl ? Qt.ControlModifier : 0) | (alt ? Qt.AltModifier : 0);
        if (keyMap[k] !== undefined) session.sendKey(keyMap[k], mods, "");
        else if (mods) session.sendKey(k.toUpperCase().charCodeAt(0), mods, k);
        else session.sendText(k, false);
        ctrl = false; alt = false;
    }
    RowLayout {
        anchors.fill: parent; anchors.leftMargin: 6; anchors.rightMargin: 6; spacing: 4
        Repeater {
            model: TerminalConfig.keyRow
            Rectangle {
                id: key
                required property string modelData
                readonly property bool latched: (modelData === "Ctrl" && row.ctrl) || (modelData === "Alt" && row.alt)
                readonly property bool arrows: modelData === "Arrows"
                Layout.fillWidth: true; Layout.preferredWidth: arrows ? 1.6 : 1; implicitHeight: 40; radius: arrows ? 20 : 12
                color: latched ? Theme.primary : arrows ? Theme.secondaryContainer : Theme.surfaceHighest
                MText { visible: !key.arrows; anchors.centerIn: parent; text: key.modelData; font.family: TerminalConfig.fontFamily; font.pixelSize: 15; font.weight: Font.DemiBold; color: key.latched ? Theme.onPrimary : Theme.text }
                Glyph { visible: key.arrows; anchors.centerIn: parent; name: "open_with"; font.pixelSize: 22; color: Theme.onSecondaryContainer }
                TapHandler { enabled: !key.arrows; onTapped: row.press(key.modelData) }
                // The arrow pad sends an arrow for each step of a swipe.
                DragHandler { enabled: key.arrows; target: null; property point last
                    onActiveChanged: last = centroid.position
                    onCentroidChanged: if (active) { const d = Qt.point(centroid.position.x - last.x, centroid.position.y - last.y);
                        if (Math.abs(d.x) > 18 || Math.abs(d.y) > 18) { row.press(Math.abs(d.x) > Math.abs(d.y) ? (d.x > 0 ? "Right" : "Left") : (d.y > 0 ? "Down" : "Up")); last = centroid.position; } } }
                Accessible.role: Accessible.Button; Accessible.name: modelData
            }
        }
    }
}
