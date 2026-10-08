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
    function tint(hex) { return Theme.mix(hex, Theme.primary, .06); }
    readonly property var ansi: theme ? theme : ({
        fg: Theme.text, dim: Theme.subtext, red: tint(Theme.light ? "#c5221f" : "#f28b82"), green: tint(Theme.light ? "#188038" : "#81c995"),
        yellow: tint(Theme.light ? "#b06000" : "#fdd663"), blue: tint(Theme.light ? "#1967d2" : "#8ab4f8"), magenta: tint(Theme.light ? "#9334e6" : "#c58af9"),
        cyan: tint(Theme.light ? "#007b83" : "#78d9ec"), cursor: Theme.primary
    })
    function esc(s) { return s.replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;").replace(/ /g, "&nbsp;"); }
    function html(spans) { let h = ""; for (const sp of spans) { const t = "<font color=\"" + ansi[sp[1]] + "\">" + esc(sp[0]) + "</font>"; h += sp[0].indexOf("❯") === 0 ? "<b>" + t + "</b>" : t; } return h || "&nbsp;"; }
    Column {
        id: col
        x: term.padding + (Object.keys(term.marks).length ? 10 : 0); y: term.padding
        Repeater {
            id: rows
            model: term.lines
            Item {
                required property var modelData
                required property int index
                width: Math.max(1, lineText.implicitWidth); height: term.lineHeight
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
        readonly property Item lastRow: rows.count > 0 ? rows.itemAt(rows.count - 1) : null
        x: col.x + (lastRow && lastRow.width > 1 ? lastRow.width : 0) + 1
        y: col.y + (lastRow ? lastRow.y : 0)
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
