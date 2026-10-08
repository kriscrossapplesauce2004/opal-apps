import QtQuick
import QtQuick.Layouts
import Opal.Material
// Rendered Markdown preview of the SPEC fixture, drawn with Opal type roles.
Rectangle {
    id: preview
    radius: 24; color: Theme.surfaceLow
    property var lines: []
    Flickable { anchors.fill: parent; anchors.margins: 28; contentHeight: col.height; clip: true
        ColumnLayout { id: col; width: parent.width; spacing: 10
            Repeater { model: preview.lines.filter(l => l.trim() !== "" && !/^\|/.test(l)).slice(0, 18)
                MText { required property string modelData; Layout.fillWidth: true; wrapMode: Text.WordWrap; textFormat: Text.StyledText
                    readonly property int level: (modelData.match(/^#+/) || [""])[0].length
                    text: modelData.replace(/^#+ /, "").replace(/^- /, "•  ").replace(/\*\*([^*]+)\*\*/g, "<b>$1</b>").replace(/`([^`]+)`/g, "<font color=\"" + Theme.tertiary + "\">$1</font>")
                    role: level === 1 ? "headline" : level === 2 ? "title" : level === 3 ? "titleS" : "body"
                    color: level ? Theme.text : Theme.subtext; Layout.topMargin: level === 2 ? 10 : 0 } } } }
}
