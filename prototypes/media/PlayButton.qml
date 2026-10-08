import QtQuick
import Opal.Material
// M3 Expressive play button: a circle while paused, a squircle while playing, so the state reads from the shape.
Rectangle {
    id: b
    property bool playing: true
    property real size: 72
    implicitWidth: size * (playing ? 1.25 : 1); implicitHeight: size
    radius: playing ? size * .3 : size / 2
    color: Theme.primary
    Behavior on radius { NumberAnimation { duration: Theme.motion(Theme.durMedium); easing.type: Easing.BezierSpline; easing.bezierCurve: Theme.springy } }
    Glyph { anchors.centerIn: parent; name: b.playing ? "pause" : "play_arrow"; fill: 1; font.pixelSize: b.size * .48; color: Theme.onPrimary }
    Accessible.role: Accessible.Button; Accessible.name: playing ? "Pause" : "Play"
}
