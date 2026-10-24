import QtQuick
import QtQuick.Layouts
import Opal.Material
import Opal.Media
// The seek bar (a wavy line while playing, draggable) and the transport: shuffle, previous, play, next, repeat.
// For audiobooks the outer buttons skip 30 seconds back and forward instead.
ColumnLayout {
    id: t
    property real size: 64
    property var s: null
    property bool compact: false
    readonly property bool book: Player.current.kind === "book"
    readonly property bool radio: Player.current.kind === "radio"
    readonly property color primary: s ? s.primary : Theme.primary
    readonly property color onPrimary: s ? s.on_primary : Theme.onPrimary
    readonly property color tonal: s ? s.secondary_container : Theme.secondaryContainer
    readonly property color onTonal: s ? s.on_secondary_container : Theme.onSecondaryContainer
    readonly property color sub: s ? s.on_surface_variant : Theme.subtext
    property real dragValue: -1
    spacing: 6
    Item {
        visible: !t.radio
        Layout.fillWidth: true; implicitHeight: 22
        WavyProgress {
            anchors.left: parent.left; anchors.right: parent.right; anchors.verticalCenter: parent.verticalCenter
            value: t.dragValue >= 0 ? t.dragValue : Player.duration > 0 ? Player.position / Player.duration : 0
            animate: Player.playing; amplitude: Player.playing ? 3 : 0; thickness: 5; wavelength: 30
            color: t.primary; trackColor: t.tonal
            Behavior on amplitude { NumberAnimation { duration: Theme.motion(Theme.durMedium) } }
        }
        MouseArea {
            anchors.fill: parent; anchors.topMargin: -8; anchors.bottomMargin: -8
            cursorShape: Qt.PointingHandCursor
            function at(x) { return Math.max(0, Math.min(1, x / width)); }
            onPressed: m => t.dragValue = at(m.x)
            onPositionChanged: m => { if (pressed) t.dragValue = at(m.x); }
            onReleased: m => { Player.seek(at(m.x) * Player.duration); t.dragValue = -1; }
            onCanceled: t.dragValue = -1
        }
        Accessible.role: Accessible.Slider; Accessible.name: qsTr("Position")
    }
    RowLayout {
        visible: !t.radio
        Layout.fillWidth: true
        MText { text: MediaLibrary.formatDuration(t.dragValue >= 0 ? t.dragValue * Player.duration : Player.position); role: "labelS"; color: t.sub }
        Item { Layout.fillWidth: true }
        MText { text: t.book ? "-" + MediaLibrary.formatDuration(Math.max(0, Player.duration - Player.position)) : MediaLibrary.formatDuration(Player.duration); role: "labelS"; color: t.sub }
    }
    RowLayout {
        Layout.alignment: Qt.AlignHCenter; spacing: t.size * .2
        RoundIcon { visible: !t.radio; icon: t.book ? "replay_30" : "shuffle"; on: !t.book && Player.shuffle; tip: t.book ? qsTr("Back 30 seconds") : qsTr("Shuffle")
            onClicked: t.book ? Player.seekBy(-30000) : Player.shuffle = !Player.shuffle }
        Rectangle {
            visible: !t.radio
            implicitWidth: t.size * .8; implicitHeight: t.size * .8; radius: width / 2; color: t.tonal
            scale: prevTap.pressed ? .92 : 1
            Glyph { anchors.centerIn: parent; name: "skip_previous"; fill: 1; font.pixelSize: t.size * .42; color: t.onTonal }
            HoverHandler { cursorShape: Qt.PointingHandCursor }
            TapHandler { id: prevTap; onTapped: Player.previous() }
            Accessible.role: Accessible.Button; Accessible.name: t.book ? qsTr("Previous chapter") : qsTr("Previous")
        }
        PlayButton { size: t.size; fill: t.primary; ink: t.onPrimary; onClicked: Player.toggle() }
        Rectangle {
            visible: !t.radio
            implicitWidth: t.size * .8; implicitHeight: t.size * .8; radius: width / 2; color: t.tonal
            scale: nextTap.pressed ? .92 : 1
            Glyph { anchors.centerIn: parent; name: "skip_next"; fill: 1; font.pixelSize: t.size * .42; color: t.onTonal }
            HoverHandler { cursorShape: Qt.PointingHandCursor }
            TapHandler { id: nextTap; onTapped: Player.next() }
            Accessible.role: Accessible.Button; Accessible.name: t.book ? qsTr("Next chapter") : qsTr("Next")
        }
        RoundIcon { visible: !t.radio; icon: t.book ? "forward_30" : Player.repeat === "one" ? "repeat_one" : "repeat"; on: !t.book && Player.repeat !== "off"
            tip: t.book ? qsTr("Forward 30 seconds") : qsTr("Repeat")
            onClicked: t.book ? Player.seekBy(30000) : Player.repeat = Player.repeat === "off" ? "all" : Player.repeat === "all" ? "one" : "off" }
    }
    component RoundIcon: Rectangle {
        id: ri
        property string icon
        property bool on: false
        property string tip
        signal clicked()
        implicitWidth: t.size * .66; implicitHeight: implicitWidth; radius: width / 2
        color: on ? Theme.alpha(t.primary, .16) : riHover.hovered ? Theme.alpha(t.sub, .1) : "transparent"
        Glyph { anchors.centerIn: parent; name: ri.icon; font.pixelSize: t.size * .36; color: ri.on ? t.primary : t.sub }
        HoverHandler { id: riHover; cursorShape: Qt.PointingHandCursor }
        TapHandler { onTapped: ri.clicked() }
        Accessible.role: Accessible.Button; Accessible.name: tip; Accessible.checked: on
    }
}
