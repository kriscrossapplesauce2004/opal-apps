import QtQuick
import QtQuick.Layouts
import Opal.Material
import "Fixtures.js" as F
// Text Editor direction C, "Focus": chrome folds away; a narrow column of prose, an outline rail you can hide, a word goal.
Rectangle {
    color: Theme.base
    RowLayout {
        anchors.fill: parent; spacing: 0
        ColumnLayout { Layout.preferredWidth: 260; Layout.maximumWidth: 260; Layout.fillHeight: true; Layout.margins: 20; spacing: 4
            MText { text: "Outline"; role: "label"; color: Theme.primary; Layout.bottomMargin: 6 }
            Repeater { model: F.md.filter(l => /^#{1,3} /.test(l))
                MText { required property string modelData; required property int index; Layout.fillWidth: true; text: modelData.replace(/^#+ /, ""); role: "label"; font.pixelSize: 13
                    Layout.leftMargin: ((modelData.match(/^#+/) || [""])[0].length - 1) * 14; color: index === 3 ? Theme.primary : Theme.subtext; font.weight: index === 3 ? Font.Bold : Font.Medium } }
            Item { Layout.fillHeight: true }
            Rectangle { Layout.fillWidth: true; implicitHeight: 84; radius: 20; color: Theme.surface
                ColumnLayout { anchors.fill: parent; anchors.margins: 14; spacing: 8
                    RowLayout { MText { text: "Today's goal"; role: "label"; Layout.fillWidth: true } MText { text: "1,284 / 1,500"; role: "labelS"; color: Theme.subtext } }
                    WavyProgress { Layout.fillWidth: true; value: .86; animate: false; amplitude: 2; wavelength: 16; thickness: 3 } } } }
        Item { Layout.fillWidth: true; Layout.fillHeight: true
            Flickable { anchors.centerIn: parent; width: 680; height: parent.height - 80; contentHeight: prose.height; clip: true
                ColumnLayout { id: prose; width: parent.width; spacing: 14
                    Repeater { model: F.md.slice(0, 30).filter(l => l.trim() !== "" && !/^\|/.test(l))
                        MText { required property string modelData; Layout.fillWidth: true; wrapMode: Text.WordWrap; textFormat: Text.StyledText
                            readonly property int level: (modelData.match(/^#+/) || [""])[0].length
                            text: modelData.replace(/^#+ /, "").replace(/\*\*([^*]+)\*\*/g, "<b>$1</b>").replace(/`([^`]+)`/g, "<font color=\"" + Theme.tertiary + "\">$1</font>").replace(/^- /, "•  ")
                            font.pixelSize: level === 1 ? 40 : level === 2 ? 26 : 18; font.weight: level ? Font.Bold : Font.Normal; font.variableAxes: level ? ({"wdth": 110, "ROND": 60}) : ({}); color: level ? Theme.text : Theme.mix(Theme.text, Theme.subtext, .3); lineHeight: 1.35 } } } }
            Rectangle { anchors.top: parent.top; anchors.right: parent.right; anchors.margins: 16; implicitHeight: 40; implicitWidth: fr.implicitWidth + 20; radius: 20; color: Theme.surfaceHigh
                RowLayout { id: fr; anchors.centerIn: parent; spacing: 4; Glyph { name: "dock_to_right"; font.pixelSize: 18; color: Theme.subtext } MText { text: "SPEC.md · saved"; role: "labelS"; color: Theme.subtext } } } }
    }
}
