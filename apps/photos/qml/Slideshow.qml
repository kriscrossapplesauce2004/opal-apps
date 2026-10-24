import QtQuick
import Opal.Material
import Opal.Photos
// Full-window slideshow: photos cross-fade with a slow drift. Space pauses, arrows step, Esc leaves.
Rectangle {
    id: show
    property var paths: []
    property int index: 0
    property bool playing: true
    property int seconds: 5
    signal closed(int index)
    color: "#000000"
    focus: true
    Component.onCompleted: { a.source = urlAt(index); forceActiveFocus(); }
    function urlAt(i) { return paths.length ? PhotoLibrary.uri(paths[(i + paths.length) % paths.length]) : ""; }
    property bool frontA: true
    function step(delta) {
        if (!paths.length) return;
        index = (index + delta + paths.length) % paths.length;
        const next = frontA ? b : a;
        next.source = urlAt(index);
        frontA = !frontA;
        tick.restart();
    }
    Timer { id: tick; interval: show.seconds * 1000; repeat: true; running: show.playing && show.paths.length > 1; onTriggered: show.step(1) }
    // Video files are left out; they show as their thumbnail.
    component Slide: Image {
        property bool front: false
        anchors.fill: parent; fillMode: Image.PreserveAspectFit; asynchronous: true; autoTransform: true
        sourceSize: Qt.size(show.width * 1.2, show.height * 1.2)
        opacity: front && status === Image.Ready ? 1 : 0
        Behavior on opacity { NumberAnimation { duration: Theme.motion(900); easing.type: Easing.InOutQuad } }
        scale: front ? 1.06 : 1
        Behavior on scale { NumberAnimation { duration: Theme.reduceMotion ? 0 : show.seconds * 1000 + 900; easing.type: Easing.Linear } }
    }
    Slide { id: a; front: show.frontA }
    Slide { id: b; front: !show.frontA }
    Rectangle {
        id: hud
        anchors.horizontalCenter: parent.horizontalCenter; anchors.bottom: parent.bottom; anchors.bottomMargin: 28
        width: hudRow.implicitWidth + 24; height: 56; radius: 28; color: Qt.rgba(0, 0, 0, .55)
        opacity: hover.hovered || !show.playing ? 1 : 0
        Behavior on opacity { NumberAnimation { duration: Theme.motion(Theme.durMedium) } }
        Row {
            id: hudRow; anchors.centerIn: parent; spacing: 6
            Repeater { model: [{i: "skip_previous", t: qsTr("Previous"), r: () => show.step(-1)}, {i: show.playing ? "pause" : "play_arrow", t: show.playing ? qsTr("Pause") : qsTr("Play"), r: () => show.playing = !show.playing},
                               {i: "skip_next", t: qsTr("Next"), r: () => show.step(1)}, {i: "close", t: qsTr("Leave slideshow"), r: () => show.closed(show.index)}]
                Rectangle { required property var modelData; width: 44; height: 44; radius: 22; color: bh.hovered ? Qt.rgba(1, 1, 1, .2) : "transparent"
                    Glyph { anchors.centerIn: parent; name: modelData.i; fill: 1; color: "#ffffff" }
                    HoverHandler { id: bh; cursorShape: Qt.PointingHandCursor }
                    TapHandler { onTapped: parent.modelData.r() }
                    Accessible.role: Accessible.Button; Accessible.name: modelData.t } }
            MText { text: (show.index + 1) + " / " + show.paths.length; color: "#ffffff"; role: "label"; anchors.verticalCenter: parent.verticalCenter; leftPadding: 6; rightPadding: 6 }
        }
    }
    HoverHandler { id: hover }
    Keys.onPressed: e => {
        if (e.key === Qt.Key_Escape || e.key === Qt.Key_Back) { closed(index); e.accepted = true; }
        else if (e.key === Qt.Key_Space || e.key === Qt.Key_Return) { playing = !playing; e.accepted = true; }
        else if (e.key === Qt.Key_Right) { step(1); e.accepted = true; }
        else if (e.key === Qt.Key_Left) { step(-1); e.accepted = true; }
    }
}
