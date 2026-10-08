import QtQuick
import Opal.Material
import "Highlight.js" as H
// Editor surface: gutter with line numbers and change marks, highlighted text, current-line band and cursor.
Rectangle {
    id: v
    property var lines: []
    property string lang: "md"
    property int firstLine: 1
    property int currentLine: 12
    property int fontSize: 14
    property bool gutter: true
    property var changed: [8, 9, 14]
    property var findHits: []
    readonly property real lh: Math.round(fontSize * 1.6)
    radius: 24
    color: Theme.surfaceLowest
    clip: true
    readonly property var c: ({fg: Theme.text, kw: Theme.primary, type: Theme.tertiary, str: Theme.mix(Theme.secondary, "#9fe0a0", .4), num: Theme.mix(Theme.tertiary, "#f0b070", .4), comment: Theme.muted, member: Theme.mix(Theme.text, Theme.primary, .35), punct: Theme.subtext})
    Rectangle { x: 0; y: 14 + (v.currentLine - v.firstLine) * v.lh; width: parent.width; height: v.lh; color: Theme.alpha(Theme.primary, .07) }
    Column {
        x: 0; y: 14
        Repeater {
            model: v.lines
            Row {
                required property string modelData
                required property int index
                height: v.lh
                Item { width: v.gutter ? 64 : 22; height: v.lh
                    Rectangle { visible: v.changed.indexOf(index + v.firstLine) >= 0; x: 6; y: 3; width: 3; height: v.lh - 6; radius: 2; color: Theme.primary }
                    Text { visible: v.gutter; anchors.right: parent.right; anchors.rightMargin: 16; anchors.verticalCenter: parent.verticalCenter; text: index + v.firstLine; font.family: "CaskaydiaCove Nerd Font Mono"; font.pixelSize: v.fontSize - 2; color: index + v.firstLine === v.currentLine ? Theme.text : Theme.muted } }
                Text { anchors.verticalCenter: parent.verticalCenter; textFormat: Text.StyledText; font.family: "CaskaydiaCove Nerd Font Mono"; font.pixelSize: v.fontSize; color: Theme.text
                    text: v.lang === "cpp" ? H.cpp(modelData, v.c) : H.md(modelData, v.c) }
            }
        }
    }
}
