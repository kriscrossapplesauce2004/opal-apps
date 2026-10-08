import QtQuick
import QtQuick.Effects
import QtQuick.Layouts
import Opal.Material
import Opal.Terminal
import Opal.TerminalUi
// The whole Terminal interface: pill tabs, split panes, the pop-out sessions sidebar, the command palette and the
// Appearance panel. On narrow touch screens the panes become a pager with the extra key row below.
Item {
    id: root
    property var entries: ({})
    property int nextId: 1
    property var tabs: []
    property int currentTab: 0
    property int focusedPane: 0
    property bool sidebar: false
    property bool appearance: false
    property alias commandPalette: cmdPalette
    property int revision: 0
    readonly property bool phone: width < 600 && Posture.touch || width < 520
    readonly property var tab: tabs[currentTab] || null
    readonly property var themePalette: (TerminalConfig.revision, TerminalThemes.themes, TerminalConfig.theme === "opal" ? null : TerminalThemes.palette(TerminalConfig.theme))
    readonly property color paneBackground: themePalette && themePalette.background ? themePalette.background : Theme.surfaceLowest

    Component { id: sessionComponent; TerminalSession {} }
    Component { id: entryComponent; SessionEntry {} }

    function tint(hex) { return Theme.mix(hex, Theme.primary, .06); }
    function applyTheme(s) {
        const p = s.palette;
        if (themePalette && themePalette.ansi) {
            p.background = themePalette.background; p.foreground = themePalette.foreground; p.cursor = themePalette.cursor;
            p.cursorText = themePalette.background; p.selection = Theme.alpha(themePalette.selection, .7); p.ansi = themePalette.ansi;
            return;
        }
        p.background = Theme.surfaceLowest; p.foreground = Theme.text; p.cursor = Theme.primary; p.cursorText = Theme.primaryInk; p.selection = Theme.alpha(Theme.primary, .32);
        p.ansi = Theme.light
            ? ["#3c4043", "#c5221f", "#188038", "#b06000", "#1967d2", "#9334e6", "#007b83", "#5f6368", "#5f6368", "#d93025", "#1e8e3e", "#c26401", "#1a73e8", "#a142f4", "#12a4af", "#202124"].map(tint)
            : ["#2b2f38", "#f28b82", "#81c995", "#fdd663", "#8ab4f8", "#c58af9", "#78d9ec", "#d7dae0", "#5b6270", "#f6a8a1", "#a3dbb1", "#fee391", "#abc9fa", "#d6aefb", "#a2e6f4", "#ffffff"].map(tint);
    }
    function applyAll() { for (const k in entries) applyTheme(entries[k].session); }
    Connections { target: TerminalConfig; function onChanged() { root.applyAll(); for (const k in root.entries) root.entries[k].session.scrollbackLimit = TerminalConfig.scrollback; } }
    Connections { target: Palette; function onChanged() { root.applyAll(); } }
    onThemePaletteChanged: applyAll()

    function createSession(command, cwd) {
        const id = nextId++;
        const shell = TerminalConfig.shell || TerminalHelpers.userShell;
        const args = command ? ["-c", command + "; exec " + shell + " -i"] : [];
        const s = sessionComponent.createObject(root, {program: shell, arguments: args, workingDirectory: cwd || TerminalHelpers.startDirectory || TerminalHelpers.homePath, scrollbackLimit: TerminalConfig.scrollback});
        applyTheme(s);
        const e = entryComponent.createObject(root, {sid: id, session: s});
        s.childExited.connect(() => Qt.callLater(() => root.paneExited(id)));
        const m = Object.assign({}, entries); m[id] = e; entries = m;
        s.start();
        return id;
    }
    function sessionFor(id) { return entries[id] ? entries[id].session : null; }
    function leaves(node) { return !node ? [] : node.pane !== undefined ? [node.pane] : leaves(node.a).concat(leaves(node.b)); }
    function newTab(command, cwd) {
        const id = createSession(command, cwd || (focusedPane && sessionFor(focusedPane) ? decodeURIComponent(sessionFor(focusedPane).cwd.replace("file://", "")) : ""));
        const t = tabs.slice(); t.push({root: {pane: id}, focus: id}); tabs = t;
        currentTab = t.length - 1; focusedPane = id;
    }
    function replace(node, target, repl) {
        if (!node) return node;
        if (node.pane === target) return repl;
        if (node.pane !== undefined) return node;
        return {split: node.split, ratio: node.ratio, a: replace(node.a, target, repl), b: replace(node.b, target, repl)};
    }
    function remove(node, target) {
        if (!node || node.pane === target) return null;
        if (node.pane !== undefined) return node;
        const a = remove(node.a, target), b = remove(node.b, target);
        return !a ? b : !b ? a : {split: node.split, ratio: node.ratio, a: a, b: b};
    }
    function split(dir) {
        if (!tab) return;
        const s = sessionFor(focusedPane);
        const id = createSession("", s && s.cwd ? decodeURIComponent(s.cwd.replace("file://", "")) : "");
        const t = tabs.slice();
        t[currentTab] = {root: replace(tab.root, focusedPane, {split: dir, ratio: .5, a: {pane: focusedPane}, b: {pane: id}}), focus: id};
        tabs = t; focusedPane = id;
    }
    function closePane(id) {
        const target = id || focusedPane;
        for (let i = 0; i < tabs.length; i++) {
            if (leaves(tabs[i].root).indexOf(target) < 0) continue;
            const rest = remove(tabs[i].root, target);
            const t = tabs.slice();
            if (rest) t[i] = {root: rest, focus: leaves(rest)[0]}; else t.splice(i, 1);
            tabs = t;
            break;
        }
        const e = entries[target];
        if (e) { const m = Object.assign({}, entries); delete m[target]; entries = m; e.session.destroy(); e.destroy(); }
        if (!tabs.length) { Qt.quit(); return; }
        currentTab = Math.min(currentTab, tabs.length - 1);
        focusedPane = tabs[currentTab].focus;
    }
    function paneExited(id) { if (entries[id]) closePane(id); }
    function closeTab(i) { for (const id of leaves(tabs[i].root)) closePane(id); }
    function focusPane(id) { focusedPane = id; const t = tabs.slice(); if (t[currentTab]) { t[currentTab] = {root: t[currentTab].root, focus: id}; } }
    function focusPaneAnywhere(id) { for (let i = 0; i < tabs.length; i++) if (leaves(tabs[i].root).indexOf(id) >= 0) { currentTab = i; focusedPane = id; } sidebar = sidebar && !phone; }
    // Moves focus to the nearest pane in a direction, measured on the pane rectangles.
    function moveFocus(dx, dy) {
        const ids = leaves(tab ? tab.root : null);
        const rects = {};
        function walk(n, x, y, w, h) { if (n.pane !== undefined) { rects[n.pane] = {x: x, y: y, w: w, h: h}; return; } const r = n.ratio || .5; if (n.split === "h") { walk(n.a, x, y, w * r, h); walk(n.b, x + w * r, y, w * (1 - r), h); } else { walk(n.a, x, y, w, h * r); walk(n.b, x, y + h * r, w, h * (1 - r)); } }
        if (!tab) return; walk(tab.root, 0, 0, 1, 1);
        const c = rects[focusedPane]; if (!c) return;
        const cx = c.x + c.w / 2, cy = c.y + c.h / 2;
        let best = 0, bestD = 1e9;
        for (const id of ids) { if (id === focusedPane) continue; const r = rects[id]; const px = r.x + r.w / 2 - cx, py = r.y + r.h / 2 - cy; if ((dx && Math.sign(px) === dx && Math.abs(px) > .01) || (dy && Math.sign(py) === dy && Math.abs(py) > .01)) { const d = Math.abs(px) + Math.abs(py); if (d < bestD) { bestD = d; best = id; } } }
        if (best) focusedPane = best;
    }
    function runInFocused(cmd) { const s = sessionFor(focusedPane); if (s) s.sendText(cmd + "\r", false); refocus(); }
    function zoom(d) { if (d === 0) TerminalConfig.set("font", "size", 14); else TerminalConfig.set("font", "size", Math.max(7, Math.min(40, TerminalConfig.fontSize + d))); }
    function refocus() { revision++; }
    readonly property var tabModel: (revision, tabs.map(t => { const ids = leaves(t.root); const s = sessionFor(t.focus || ids[0]); const st = ids.map(i => entries[i] ? entries[i].status : "");
        const fg = s ? s.foregroundProcess : "";
        return {title: s ? (s.title || (s.cwd ? decodeURIComponent(s.cwd).split("/").filter(x => x).slice(-1)[0] : "") || fg || qsTr("Terminal")) : qsTr("Terminal"),
                icon: /ssh/.test(fg) ? "dns" : /n?vim|nano|hx|helix|emacs/.test(fg) ? "edit_note" : "terminal",
                status: st.indexOf("running") >= 0 ? "running" : st.indexOf("fail") >= 0 ? "fail" : st.indexOf("ok") >= 0 ? "ok" : ""}; }))
    readonly property var paneList: (revision, tabs.reduce((all, t) => all.concat(leaves(t.root).map(id => { const s = sessionFor(id); const e = entries[id];
        return {id: id, focused: id === focusedPane, status: e ? e.status : "", icon: s && /ssh/.test(s.foregroundProcess) ? "dns" : "terminal",
                title: s ? (s.title || qsTr("Terminal")) : "", detail: s ? [s.foregroundProcess, decodeURIComponent(s.cwd.replace("file://", "")).replace(TerminalHelpers.homePath, "~")].filter(x => x).join(" · ") : ""}; })), []))
    Timer { interval: 700; running: true; repeat: true; onTriggered: root.revision++ }
    Component.onCompleted: newTab()

    // Shortcuts. TerminalItem lets these through via appShortcuts.
    Shortcut { sequence: "Ctrl+Shift+T"; onActivated: root.newTab() }
    Shortcut { sequence: "Ctrl+Shift+W"; onActivated: root.closePane() }
    Shortcut { sequence: "Ctrl+Shift+D"; onActivated: root.split("h") }
    Shortcut { sequence: "Ctrl+Shift+E"; onActivated: root.split("v") }
    Shortcut { sequence: "Ctrl+Shift+P"; onActivated: cmdPalette.open = !cmdPalette.open }
    Shortcut { sequence: "Ctrl+Shift+S"; onActivated: root.sidebar = !root.sidebar }
    Shortcut { sequence: "Ctrl+,"; onActivated: root.appearance = !root.appearance }
    Shortcut { sequence: "Ctrl+Tab"; onActivated: { root.currentTab = (root.currentTab + 1) % root.tabs.length; root.focusedPane = root.tab.focus; } }
    Shortcut { sequence: "Ctrl+Shift+Tab"; onActivated: { root.currentTab = (root.currentTab + root.tabs.length - 1) % root.tabs.length; root.focusedPane = root.tab.focus; } }
    Shortcut { sequence: "Alt+Left"; onActivated: root.moveFocus(-1, 0) }
    Shortcut { sequence: "Alt+Right"; onActivated: root.moveFocus(1, 0) }
    Shortcut { sequence: "Alt+Up"; onActivated: root.moveFocus(0, -1) }
    Shortcut { sequence: "Alt+Down"; onActivated: root.moveFocus(0, 1) }
    Shortcut { sequence: "Ctrl+="; onActivated: root.zoom(1) }
    Shortcut { sequence: "Ctrl+-"; onActivated: root.zoom(-1) }
    Shortcut { sequence: "Ctrl+0"; onActivated: root.zoom(0) }

    Rectangle { anchors.fill: parent; color: Theme.alpha(Theme.base, TerminalConfig.opacity) }
    // Optional background image behind the panes, dimmed and blurred by the config.
    Item {
        anchors.fill: parent
        visible: TerminalConfig.backgroundImage !== ""
        Image { id: bgImage; anchors.fill: parent; source: TerminalConfig.backgroundImage ? (TerminalConfig.backgroundImage.startsWith("/") ? "file://" + TerminalConfig.backgroundImage : TerminalConfig.backgroundImage.replace("~", "file://" + TerminalHelpers.homePath)) : ""
            fillMode: Image.PreserveAspectCrop; asynchronous: true; visible: false; sourceSize: Qt.size(root.width, root.height) }
        MultiEffect { anchors.fill: parent; source: bgImage; blurEnabled: TerminalConfig.blur > 0; blur: TerminalConfig.blur; blurMax: 64; brightness: -TerminalConfig.backgroundDim * .6 }
    }

    // Desktop and tablet layout.
    ColumnLayout {
        anchors.fill: parent; anchors.margins: 8; spacing: 8
        visible: !root.phone
        RowLayout { Layout.fillWidth: true; spacing: 4
            MButton { icon: "view_sidebar"; compact: true; tonal: root.sidebar; tooltip: qsTr("Sessions (Ctrl+Shift+S)"); onClicked: root.sidebar = !root.sidebar }
            TermTabs { model: root.tabModel; current: root.currentTab; onPicked: i => { root.currentTab = i; root.focusedPane = root.tabs[i].focus; } onClosed: i => root.closeTab(i); onAdded: root.newTab() }
            Item { Layout.fillWidth: true }
            MButton { icon: "vertical_split"; compact: true; tooltip: qsTr("Split right (Ctrl+Shift+D)"); onClicked: root.split("h") }
            MButton { icon: "horizontal_split"; compact: true; tooltip: qsTr("Split down (Ctrl+Shift+E)"); onClicked: root.split("v") }
            MButton { icon: "search"; compact: true; tooltip: qsTr("Command palette (Ctrl+Shift+P)"); onClicked: cmdPalette.open = true }
            MButton { icon: "palette"; compact: true; tonal: root.appearance; tooltip: qsTr("Appearance (Ctrl+,)"); onClicked: root.appearance = !root.appearance } }
        RowLayout { Layout.fillWidth: true; Layout.fillHeight: true; spacing: 8
            SessionsSidebar { visible: root.sidebar; view: root; Layout.fillHeight: true; Layout.preferredWidth: 280 }
            Item { Layout.fillWidth: true; Layout.fillHeight: true
                Repeater { model: root.tabs.length
                    SplitNode { required property int index; anchors.fill: parent; visible: index === root.currentTab; view: root; tree: root.tabs[index] ? root.tabs[index].root : null } } }
            AppearancePanel { visible: root.appearance; view: root; Layout.fillHeight: true; Layout.preferredWidth: 340 } }
    }

    // Phone layout: one pane at a time; swipe sideways for the others; pinch to zoom; the key row above the keyboard.
    ColumnLayout {
        anchors.fill: parent; spacing: 0
        visible: root.phone
        RowLayout { Layout.fillWidth: true; Layout.preferredHeight: 60; Layout.leftMargin: 8; Layout.rightMargin: 4; spacing: 6
            Rectangle { implicitHeight: 40; implicitWidth: tr.implicitWidth + 28; radius: 20; color: Theme.surfaceHigh
                RowLayout { id: tr; anchors.centerIn: parent; spacing: 8
                    Glyph { name: "terminal"; font.pixelSize: 18; color: Theme.primary }
                    MText { text: root.tabModel[root.currentTab] ? root.tabModel[root.currentTab].title : ""; role: "titleS"; Layout.maximumWidth: 150 }
                    Rectangle { implicitWidth: 22; implicitHeight: 18; radius: 9; color: Theme.secondaryContainer
                        MText { anchors.centerIn: parent; text: root.tabs.length; font.pixelSize: 11; font.weight: Font.Bold; color: Theme.onSecondaryContainer } } }
                TapHandler { onTapped: root.sidebar = !root.sidebar } }
            Item { Layout.fillWidth: true }
            Row { spacing: 5; Repeater { model: root.tab ? root.leaves(root.tab.root) : []
                Rectangle { required property int modelData; width: modelData === root.focusedPane ? 18 : 6; height: 6; radius: 3; color: modelData === root.focusedPane ? Theme.primary : Theme.alpha(Theme.subtext, .5) } } }
            MButton { icon: "add"; compact: true; onClicked: root.newTab() }
            MButton { icon: "more_vert"; compact: true; onClicked: cmdPalette.open = true } }
        Item {
            Layout.fillWidth: true; Layout.fillHeight: true; Layout.leftMargin: 6; Layout.rightMargin: 6
            TermPane { id: phonePane; anchors.fill: parent; visible: !root.sidebar; session: root.sessionFor(root.focusedPane); focused: true; showChrome: false; background: root.paneBackground }
            SessionsSidebar { anchors.fill: parent; visible: root.sidebar; view: root }
            PinchHandler { target: null; property real start: 14
                onActiveChanged: if (active) start = TerminalConfig.fontSize
                onActiveScaleChanged: if (active) TerminalConfig.set("font", "size", Math.round(Math.max(8, Math.min(32, start * activeScale)))) }
            // Swipe sideways between the panes of the tab.
            DragHandler { target: null; xAxis.enabled: true; yAxis.enabled: false; property real sx: 0
                onActiveChanged: { if (active) sx = centroid.position.x; else if (Math.abs(centroid.position.x - sx) > 80 && root.tab) {
                    const ids = root.leaves(root.tab.root); const i = ids.indexOf(root.focusedPane); root.focusedPane = ids[(i + (centroid.position.x < sx ? 1 : ids.length - 1)) % ids.length]; } } }
        }
        KeyRow { Layout.fillWidth: true; session: root.sessionFor(root.focusedPane); visible: Posture.touch }
    }

    CommandPalette { id: cmdPalette; view: root }
}
