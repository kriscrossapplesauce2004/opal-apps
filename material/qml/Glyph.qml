import QtQuick
import Opal.Material
// Material Symbols Rounded glyph. Set fill to 1 for the filled style.
Text {
    property string name: "apps"
    property real fill: 0
    property int weight: 400
    text: Theme.glyphs[name] || name
    font.family: "Material Symbols Rounded"
    font.pixelSize: 22
    font.variableAxes: ({"FILL": fill, "wght": weight, "opsz": Math.max(20, Math.min(48, font.pixelSize))})
    color: Theme.text
    horizontalAlignment: Text.AlignHCenter
    verticalAlignment: Text.AlignVCenter
    renderType: Text.QtRendering
    Behavior on fill { NumberAnimation { duration: Theme.motion(Theme.durMedium) } }
}
