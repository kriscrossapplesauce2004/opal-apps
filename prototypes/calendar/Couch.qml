import QtQuick
import QtQuick.Layouts
import Opal.Material
import "Fixtures.js" as F
// Calendar on the couch: a glanceable today and the next days, readable from across the room.
AppFrame {
    destinations: [{title: "Today", icon: "today"}, {title: "Week", icon: "view_week"}, {title: "Month", icon: "calendar_month"}]
    hints: [{b: "A", t: "Open"}, {b: "B", t: "Back"}, {b: "Y", t: "Search"}]
    RowLayout { anchors.fill: parent; anchors.leftMargin: 72; anchors.rightMargin: 72; anchors.bottomMargin: 90; spacing: 40
        ColumnLayout { Layout.preferredWidth: 600; Layout.maximumWidth: 600; Layout.fillHeight: true; spacing: 18
            MText { text: "Thursday"; font.pixelSize: 30; color: Theme.primary }
            MText { text: "8 October"; font.pixelSize: 80; font.weight: Font.Black; font.variableAxes: ({"wdth": 120, "ROND": 80}) }
            Rectangle { Layout.fillWidth: true; implicitHeight: 72; radius: 36; color: Theme.alpha(F.cal("birthdays").c, .25)
                RowLayout { anchors.fill: parent; anchors.leftMargin: 24; spacing: 14; Glyph { name: "cake"; font.pixelSize: 34; color: F.cal("birthdays").c } MText { text: "Jordan's birthday"; font.pixelSize: 26; font.weight: Font.DemiBold } } }
            Item { Layout.fillHeight: true } }
        ColumnLayout { Layout.fillWidth: true; Layout.fillHeight: true; spacing: 16
            Repeater { model: F.events.filter(e => e.d === 3).concat(F.events.filter(e => e.d === 4))
                Rectangle { required property var modelData; required property int index; Layout.fillWidth: true; implicitHeight: 96; radius: 32; color: Theme.mix(Theme.surfaceHigh, F.cal(modelData.cal).c, .22); border.width: index === 1 ? 4 : 0; border.color: Theme.primary
                    RowLayout { anchors.fill: parent; anchors.leftMargin: 28; anchors.rightMargin: 28; spacing: 22
                        MText { text: F.fmt(modelData.s); font.pixelSize: 30; font.weight: Font.Bold; Layout.preferredWidth: 100 }
                        ColumnLayout { Layout.fillWidth: true; spacing: 2; MText { text: modelData.t; font.pixelSize: 28; font.weight: Font.DemiBold } MText { text: (modelData.d === 4 ? "Tomorrow" : "Today") + (modelData.where ? " · " + modelData.where : ""); font.pixelSize: 20; color: Theme.subtext } } } } }
            Item { Layout.fillHeight: true } } }
}
