import QtQuick
import QtQuick.Layouts
import Opal.Material
// Compact month grid for October 2026 with today and the shown week marked.
ColumnLayout {
    spacing: 6
    RowLayout { Layout.fillWidth: true; MText { text: "October 2026"; role: "titleS"; Layout.fillWidth: true } Glyph { name: "chevron_left"; color: Theme.subtext } Glyph { name: "chevron_right"; color: Theme.subtext } }
    Grid { columns: 7; rowSpacing: 2; columnSpacing: 2
        Repeater { model: ["M","T","W","T","F","S","S"]; MText { required property string modelData; width: 28; height: 22; horizontalAlignment: Text.AlignHCenter; text: modelData; role: "labelS"; color: Theme.subtext } }
        Repeater { model: 35
            Rectangle { required property int index; readonly property int day: index - 2; width: 28; height: 28; radius: 14
                readonly property bool inWeek: day >= 5 && day <= 11
                color: day === 8 ? Theme.primary : inWeek ? Theme.alpha(Theme.primary, .14) : "transparent"
                MText { anchors.centerIn: parent; text: day >= 1 && day <= 31 ? day : ""; role: "labelS"; font.weight: day === 8 ? Font.Bold : Font.Medium; color: day === 8 ? Theme.onPrimary : Theme.text } } } }
}
