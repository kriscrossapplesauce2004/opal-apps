import QtQuick
import QtQuick.Layouts
import Opal.Material
// Status line as quiet chips: language, position, indentation, encoding, line endings.
RowLayout {
    property string lang: "Markdown"
    property string pos: "Ln 12, Col 4"
    spacing: 6
    Repeater { model: [{i: "code", t: lang}, {i: "my_location", t: pos}, {i: "space_bar", t: "Spaces: 4"}, {i: "text_fields", t: "UTF-8"}, {i: "keyboard_return", t: "LF"}]
        Rectangle { required property var modelData; implicitHeight: 28; implicitWidth: r.implicitWidth + 20; radius: 14; color: Theme.surfaceHigh
            RowLayout { id: r; anchors.centerIn: parent; spacing: 5; Glyph { name: modelData.i; font.pixelSize: 14; color: Theme.subtext } MText { text: modelData.t; role: "labelS" } } } }
}
