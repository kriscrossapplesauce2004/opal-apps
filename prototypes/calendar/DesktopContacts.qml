import QtQuick
import QtQuick.Layouts
import Opal.Material
import "Fixtures.js" as F
// Contacts inside Calendar: list with letter groups and a detail card with upcoming events together.
Rectangle {
    color: Theme.base
    RowLayout {
        anchors.fill: parent; spacing: 0
        NavRail { Layout.fillHeight: true; Layout.preferredWidth: 96; currentIndex: 1; fabIcon: "person_add"
            destinations: [{title: "Calendar", icon: "calendar_month"}, {title: "Contacts", icon: "contacts"}, {title: "Tasks", icon: "task_alt"}] }
        Rectangle { Layout.preferredWidth: 360; Layout.fillHeight: true; Layout.margins: 12; Layout.leftMargin: 0; radius: 28; color: Theme.surfaceLow
            ColumnLayout { anchors.fill: parent; anchors.margins: 12; spacing: 4
                SearchField { Layout.fillWidth: true; placeholder: "Search contacts" }
                Repeater { model: F.contacts
                    ListRow { required property var modelData; required property int index; Layout.fillWidth: true; title: modelData.n; subtitle: modelData.p; selected: index === 1; trailingIcon: modelData.bday ? "cake" : ""
                        Rectangle { width: 40; height: 40; radius: 20; color: modelData.c; MText { anchors.centerIn: parent; text: modelData.n[0]; role: "title"; color: "#1a1a1a" } } } }
                Item { Layout.fillHeight: true } } }
        Rectangle { Layout.fillWidth: true; Layout.fillHeight: true; Layout.margins: 12; Layout.leftMargin: 0; radius: 28; color: Theme.surface
            ColumnLayout { anchors.fill: parent; anchors.margins: 32; spacing: 18
                RowLayout { spacing: 20
                    Rectangle { implicitWidth: 112; implicitHeight: 112; radius: 40; color: F.contacts[1].c; MText { anchors.centerIn: parent; text: "J"; font.pixelSize: 54; font.weight: Font.Bold; color: "#1a1a1a" } }
                    ColumnLayout { spacing: 4; MText { text: "Jordan Lee"; role: "displayS" } MText { text: "Birthday today · 8 October"; role: "body"; color: Theme.primary } } }
                SplitPillRow {}
                Rectangle { Layout.fillWidth: true; implicitHeight: 150; radius: 24; color: Theme.surfaceHigh
                    ColumnLayout { anchors.fill: parent; anchors.margins: 18; spacing: 10
                        Repeater { model: [["call", F.contacts[1].p, "Mobile"], ["mail", F.contacts[1].e, "Personal"], ["cake", "8 October", "Birthday"]]
                            RowLayout { required property var modelData; spacing: 16; Glyph { name: modelData[0]; color: Theme.subtext } ColumnLayout { spacing: 0; MText { text: modelData[1]; role: "titleS" } MText { text: modelData[2]; role: "labelS"; color: Theme.subtext } } } } } }
                MText { text: "Together"; role: "label"; color: Theme.primary }
                Repeater { model: [{t: "Game night", w: "Wed 7 Oct, 18:00"}, {t: "Weekend away", w: "Fri 9 – Sun 11 Oct"}]
                    RowLayout { required property var modelData; spacing: 14; Rectangle { implicitWidth: 4; implicitHeight: 36; radius: 2; color: F.cal("personal").c } ColumnLayout { spacing: 0; MText { text: modelData.t; role: "titleS" } MText { text: modelData.w; role: "labelS"; color: Theme.subtext } } } }
                Item { Layout.fillHeight: true } } }
    }
    component SplitPillRow: Row { spacing: 2
        Repeater { model: [{i: "call", t: "Call"}, {i: "chat", t: "Message"}, {i: "mail", t: "Email"}, {i: "event", t: "Invite"}]
            Rectangle { required property var modelData; required property int index; height: 44; width: pr.implicitWidth + 32
                topLeftRadius: index === 0 ? 22 : 6; bottomLeftRadius: topLeftRadius; topRightRadius: index === 3 ? 22 : 6; bottomRightRadius: topRightRadius; color: Theme.surfaceHigh
                RowLayout { id: pr; anchors.centerIn: parent; spacing: 6; Glyph { name: modelData.i; font.pixelSize: 18; color: Theme.primary } MText { text: modelData.t; role: "label" } } } } }
}
