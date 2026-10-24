import QtQuick
import Opal.Material
import Opal.Media
// M3 Expressive play button: a circle while paused, a squircle while playing, so the state reads from the shape.
Rectangle {
    id: b
    property bool playing: Player.playing
    property bool busy: Player.buffering && Player.playing
    property real size: 72
    property color fill: Theme.primary
    property color ink: Theme.onPrimary
    signal clicked()
    implicitWidth: size * (playing ? 1.25 : 1); implicitHeight: size
    radius: playing ? size * .3 : size / 2
    color: fill
    Behavior on radius { NumberAnimation { duration: Theme.motion(Theme.durMedium); easing.type: Easing.BezierSpline; easing.bezierCurve: Theme.springy } }
    Behavior on implicitWidth { NumberAnimation { duration: Theme.motion(Theme.durMedium); easing.type: Easing.BezierSpline; easing.bezierCurve: Theme.springy } }
    scale: tap.pressed ? .94 : 1
    Behavior on scale { NumberAnimation { duration: Theme.motion(Theme.durShort) } }
    Glyph { anchors.centerIn: parent; visible: !b.busy; name: b.playing ? "pause" : "play_arrow"; fill: 1; font.pixelSize: b.size * .48; color: b.ink }
    LoadingIndicator { anchors.centerIn: parent; visible: b.busy; width: b.size * .5; height: width; color: b.ink }
    HoverHandler { cursorShape: Qt.PointingHandCursor }
    TapHandler { id: tap; onTapped: b.clicked() }
    Accessible.role: Accessible.Button; Accessible.name: playing ? qsTr("Pause") : qsTr("Play")
    Accessible.onPressAction: clicked()
}
