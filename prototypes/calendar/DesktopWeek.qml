import QtQuick
import QtQuick.Layouts
import Opal.Material
import "Fixtures.js" as F
// Calendar direction A, "Week": mini month and calendars on the left, a week grid with tonal event blocks and a now line.
Rectangle {
    color: Theme.base
    RowLayout {
        anchors.fill: parent; spacing: 0
        ColumnLayout { Layout.preferredWidth: 270; Layout.maximumWidth: 270; Layout.fillHeight: true; Layout.margins: 16; spacing: 18
            Fab { icon: "add"; text: "New event"; size: "small" }
            MiniMonth {}
            MText { text: "Calendars"; role: "label"; color: Theme.subtext }
            Repeater { model: F.cals
                RowLayout { required property var modelData; spacing: 12; Layout.fillWidth: true
                    Rectangle { implicitWidth: 18; implicitHeight: 18; radius: 6; color: modelData.c; Glyph { anchors.centerIn: parent; name: "check"; font.pixelSize: 14; color: "#1a1a1a" } }
                    MText { text: modelData.name; role: "body"; Layout.fillWidth: true } } }
            Item { Layout.fillHeight: true }
            Rectangle { Layout.fillWidth: true; implicitHeight: 56; radius: 18; color: Theme.surface
                RowLayout { anchors.fill: parent; anchors.margins: 12; spacing: 10; Glyph { name: "cloud_sync"; color: Theme.primary } ColumnLayout { spacing: 0; MText { text: "nas.local · CalDAV"; role: "label" } MText { text: "Synced 2 min ago"; role: "labelS"; color: Theme.subtext } } } } }
        ColumnLayout { Layout.fillWidth: true; Layout.fillHeight: true; Layout.topMargin: 14; Layout.rightMargin: 12; Layout.bottomMargin: 12; spacing: 10
            RowLayout { Layout.fillWidth: true; spacing: 10
                MText { text: "5 – 11 Oct"; role: "headline" }
                MButton { icon: "chevron_left"; compact: true } MButton { icon: "chevron_right"; compact: true }
                MButton { text: "Today"; contained: true }
                Item { Layout.fillWidth: true }
                SearchField { Layout.preferredWidth: 240; implicitHeight: 40; placeholder: "Search events" }
                SegmentedButtons { current: "week"; options: [{id: "day", name: "Day"}, {id: "week", name: "Week"}, {id: "month", name: "Month"}, {id: "agenda", name: "Agenda"}] } }
            Rectangle {
                id: grid; Layout.fillWidth: true; Layout.fillHeight: true; radius: 28; color: Theme.surfaceLow; clip: true
                readonly property real gutter: 56
                readonly property real colW: (width - gutter - 8) / 7
                readonly property real topY: 76
                readonly property real hourH: (height - topY - 8) / 13
                function yAt(h) { return topY + (h - 8) * hourH; }
                // Day headers and all-day row.
                Repeater { model: 7
                    Column { required property int index; x: grid.gutter + index * grid.colW; y: 8; width: grid.colW; spacing: 2
                        MText { anchors.horizontalCenter: parent.horizontalCenter; text: F.days[index]; role: "labelS"; color: index === F.today ? Theme.primary : Theme.subtext }
                        Rectangle { anchors.horizontalCenter: parent.horizontalCenter; width: 34; height: 34; radius: 17; color: index === F.today ? Theme.primary : "transparent"
                            MText { anchors.centerIn: parent; text: 5 + index; role: "title"; color: index === F.today ? Theme.onPrimary : Theme.text } } } }
                Repeater { model: F.allDay
                    Rectangle { required property var modelData; x: grid.gutter + modelData.d * grid.colW + 3; y: grid.topY - 22; width: ((modelData.d2 || modelData.d) - modelData.d + 1) * grid.colW - 6; height: 18; radius: 9; color: Theme.alpha(F.cal(modelData.cal).c, .3)
                        MText { anchors.verticalCenter: parent.verticalCenter; x: 8; text: modelData.t; role: "labelS"; font.weight: Font.DemiBold } } }
                Repeater { model: 13
                    Item { required property int index; y: grid.yAt(8 + index); width: grid.width; height: 1
                        Rectangle { x: grid.gutter; width: parent.width - grid.gutter - 8; height: 1; color: Theme.outline }
                        MText { x: 8; y: -7; text: (8 + index) + ":00"; role: "labelS"; color: Theme.muted } } }
                Repeater { model: F.events
                    Rectangle { required property var modelData; readonly property var cl: F.cal(modelData.cal)
                        x: grid.gutter + modelData.d * grid.colW + 3; y: grid.yAt(modelData.s) + 2; width: grid.colW - 6; height: (modelData.e - modelData.s) * grid.hourH - 4
                        radius: 12; color: Theme.mix(Theme.surfaceHigh, cl.c, Theme.light ? .45 : .32)
                        Rectangle { width: 4; height: parent.height - 12; x: 5; y: 6; radius: 2; color: cl.c }
                        Column { x: 14; y: 6; width: parent.width - 20; spacing: 1
                            MText { width: parent.width; text: modelData.t; role: "label"; font.weight: Font.DemiBold; elide: Text.ElideRight }
                            MText { width: parent.width; visible: parent.parent.height > 40; text: F.fmt(modelData.s) + (modelData.where ? " · " + modelData.where : ""); role: "labelS"; color: Theme.subtext } } } }
                // Now line.
                Rectangle { x: grid.gutter + F.today * grid.colW - 5; y: grid.yAt(15.4) - 5; width: 10; height: 10; radius: 5; color: Theme.error }
                Rectangle { x: grid.gutter + F.today * grid.colW; y: grid.yAt(15.4) - 1; width: grid.colW; height: 2; color: Theme.error }
            }
        }
    }
}
