import QtQuick
import Opal.Material
// One terminal pane drawn from fixture spans. The ANSI palette follows the Opal colours unless a theme gives its own.
Rectangle {
    id: term
    property var lines: []
    property string fontFamily: "CaskaydiaCove Nerd Font Mono"
    property int fontSize: 14
    property real padding: 14
    property bool cursor: true
    property string cursorStyle: "block"
    property bool active: true
    property var theme: null
    property real lineHeight: Math.round(fontSize * 1.42)
    // Command marks from shell integration: line index to "ok", "fail" or "run".
    property var marks: ({})
    // A few fading copies behind the cursor show the cursor trail.
    property bool trail: false
    radius: 20
    color: theme ? theme.bg : Theme.surfaceLowest
    function tint(hex) { return Theme.mix(hex, Theme.primary, Theme.light ? .1 : .14); }
    readonly property var ansi: theme ? theme : ({
        fg: Theme.text, dim: Theme.subtext, red: tint(Theme.light ? "#b3261e" : "#f2a59d"), green: tint(Theme.light ? "#2e7d32" : "#a8d8a0"),
        yellow: tint(Theme.light ? "#8a6d00" : "#ecd38a"), blue: tint(Theme.light ? "#1a5fb4" : "#a8c8ff"), magenta: tint(Theme.light ? "#8e3fa8" : "#e2b6f2"),
        cyan: tint(Theme.light ? "#00727a" : "#95d8d4"), cursor: Theme.primary
    })
    function esc(s) { return s.replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;").replace(/ /g, "&nbsp;"); }
    function html(spans) { let h = ""; for (const sp of spans) h += "<font color=\"" + ansi[sp[1]] + "\">" + esc(sp[0]) + "</font>"; return h || "&nbsp;"; }
    Column {
        id: col
        x: term.padding + (Object.keys(term.marks).length ? 10 : 0); y: term.padding
        Repeater {
            model: term.lines
            Item {
                required property var modelData
                required property int index
                width: lineText.implicitWidth; height: term.lineHeight
                Text { id: lineText; anchors.verticalCenter: parent.verticalCenter; text: term.html(parent.modelData); textFormat: Text.StyledText; font.family: term.fontFamily; font.pixelSize: term.fontSize; color: term.ansi.fg }
                Rectangle {
                    readonly property string mark: term.marks[parent.index] || ""
                    visible: mark !== ""; x: -12; anchors.verticalCenter: parent.verticalCenter
                    width: 4; height: term.lineHeight - 6; radius: 2
                    color: mark === "fail" ? term.ansi.red : mark === "run" ? term.ansi.yellow : term.ansi.green
                }
            }
        }
    }
    // Cursor after the last line's text.
    Item {
        id: caret
        visible: term.cursor
        readonly property real cw: Math.round(term.fontSize * .6)
        x: col.x + (col.children.length > 1 ? col.children[col.children.length - 2].width : 0) + 1
        y: col.y + Math.max(0, term.lines.length - 1) * term.lineHeight
        Repeater {
            model: term.trail ? 4 : 0
            Rectangle { required property int index; x: -(index + 1) * caret.cw * 1.6; y: 3; width: caret.cw; height: term.lineHeight - 6; radius: 2; color: term.ansi.cursor; opacity: .32 - index * .07; scale: 1 - index * .12 }
        }
        Rectangle {
            y: term.cursorStyle === "underline" ? term.lineHeight - 4 : 3
            width: term.cursorStyle === "bar" ? 2 : caret.cw; height: term.cursorStyle === "underline" ? 2 : term.lineHeight - 6
            radius: 2; color: term.ansi.cursor; opacity: term.active ? 1 : 0
        }
        Rectangle { visible: !term.active; y: 3; width: caret.cw; height: term.lineHeight - 6; radius: 2; color: "transparent"; border.width: 1; border.color: term.ansi.cursor }
    }
    clip: true
}
