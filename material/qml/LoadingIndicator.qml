import QtQuick
import Opal.Material
// M3 Expressive loading indicator: a shape that morphs through soft polygons while it turns. contained puts it on a tonal circle.
Item {
    id: li
    property bool contained: false
    property color color: contained ? Theme.onContainer : Theme.primary
    property bool running: visible
    implicitWidth: 48; implicitHeight: 48
    // Lobes and depth of each step: soft burst, cookie 9, pentagon, pill, sunny, cookie 4, oval.
    readonly property var shapes: [[10, .10], [9, .16], [5, .14], [2, .22], [8, .12], [4, .2], [2, .12]]
    property int step: 0
    property real k: 1
    property real turn: 0
    NumberAnimation on turn { running: li.running && !Theme.reduceMotion; from: 0; to: 360; duration: 4200; loops: Animation.Infinite }
    SequentialAnimation {
        running: li.running && !Theme.reduceMotion; loops: Animation.Infinite
        ScriptAction { script: { li.step = (li.step + 1) % li.shapes.length; li.k = 0; } }
        NumberAnimation { target: li; property: "k"; from: 0; to: 1; duration: 560; easing.type: Easing.BezierSpline; easing.bezierCurve: Theme.emphasized }
        PauseAnimation { duration: 90 }
    }
    onKChanged: canvas.requestPaint()
    onTurnChanged: canvas.requestPaint()
    onColorChanged: canvas.requestPaint()
    Rectangle { visible: li.contained; anchors.fill: parent; radius: width / 2; color: Theme.primaryContainer }
    Canvas {
        id: canvas
        anchors.fill: parent; anchors.margins: li.contained ? parent.width * .16 : 0
        onPaint: {
            const c = getContext("2d"); c.reset();
            const n = li.shapes.length, a = li.shapes[(li.step + n - 1) % n], b = li.shapes[li.step];
            const cx = width / 2, cy = height / 2, base = Math.min(width, height) * .5 / 1.25, off = li.turn * Math.PI / 180;
            c.beginPath();
            for (let s = 0; s <= 160; s++) {
                const ang = s / 160 * Math.PI * 2;
                const ra = 1 + a[1] * Math.cos(a[0] * (ang - off)), rb = 1 + b[1] * Math.cos(b[0] * (ang - off));
                const r = base * (ra + (rb - ra) * li.k);
                if (s === 0) c.moveTo(cx + Math.cos(ang) * r, cy + Math.sin(ang) * r); else c.lineTo(cx + Math.cos(ang) * r, cy + Math.sin(ang) * r);
            }
            c.closePath(); c.fillStyle = li.color; c.fill();
        }
    }
    Accessible.role: Accessible.ProgressBar; Accessible.name: qsTr("Loading")
}
