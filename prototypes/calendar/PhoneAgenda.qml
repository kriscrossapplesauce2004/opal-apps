import QtQuick
import QtQuick.Layouts
import Opal.Material
import "Fixtures.js" as F
// Calendar on a phone: a week strip that pulls down into a month, the agenda below, and the new-event sheet half open.
AppFrame {
    destinations: [{title: "Calendar", icon: "calendar_month"}, {title: "Contacts", icon: "contacts"}, {title: "Tasks", icon: "task_alt"}]
    ColumnLayout {
        anchors.fill: parent; spacing: 10
        RowLayout { Layout.fillWidth: true; Layout.margins: 16; Layout.bottomMargin: 0
            MText { text: "October"; role: "headline"; Layout.fillWidth: true }
            MButton { icon: "search"; compact: true } MButton { icon: "today"; compact: true } }
        RowLayout { Layout.fillWidth: true; Layout.leftMargin: 10; Layout.rightMargin: 10; spacing: 2
            Repeater { model: 7
                Rectangle { required property int index; Layout.fillWidth: true; implicitHeight: 64; radius: 20; color: index === F.today ? Theme.primary : "transparent"
                    Column { anchors.centerIn: parent; spacing: 2
                        MText { anchors.horizontalCenter: parent.horizontalCenter; text: F.days[index][0]; role: "labelS"; color: index === F.today ? Theme.onPrimary : Theme.subtext }
                        MText { anchors.horizontalCenter: parent.horizontalCenter; text: 5 + index; role: "title"; color: index === F.today ? Theme.onPrimary : Theme.text }
                        Row { anchors.horizontalCenter: parent.horizontalCenter; spacing: 2; Repeater { model: Math.min(3, F.events.filter(e => e.d === index).length); Rectangle { width: 4; height: 4; radius: 2; color: index === F.today ? Theme.onPrimary : Theme.primary } } } } } } }
        Rectangle { Layout.alignment: Qt.AlignHCenter; implicitWidth: 32; implicitHeight: 4; radius: 2; color: Theme.alpha(Theme.subtext, .4) }
        ColumnLayout { Layout.fillWidth: true; Layout.leftMargin: 16; Layout.rightMargin: 16; spacing: 8
            MText { text: "Today · Thursday 8"; role: "label"; color: Theme.primary }
            Rectangle { Layout.fillWidth: true; implicitHeight: 40; radius: 20; color: Theme.alpha(F.cal("birthdays").c, .25)
                RowLayout { anchors.fill: parent; anchors.leftMargin: 14; spacing: 10; Glyph { name: "cake"; font.pixelSize: 18; color: F.cal("birthdays").c } MText { text: "Jordan's birthday"; role: "label"; font.weight: Font.DemiBold } } }
            Repeater { model: F.events.filter(e => e.d === 3)
                Rectangle { required property var modelData; Layout.fillWidth: true; implicitHeight: 64; radius: 20; color: Theme.mix(Theme.surfaceHigh, F.cal(modelData.cal).c, .22)
                    RowLayout { anchors.fill: parent; anchors.leftMargin: 14; anchors.rightMargin: 14; spacing: 12
                        Rectangle { implicitWidth: 4; implicitHeight: 40; radius: 2; color: F.cal(modelData.cal).c }
                        ColumnLayout { Layout.fillWidth: true; spacing: 1; MText { text: modelData.t; role: "titleS" } MText { text: F.fmt(modelData.s) + " – " + F.fmt(modelData.e) + (modelData.where ? " · " + modelData.where : ""); role: "labelS"; color: Theme.subtext } } } } }
            MText { text: "Tomorrow · Friday 9"; role: "label"; color: Theme.subtext; Layout.topMargin: 6 } }
        Item { Layout.fillHeight: true }
    }
    // New event sheet, half open.
    Rectangle { anchors.left: parent.left; anchors.right: parent.right; anchors.bottom: parent.bottom; height: 300; topLeftRadius: 28; topRightRadius: 28; color: Theme.surfaceLow
        Rectangle { anchors.horizontalCenter: parent.horizontalCenter; y: 10; width: 32; height: 4; radius: 2; color: Theme.alpha(Theme.subtext, .5) }
        ColumnLayout { anchors.fill: parent; anchors.margins: 18; anchors.topMargin: 26; spacing: 12
            MText { text: "Coffee with Riley"; font.pixelSize: 24; font.weight: Font.Bold }
            Flow { Layout.fillWidth: true; spacing: 8
                Chip { text: "Fri 9 Oct"; icon: "event"; selected: true }
                Chip { text: "10:30 – 11:00"; icon: "schedule"; glass: false }
                Chip { text: "Personal"; icon: "circle"; glass: false }
                Chip { text: "10 min before"; icon: "notifications"; glass: false }
                Chip { text: "Invite"; icon: "person_add"; glass: false } }
            Item { Layout.fillHeight: true }
            RowLayout { Layout.fillWidth: true; Item { Layout.fillWidth: true } MButton { text: "More options"; contained: true } MButton { text: "Save"; filled: true } } } }
}
