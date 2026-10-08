import QtQuick
import QtQuick.Layouts
import Opal.Material
// Shell-integration pill for one command: result, duration and quick actions. It floats at the command's line on hover.
Rectangle {
    id: pill
    property string state_: "fail"
    property string command: "ctest --test-dir build"
    property string detail: "exit 8 · 1.4 s"
    implicitHeight: 40
    implicitWidth: row.implicitWidth + 16
    radius: 20
    color: Theme.inverseSurface
    RowLayout { id: row; anchors.centerIn: parent; spacing: 8
        Rectangle { implicitWidth: 28; implicitHeight: 28; radius: 14; color: pill.state_ === "fail" ? Theme.error : Theme.primary
            Glyph { anchors.centerIn: parent; name: pill.state_ === "fail" ? "close" : "check"; font.pixelSize: 17; color: pill.state_ === "fail" ? Theme.role("on_error", "#601410") : Theme.onPrimary } }
        MText { text: pill.command; role: "label"; font.family: "CaskaydiaCove Nerd Font Mono"; color: Theme.inverseInk }
        MText { text: pill.detail; role: "labelS"; color: Theme.alpha(Theme.inverseInk, .7) }
        Rectangle { implicitWidth: 1; implicitHeight: 20; color: Theme.alpha(Theme.inverseInk, .25) }
        Repeater { model: ["content_copy", "replay", "bookmark_add"]
            Glyph { required property string modelData; name: modelData; font.pixelSize: 18; color: Theme.inverseInk } }
    }
}
