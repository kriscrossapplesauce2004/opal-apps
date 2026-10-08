import QtQuick
import QtQuick.Layouts
import Opal.Material
import Opal.Terminal
import Opal.TerminalUi
// One terminal pane: the session's grid, shell-integration marks in the gutter, a result pill for the hovered command,
// a chip with the running program, the focus outline, an optional cursor trail and an optional post-process shader.
Item {
    id: pane
    property TerminalSession session
    property bool focused: false
    property bool showChrome: true
    property color background: Theme.surfaceLowest
    property alias term: term
    signal clicked()
    readonly property real gutter: TerminalConfig.commandMarks ? 10 : 0

    Rectangle {
        id: backdrop
        anchors.fill: parent
        radius: 20
        color: Qt.rgba(pane.background.r, pane.background.g, pane.background.b, TerminalConfig.opacity)
    }
    // The grid and its overlays render into one layer so a shader can post-process all of it.
    Item {
        id: content
        anchors.fill: parent
        layer.enabled: TerminalConfig.shader !== "none" && TerminalConfig.shader !== ""
        layer.effect: ShaderEffect {
            property vector2d size: Qt.vector2d(width, height)
            property real time: 0
            property color glow: Theme.primary
            NumberAnimation on time { from: 0; to: 6.2832; duration: 4000; loops: Animation.Infinite; running: TerminalConfig.shader === "edge-glow" && !Theme.reduceMotion }
            fragmentShader: TerminalConfig.shader === "crt" ? "qrc:/opal/terminal-ui/shaders/crt.frag.qsb" : TerminalConfig.shader === "bloom" ? "qrc:/opal/terminal-ui/shaders/bloom.frag.qsb"
                          : TerminalConfig.shader === "edge-glow" ? "qrc:/opal/terminal-ui/shaders/edgeglow.frag.qsb" : TerminalConfig.shader.endsWith(".qsb") ? "file://" + TerminalConfig.shader : ""
        }
        TerminalItem {
            id: term
            anchors.fill: parent
            anchors.leftMargin: pane.gutter
            session: pane.session
            drawBackground: false
            fontFamily: TerminalConfig.fontFamily
            fontPixelSize: TerminalConfig.fontSize
            lineHeightFactor: TerminalConfig.lineHeight
            padding: TerminalConfig.padding
            cursorStyle: TerminalConfig.cursorStyle === "bar" ? TerminalItem.CursorBar : TerminalConfig.cursorStyle === "underline" ? TerminalItem.CursorUnderline : TerminalItem.CursorBlock
            cursorBlink: TerminalConfig.cursorBlink
            appShortcuts: ["Ctrl+Shift+T", "Ctrl+Shift+W", "Ctrl+Shift+D", "Ctrl+Shift+E", "Ctrl+Shift+P", "Ctrl+Shift+S", "Ctrl+,", "Ctrl+Shift+N",
                           "Ctrl+Tab", "Ctrl+Shift+Tab", "Alt+Left", "Alt+Right", "Alt+Up", "Alt+Down", "Ctrl+=", "Ctrl+-", "Ctrl+0", "Ctrl+Shift+Return", "F11"]
            onActiveFocusChanged: if (activeFocus) pane.clicked()
            Accessible.role: Accessible.Terminal
            Accessible.name: pane.session ? pane.session.title : qsTr("Terminal")
        }
        // Cursor trail: fading copies at the last cursor positions.
        Item {
            id: trail
            anchors.fill: term
            visible: TerminalConfig.cursorTrail && pane.focused && pane.session && pane.session.viewOffset === 0
            property var points: []
            readonly property point cur: pane.session ? Qt.point(term.padding + pane.session.cursorColumn * term.cellWidth, term.padding + pane.session.cursorRow * term.cellHeight) : Qt.point(0, 0)
            onCurChanged: { const p = points.slice(); p.unshift(cur); points = p.slice(0, 6); fade.restart(); }
            Timer { id: fade; interval: 140; onTriggered: trail.points = trail.points.slice(0, 1) }
            Repeater {
                model: trail.points.slice(1)
                Rectangle {
                    required property var modelData
                    required property int index
                    x: modelData.x; y: modelData.y + 2; width: term.cellWidth; height: term.cellHeight - 4; radius: 3
                    color: pane.session && pane.session.palette ? pane.session.palette.cursor : Theme.primary
                    opacity: .45 - index * .08
                    scale: 1 - index * .12
                }
            }
        }
    }
    // Gutter marks from OSC 133: green done, red failed, amber running.
    Item {
        anchors.left: parent.left; anchors.top: parent.top; anchors.bottom: parent.bottom; width: 12
        visible: TerminalConfig.commandMarks
        clip: true
        Repeater {
            model: pane.session ? pane.session.marks : null
            Rectangle {
                id: mark
                required property int line
                required property string state
                required property int exitCode
                required property int durationMs
                required property string commandText
                readonly property real yy: (term.contentY, pane.session.viewOffset, pane.session.scrollbackCount, term.yForLine(line))
                visible: yy >= 0 && yy < pane.height - term.cellHeight
                x: 4; y: yy + 3; width: 4; height: term.cellHeight - 6; radius: 2
                color: state === "fail" ? Theme.error : state === "running" ? Theme.tertiary : Theme.primary
                HoverHandler { id: mh; onHoveredChanged: if (hovered) { resultPill.mark = mark; resultPill.show = true; } }
            }
        }
    }
    // Result pill for the hovered command.
    Rectangle {
        id: resultPill
        property var mark: null
        property bool show: false
        visible: show && mark !== null && mark.visible
        anchors.right: parent.right; anchors.rightMargin: 16
        y: mark ? Math.max(4, mark.yy - 6) : 0
        implicitHeight: 40; implicitWidth: rp.implicitWidth + 16; radius: 20
        color: Theme.inverseSurface
        z: 5
        HoverHandler { onHoveredChanged: if (!hovered) hideTimer.restart(); else hideTimer.stop() }
        Timer { id: hideTimer; interval: 1200; onTriggered: resultPill.show = false }
        onShowChanged: if (show) hideTimer.restart()
        RowLayout { id: rp; anchors.centerIn: parent; spacing: 8
            Rectangle { implicitWidth: 28; implicitHeight: 28; radius: 14; color: resultPill.mark && resultPill.mark.state === "fail" ? Theme.error : resultPill.mark && resultPill.mark.state === "running" ? Theme.tertiary : Theme.primary
                Glyph { anchors.centerIn: parent; name: !resultPill.mark ? "check" : resultPill.mark.state === "fail" ? "close" : resultPill.mark.state === "running" ? "more_horiz" : "check"; font.pixelSize: 17; color: Theme.onPrimary } }
            MText { text: resultPill.mark ? resultPill.mark.commandText : ""; role: "label"; font.family: TerminalConfig.fontFamily; color: Theme.inverseInk; Layout.maximumWidth: 320; elide: Text.ElideRight }
            MText { text: !resultPill.mark ? "" : (resultPill.mark.state === "running" ? qsTr("running") : qsTr("exit %1").arg(resultPill.mark.exitCode)) + (resultPill.mark && resultPill.mark.durationMs > 0 ? " · " + (resultPill.mark.durationMs / 1000).toFixed(1) + " s" : ""); role: "labelS"; color: Theme.alpha(Theme.inverseInk, .7) }
            Rectangle { implicitWidth: 1; implicitHeight: 20; color: Theme.alpha(Theme.inverseInk, .25) }
            Repeater { model: [{i: "content_copy", t: qsTr("Copy command")}, {i: "replay", t: qsTr("Run again")}, {i: "bookmark_add", t: qsTr("Save as snippet")}]
                Glyph { required property var modelData; name: modelData.i; font.pixelSize: 18; color: Theme.inverseInk
                    TapHandler { onTapped: {
                        const cmd = resultPill.mark.commandText;
                        if (modelData.i === "content_copy") TerminalHelpers.copyText(cmd);
                        else if (modelData.i === "replay") pane.session.sendText(cmd + "\r");
                        else TerminalConfig.set("snippets", cmd.length > 40 ? cmd.slice(0, 40) + "…" : cmd, cmd);
                        resultPill.show = false; } }
                    Accessible.role: Accessible.Button; Accessible.name: modelData.t } } }
    }
    // Chip with the running program and the folder, from the foreground process and OSC 7.
    // It shows while the pane is hovered and for a few seconds after the program changes, so it never hides prompt text for long.
    HoverHandler { id: paneHover }
    Timer { id: chipTimer; interval: 3500 }
    Connections { target: pane.session; function onForegroundProcessChanged() { chipTimer.restart(); } }
    Rectangle {
        visible: opacity > 0
        opacity: pane.showChrome && pane.session && (pane.session.foregroundProcess !== "" || pane.session.cwd !== "") && (paneHover.hovered || chipTimer.running) ? .95 : 0
        Behavior on opacity { NumberAnimation { duration: Theme.motion(Theme.durMedium) } }
        anchors.right: parent.right; anchors.top: parent.top; anchors.margins: 10
        implicitHeight: 28; implicitWidth: pc.implicitWidth + 20; radius: 14; color: Theme.surfaceHigh
        RowLayout { id: pc; anchors.centerIn: parent; spacing: 6
            Glyph { name: pane.session && /n?vim|nano|hx|helix|emacs|micro/.test(pane.session.foregroundProcess) ? "edit_note" : pane.session && /ssh/.test(pane.session.foregroundProcess) ? "dns" : "terminal"; font.pixelSize: 15; color: Theme.tertiary }
            MText { text: pane.session ? [pane.session.foregroundProcess, pane.session.cwd.replace(TerminalHelpers.homePath, "~").split("/").slice(-1)[0] || "~"].filter(s => s).join(" · ") : ""; role: "labelS" } }
    }
    // Focus outline.
    Rectangle { anchors.fill: parent; radius: 20; color: "transparent"; border.width: 2; border.color: Theme.alpha(Theme.primary, .5); visible: pane.focused && pane.showChrome }
    TapHandler { onTapped: { term.forceActiveFocus(); pane.clicked(); } }
}
