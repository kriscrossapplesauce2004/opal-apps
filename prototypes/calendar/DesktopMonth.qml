import QtQuick
import QtQuick.Layouts
import Opal.Material
import "Fixtures.js" as F
// Calendar direction B, "Month": a month of event chips, the chosen day's agenda in a side panel.
Rectangle {
    color: Theme.base
    RowLayout {
        anchors.fill: parent; anchors.margins: 12; spacing: 12
        ColumnLayout { Layout.fillWidth: true; Layout.fillHeight: true; spacing: 10
            RowLayout { Layout.fillWidth: true; spacing: 10
                MText { text: "October 2026"; role: "displayS" }
                Item { Layout.fillWidth: true }
                SegmentedButtons { current: "month"; options: [{id: "week", name: "Week"}, {id: "month", name: "Month"}, {id: "agenda", name: "Agenda"}] }
                Fab { icon: "add"; size: "small" } }
            Rectangle { Layout.fillWidth: true; Layout.fillHeight: true; radius: 28; color: Theme.surfaceLow
                GridLayout { anchors.fill: parent; anchors.margins: 10; columns: 7; rowSpacing: 4; columnSpacing: 4
                    Repeater { model: F.days; MText { required property string modelData; Layout.fillWidth: true; horizontalAlignment: Text.AlignHCenter; text: modelData; role: "labelS"; color: Theme.subtext } }
                    Repeater { model: 35
                        Rectangle { required property int index; readonly property int day: index - 2; readonly property int wd: index % 7
                            Layout.fillWidth: true; Layout.fillHeight: true; radius: 16
                            color: day === 8 ? Theme.secondaryContainer : day >= 1 && day <= 31 ? Theme.surface : "transparent"
                            MText { x: 10; y: 6; text: day >= 1 && day <= 31 ? day : ""; role: "label"; font.weight: day === 8 ? Font.Bold : Font.Medium; color: day === 8 ? Theme.onSecondaryContainer : Theme.text }
                            Column { x: 6; y: 28; width: parent.width - 12; spacing: 3
                                Repeater { model: day >= 5 && day <= 11 ? F.events.filter(e => e.d === day - 5).slice(0, 3) : (day === 15 || day === 22 ? [{t: "Stand-up", cal: "work"}] : day === 20 ? [{t: "Signals exam", cal: "uni"}] : [])
                                    Rectangle { required property var modelData; width: parent.width; height: 20; radius: 10; color: Theme.alpha(F.cal(modelData.cal).c, .3)
                                        MText { anchors.verticalCenter: parent.verticalCenter; x: 8; width: parent.width - 12; text: modelData.t; role: "labelS"; elide: Text.ElideRight } } } } } } } } }
        Rectangle { Layout.preferredWidth: 340; Layout.fillHeight: true; radius: 28; color: Theme.surface
            ColumnLayout { anchors.fill: parent; anchors.margins: 20; spacing: 12
                MText { text: "Thursday"; role: "label"; color: Theme.primary }
                MText { text: "8 October"; role: "headline" }
                Rectangle { Layout.fillWidth: true; implicitHeight: 44; radius: 22; color: Theme.alpha(F.cal("birthdays").c, .25)
                    RowLayout { anchors.fill: parent; anchors.leftMargin: 14; spacing: 10; Glyph { name: "cake"; color: F.cal("birthdays").c } MText { text: "Jordan's birthday"; role: "label"; font.weight: Font.DemiBold } } }
                Repeater { model: F.events.filter(e => e.d === 3)
                    RowLayout { required property var modelData; Layout.fillWidth: true; Layout.preferredHeight: 52; spacing: 14
                        ColumnLayout { spacing: 0; Layout.preferredWidth: 52; MText { text: F.fmt(modelData.s); role: "label" } MText { text: F.fmt(modelData.e); role: "labelS"; color: Theme.subtext } }
                        Rectangle { implicitWidth: 4; implicitHeight: 44; radius: 2; color: F.cal(modelData.cal).c }
                        ColumnLayout { Layout.fillWidth: true; spacing: 1; MText { text: modelData.t; role: "titleS" } MText { visible: !!modelData.where; text: modelData.where || ""; role: "labelS"; color: Theme.subtext } } } }
                Item { Layout.fillHeight: true } } }
    }
}
