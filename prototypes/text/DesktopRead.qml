import QtQuick
import QtQuick.Effects
import QtQuick.Layouts
import Opal.Material
// Text Editor round 3, Read mode (the merged Documents app): page thumbnails, a paged PDF with a highlight, and a floating reading toolbar.
Rectangle {
    color: Theme.base
    readonly property string pdf: "file:///home/melissa/.local/state/opal/apps/artifacts/text/pdf/p-"
    ColumnLayout {
        anchors.fill: parent; anchors.margins: 8; spacing: 8
        RowLayout { Layout.fillWidth: true; spacing: 6
            SegmentedButtons { current: "read"; options: [{id: "write", name: "Write", icon: "edit_note"}, {id: "code", name: "Code", icon: "code"}, {id: "read", name: "Read", icon: "auto_stories"}] }
            Rectangle { implicitHeight: 36; implicitWidth: t1.implicitWidth + 26; radius: 18; color: Theme.surfaceHigh; Layout.leftMargin: 8
                RowLayout { id: t1; anchors.centerIn: parent; spacing: 8; Glyph { name: "picture_as_pdf"; font.pixelSize: 17; color: Theme.error } MText { text: "AU1388.pdf"; role: "label"; font.weight: Font.DemiBold } } }
            Rectangle { implicitHeight: 36; implicitWidth: t2.implicitWidth + 26; radius: 18; color: "transparent"
                RowLayout { id: t2; anchors.centerIn: parent; spacing: 8; Glyph { name: "menu_book"; font.pixelSize: 17; color: Theme.subtext } MText { text: "Frankenstein.epub"; role: "label"; color: Theme.subtext } } }
            Item { Layout.fillWidth: true }
            SearchField { Layout.preferredWidth: 260; implicitHeight: 40; placeholder: "Search in document" }
            MButton { icon: "more_vert"; compact: true } }
        RowLayout { Layout.fillWidth: true; Layout.fillHeight: true; spacing: 8
            Rectangle { Layout.preferredWidth: 170; Layout.fillHeight: true; radius: 24; color: Theme.surfaceLow
                Column { anchors.horizontalCenter: parent.horizontalCenter; y: 16; spacing: 14
                    Repeater { model: 3
                        Column { required property int index; spacing: 6
                            Rectangle { width: 118; height: 167; radius: 10; color: "#ffffff"; border.width: index === 0 ? 3 : 0; border.color: Theme.primary
                                Image { anchors.fill: parent; anchors.margins: 3; source: pdf + (index + 1) + ".png"; fillMode: Image.PreserveAspectFit; sourceSize: Qt.size(240, 340) } }
                            MText { anchors.horizontalCenter: parent.horizontalCenter; text: index + 1; role: "labelS"; color: index === 0 ? Theme.primary : Theme.subtext } } } } }
            Item { Layout.fillWidth: true; Layout.fillHeight: true
                Rectangle { id: page; anchors.horizontalCenter: parent.horizontalCenter; y: 10; height: parent.height - 20; width: height * 0.707; color: "#ffffff"; radius: 6
                    layer.enabled: true; layer.effect: MultiEffect { shadowEnabled: true; shadowBlur: .8; shadowOpacity: .5; shadowVerticalOffset: 6 }
                    Image { anchors.fill: parent; source: pdf + "1.png"; fillMode: Image.PreserveAspectFit; sourceSize: Qt.size(1100, 1560) }
                    // A highlight with a note pin.
                    Rectangle { x: parent.width * .1; y: parent.height * .225; width: parent.width * .42; height: parent.height * .024; radius: 3; color: Qt.rgba(1, .85, .2, .38) }
                    Rectangle { x: parent.width * .54; y: parent.height * .215; width: 30; height: 30; radius: 15; color: Theme.tertiary
                        Glyph { anchors.centerIn: parent; name: "sticky_note_2"; fill: 1; font.pixelSize: 16; color: Theme.base } } }
                // Reading toolbar.
                Rectangle { anchors.horizontalCenter: parent.horizontalCenter; anchors.bottom: parent.bottom; anchors.bottomMargin: 22; implicitHeight: 60; implicitWidth: rt.implicitWidth + 20; radius: 30; color: Theme.surfaceHigh
                    RowLayout { id: rt; anchors.centerIn: parent; spacing: 6
                        Glyph { name: "chevron_left"; color: Theme.text } MText { text: "1 / 3"; role: "label" } Glyph { name: "chevron_right"; color: Theme.text }
                        Rectangle { implicitWidth: 1; implicitHeight: 28; color: Theme.outline }
                        SegmentedButtons { current: "fitw"; options: [{id: "fitw", icon: "fit_width"}, {id: "fitp", icon: "fit_screen"}, {id: "two", icon: "auto_stories"}] }
                        Rectangle { implicitWidth: 1; implicitHeight: 28; color: Theme.outline }
                        Repeater { model: [{i: "ink_highlighter", on: true}, {i: "sticky_note_2"}, {i: "draw"}, {i: "dark_mode"}]
                            Rectangle { required property var modelData; implicitWidth: 44; implicitHeight: 40; radius: 20; color: modelData.on ? Theme.secondaryContainer : "transparent"
                                Glyph { anchors.centerIn: parent; name: modelData.i; fill: modelData.on ? 1 : 0; color: modelData.on ? Theme.onSecondaryContainer : Theme.text } } } } } }
            Rectangle { Layout.preferredWidth: 280; Layout.fillHeight: true; radius: 24; color: Theme.surfaceLow
                ColumnLayout { anchors.fill: parent; anchors.margins: 16; spacing: 10
                    SegmentedButtons { current: "notes"; options: [{id: "toc", name: "Outline"}, {id: "notes", name: "Notes · 1"}] }
                    Rectangle { Layout.fillWidth: true; implicitHeight: 110; radius: 18; color: Theme.surfaceHigh
                        ColumnLayout { anchors.fill: parent; anchors.margins: 12; spacing: 4
                            RowLayout { Rectangle { implicitWidth: 10; implicitHeight: 10; radius: 5; color: "#f5c84a" } MText { text: "Page 1 · highlight"; role: "labelS"; color: Theme.subtext } }
                            MText { Layout.fillWidth: true; text: "“FAKE AADHAAR CARD”"; role: "titleS"; wrapMode: Text.Wrap }
                            MText { Layout.fillWidth: true; text: "Check the follow-up answer in session 2."; role: "bodyS"; color: Theme.subtext; wrapMode: Text.Wrap } } }
                    Item { Layout.fillHeight: true }
                    MButton { text: "Export notes as Markdown"; icon: "edit_note"; tonal: true; Layout.fillWidth: true } } }
        }
    }
}
