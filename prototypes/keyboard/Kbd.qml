import QtQuick
import QtQuick.Layouts
import QtQuick.Shapes
import Opal.Material
// Borealis keyboard prototype. style: tonal keys or borderless; mode: letters, terminal, clipboard or trackpad; bar: suggestions or tools.
Rectangle {
    id: kb
    property string style: "tonal"
    property string mode: "letters"
    property string bar: "suggest"
    property string pressed: ""
    property string accentKey: ""
    property var glide: []
    property string enterIcon: "keyboard_return"
    property string enterLabel: ""
    property bool split: false
    property real keyH: 54
    property bool ctrlLatched: false
    color: Theme.surfaceLow
    implicitHeight: col.implicitHeight + 16
    readonly property var letters: [["q","w","e","r","t","y","u","i","o","p"], ["a","s","d","f","g","h","j","k","l"], ["⇧","z","x","c","v","b","n","m","⌫"]]
    readonly property var digits: ["1","2","3","4","5","6","7","8","9","0"]
    readonly property var termTop: [{t: "Esc"}, {t: "Tab"}, {t: "Ctrl", latch: true}, {t: "Alt"}, {i: "keyboard_arrow_left"}, {i: "keyboard_arrow_down"}, {i: "keyboard_arrow_up"}, {i: "keyboard_arrow_right"}]
    readonly property var termSyms: ["|", "~", "/", "-", "_", "$", ">", "&", "*", ";"]
    readonly property bool tonal: style === "tonal"
    // Key centres are recorded so the glide trail can pass through them.
    property var centers: ({})
    property var keys: []
    // Positions are known only after layout, so the centres are measured once the deck settles.
    Timer { interval: 300; running: true; onTriggered: { const c = {}; for (const k of kb.keys) if (k.label !== "" && k.visible && !(k.label in c)) c[k.label] = k.mapToItem(kb, k.width / 2, k.height / 2); kb.centers = c; } }
    component Key: Item {
        id: key
        property string label: ""
        property string icon: ""
        property string hint: ""
        property string kind: "char"
        property real weight: 1
        property bool latched: false
        readonly property bool isPressed: kb.pressed !== "" && kb.pressed === label
        Layout.fillWidth: true; Layout.preferredWidth: weight * 100; implicitHeight: kb.keyH
        Rectangle {
            id: cap
            anchors.fill: parent; anchors.margins: 2.5
            radius: key.isPressed ? height / 2 : kb.tonal ? 14 : 22
            color: key.latched || key.kind === "enter" ? Theme.primary
                 : key.isPressed ? Theme.primaryContainer
                 : key.kind === "mod" ? (kb.tonal ? Theme.secondaryContainer : Theme.alpha(Theme.secondaryContainer, .7))
                 : kb.tonal ? Theme.surfaceHighest : "transparent"
            readonly property color ink: key.latched || key.kind === "enter" ? Theme.onPrimary : key.kind === "mod" ? Theme.onSecondaryContainer : Theme.text
            MText { anchors.centerIn: parent; visible: key.icon === ""; text: key.label; font.pixelSize: key.label.length > 1 ? 15 : 23; font.weight: key.label.length > 1 ? Font.DemiBold : Font.Normal; color: cap.ink; font.family: key.kind === "term" ? "CaskaydiaCove Nerd Font Mono" : Theme.font }
            Glyph { anchors.centerIn: parent; visible: key.icon !== ""; name: key.icon; font.pixelSize: 23; fill: key.kind === "enter" ? 1 : 0; color: cap.ink }
            MText { visible: key.hint !== ""; anchors.right: parent.right; anchors.top: parent.top; anchors.margins: 5; text: key.hint; font.pixelSize: 10; color: Theme.muted }
        }
        Component.onCompleted: kb.keys.push(key)
    }
    ColumnLayout {
        id: col
        anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top; anchors.margins: 6; spacing: 4
        // Top bar: suggestions, or the tool chips that the suggestions morph into.
        Item {
            Layout.fillWidth: true; implicitHeight: 48
            RowLayout {
                anchors.fill: parent; visible: kb.bar === "suggest" && kb.mode === "letters"; spacing: 6
                Rectangle { implicitWidth: 40; implicitHeight: 40; radius: 20; color: Theme.surfaceHigh
                    Glyph { anchors.centerIn: parent; name: "apps"; font.pixelSize: 20; color: Theme.subtext } }
                Repeater {
                    model: [{t: "hello"}, {t: "help", main: true}, {t: "helm"}]
                    Rectangle { required property var modelData
                        Layout.fillWidth: true; implicitHeight: 40; radius: 20
                        color: modelData.main ? Theme.secondaryContainer : "transparent"
                        MText { anchors.centerIn: parent; text: modelData.t; font.pixelSize: 17; font.weight: modelData.main ? Font.DemiBold : Font.Normal; color: modelData.main ? Theme.onSecondaryContainer : Theme.text } } }
                Rectangle { implicitWidth: 40; implicitHeight: 40; radius: 20; color: Theme.surfaceHigh
                    Glyph { anchors.centerIn: parent; name: "mic"; font.pixelSize: 20; color: Theme.subtext } }
            }
            Flickable {
                anchors.fill: parent; visible: kb.bar === "tools" || kb.mode !== "letters"; contentWidth: tools.width; clip: true
                Row {
                    id: tools; spacing: 6; anchors.verticalCenter: parent.verticalCenter
                    Rectangle { width: 40; height: 40; radius: 20; color: Theme.primary
                        Glyph { anchors.centerIn: parent; name: kb.mode === "letters" ? "close" : "keyboard"; font.pixelSize: 20; color: Theme.onPrimary } }
                    Repeater {
                        model: [{t: "Clipboard", i: "content_paste", m: "clipboard"}, {t: "Emoji", i: "mood"}, {t: "Trackpad", i: "touchpad_mouse", m: "trackpad"}, {t: "Terminal keys", i: "terminal", m: "terminal"}, {t: "One hand", i: "back_hand"}, {t: "Layouts", i: "language"}]
                        Chip { required property var modelData; text: modelData.t; icon: modelData.i; selected: kb.mode === modelData.m; glass: false; implicitHeight: 40 }
                    }
                }
            }
        }
        // Main area by mode.
        ColumnLayout {
            Layout.fillWidth: true; spacing: 2; visible: kb.mode === "letters" || kb.mode === "terminal"
            RowLayout { Layout.fillWidth: true; spacing: 0; visible: kb.mode === "letters"
                Repeater { model: kb.digits; Key { required property string modelData; label: modelData; kind: "digit"; implicitHeight: 40 } } }
            RowLayout { Layout.fillWidth: true; spacing: 0; visible: kb.mode === "terminal"
                Repeater { model: kb.termTop; Key { required property var modelData; label: modelData.t || ""; icon: modelData.i || ""; kind: "mod"; latched: !!modelData.latch && kb.ctrlLatched; implicitHeight: 44 } } }
            Repeater {
                model: kb.letters
                RowLayout {
                    id: rowL
                    required property var modelData
                    required property int index
                    Layout.fillWidth: true; spacing: 0
                    Layout.leftMargin: index === 1 ? kb.width * .045 : 0; Layout.rightMargin: index === 1 ? kb.width * .045 : 0
                    Repeater {
                        model: rowL.modelData
                        Key {
                            required property string modelData
                            required property int index
                            label: modelData === "⇧" || modelData === "⌫" ? "" : modelData
                            icon: modelData === "⇧" ? "shift" : modelData === "⌫" ? "backspace" : ""
                            kind: modelData === "⇧" || modelData === "⌫" ? "mod" : "char"
                            weight: kind === "mod" ? 1.45 : 1
                            hint: kb.mode === "terminal" && rowL.index === 0 ? kb.digits[index] : ""
                            Layout.rightMargin: kb.split && rowL.index === 0 && index === 4 ? kb.width * .28 : kb.split && rowL.index === 1 && index === 4 ? kb.width * .28 : kb.split && rowL.index === 2 && index === 4 ? kb.width * .28 : 0
                        }
                    }
                }
            }
            RowLayout {
                Layout.fillWidth: true; spacing: 0
                Key { label: kb.mode === "terminal" ? "!#1" : "?123"; kind: "mod"; weight: 1.5 }
                Key { visible: kb.mode !== "terminal"; icon: "mood"; kind: "mod"; weight: 1.1 }
                Repeater { model: kb.mode === "terminal" ? kb.termSyms.slice(0, 3) : [","]; Key { required property string modelData; label: modelData; kind: kb.mode === "terminal" ? "term" : "char" } }
                Key { label: kb.mode === "terminal" ? "space" : "English"; kind: "space"; weight: kb.split ? 1.8 : 4.2; Layout.rightMargin: kb.split ? kb.width * .28 : 0 }
                Repeater { model: kb.mode === "terminal" ? kb.termSyms.slice(3, 5) : ["."]; Key { required property string modelData; label: modelData; kind: kb.mode === "terminal" ? "term" : "char" } }
                Key { icon: kb.enterIcon; label: kb.enterLabel; kind: "enter"; weight: 1.6 }
            }
        }
        // Clipboard: recent clips from the Opal clipboard service as cards; pinned first.
        GridLayout {
            Layout.fillWidth: true; visible: kb.mode === "clipboard"; columns: 2; rowSpacing: 6; columnSpacing: 6
            Repeater {
                model: [{t: "cmake --build build && ctest --test-dir build", pin: true, code: true}, {t: "kriscrossapplesauce2004/achroot", link: true}, {img: "file:///home/melissa/Pictures/Wallpapers/dr5-keyart-YzI66DkClTeJ5jBQ.png"}, {t: "Meet at the station at 7, bring the dock"}, {t: "#cfbdff"}, {t: "sudo pacman -Syu", code: true}]
                Rectangle {
                    required property var modelData
                    Layout.fillWidth: true; implicitHeight: 82; radius: 20; color: Theme.surfaceHigh
                    RoundedImage { visible: !!modelData.img; anchors.fill: parent; radius: 20; source: modelData.img || ""; sourceSize: Qt.size(400, 200) }
                    MText { visible: !modelData.img; anchors.fill: parent; anchors.margins: 12; text: modelData.t || ""; wrapMode: Text.Wrap; maximumLineCount: 3; font.pixelSize: 13; font.family: modelData.code ? "CaskaydiaCove Nerd Font Mono" : Theme.font; color: modelData.link ? Theme.primary : Theme.text }
                    Rectangle { visible: !!modelData.t && modelData.t[0] === "#"; anchors.right: parent.right; anchors.bottom: parent.bottom; anchors.margins: 10; width: 28; height: 28; radius: 14; color: modelData.t || "transparent" }
                    Glyph { visible: !!modelData.pin; anchors.right: parent.right; anchors.top: parent.top; anchors.margins: 8; name: "keep"; fill: 1; font.pixelSize: 16; color: Theme.primary }
                }
            }
        }
        // Trackpad: hold space and the keys become a trackpad surface.
        Rectangle {
            Layout.fillWidth: true; visible: kb.mode === "trackpad"; implicitHeight: kb.keyH * 4 + 44; radius: 26; color: Theme.surface
            Grid { anchors.fill: parent; anchors.margins: 18; columns: 10; rows: 4; columnSpacing: (width - 10 * 6) / 9; rowSpacing: (height - 4 * 6) / 3
                Repeater { model: 40; Rectangle { width: 6; height: 6; radius: 3; color: Theme.alpha(Theme.subtext, .25) } } }
            Column { anchors.centerIn: parent; spacing: 6
                Glyph { anchors.horizontalCenter: parent.horizontalCenter; name: "touchpad_mouse"; font.pixelSize: 40; color: Theme.primary }
                MText { anchors.horizontalCenter: parent.horizontalCenter; text: "Trackpad"; role: "title" }
                MText { anchors.horizontalCenter: parent.horizontalCenter; text: "Drag to move the cursor · two fingers to select"; role: "labelS"; color: Theme.subtext } }
            Rectangle { x: parent.width * .58; y: parent.height * .44; width: 56; height: 56; radius: 28; color: Theme.alpha(Theme.primary, .25); border.width: 2; border.color: Theme.primary }
        }
    }
    // Glide trail through the key centres of the word.
    // Catmull-Rom spline through the key centres, so the trail reads as one finger stroke.
    function spline(pts) {
        if (pts.length < 2) return pts;
        const out = [], p = [pts[0]].concat(pts, [pts[pts.length - 1]]);
        for (let i = 1; i < p.length - 2; i++)
            for (let t = 0; t < 1; t += 1 / 16) {
                const t2 = t * t, t3 = t2 * t;
                const f = (a, b, c, d) => .5 * (2 * b + (-a + c) * t + (2 * a - 5 * b + 4 * c - d) * t2 + (-a + 3 * b - 3 * c + d) * t3);
                out.push(Qt.point(f(p[i - 1].x, p[i].x, p[i + 1].x, p[i + 2].x), f(p[i - 1].y, p[i].y, p[i + 1].y, p[i + 2].y)));
            }
        out.push(pts[pts.length - 1]);
        return out;
    }
    readonly property var trail: spline(glide.map(l => centers[l]).filter(p => !!p))
    Shape {
        anchors.fill: parent; visible: kb.trail.length > 1
        preferredRendererType: Shape.CurveRenderer
        ShapePath { strokeWidth: 18; strokeColor: Theme.alpha(Theme.primary, .18); fillColor: "transparent"; capStyle: ShapePath.RoundCap; joinStyle: ShapePath.RoundJoin
            PathPolyline { path: kb.trail } }
        ShapePath { strokeWidth: 6; strokeColor: Theme.primary; fillColor: "transparent"; capStyle: ShapePath.RoundCap; joinStyle: ShapePath.RoundJoin
            PathPolyline { path: kb.trail } }
    }
    Rectangle { visible: kb.trail.length > 1; readonly property point e: kb.trail.length ? kb.trail[kb.trail.length - 1] : Qt.point(0, 0); x: e.x - 11; y: e.y - 11; width: 22; height: 22; radius: 11; color: Theme.primary; border.width: 4; border.color: Theme.alpha(Theme.onPrimary, .6) }
    // Press preview balloon above the pressed key.
    Rectangle {
        readonly property point c: kb.centers[kb.pressed] || Qt.point(-200, -200)
        visible: kb.pressed !== "" && kb.accentKey === ""
        width: 58; height: 76; radius: 22; x: c.x - width / 2; y: c.y - height - 18
        color: Theme.primaryContainer
        MText { anchors.centerIn: parent; text: kb.pressed; font.pixelSize: 34; color: Theme.onContainer }
    }
    // Accent picker: a pill that grows out of the held key.
    Rectangle {
        readonly property point c: kb.centers[kb.accentKey] || Qt.point(-200, -200)
        visible: kb.accentKey !== ""
        width: accRow.implicitWidth + 12; height: 60; radius: 30; x: Math.max(6, Math.min(kb.width - width - 6, c.x - 30)); y: c.y - height - 30
        color: Theme.surfaceHighest; border.width: 1; border.color: Theme.outline
        Row { id: accRow; anchors.centerIn: parent; spacing: 2
            Repeater { model: ["e", "é", "è", "ê", "ë", "ē"]
                Rectangle { required property string modelData; required property int index; width: 44; height: 48; radius: 24; color: index === 1 ? Theme.primary : "transparent"
                    MText { anchors.centerIn: parent; text: modelData; font.pixelSize: 24; color: index === 1 ? Theme.onPrimary : Theme.text } } } }
    }
}
