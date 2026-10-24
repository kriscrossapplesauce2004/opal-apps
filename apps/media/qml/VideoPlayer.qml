import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtMultimedia
import Opal.Material
import Opal.Media
// Video playback over the whole window: resumes where you stopped, controls fade while it plays, subtitles and audio tracks.
Rectangle {
    id: vp
    property var video: ({})
    property bool chrome: true
    signal close()
    signal fullScreenRequested(bool on)
    color: "#000000"
    focus: true
    Component.onCompleted: {
        Player.pause();
        mp.source = video.url;
        if (video.position > 0) resumeAt = video.position;
        mp.play();
        forceActiveFocus();
    }
    property real resumeAt: 0
    function leave() { MediaLibrary.setVideoPosition(video.path, mp.position); mp.stop(); close(); }
    MediaPlayer {
        id: mp
        audioOutput: AudioOutput { id: ao; volume: Player.volume; muted: Player.muted }
        videoOutput: out
        onMediaStatusChanged: {
            if (mediaStatus === MediaPlayer.LoadedMedia && vp.resumeAt > 0) { position = vp.resumeAt; vp.resumeAt = 0; }
            if (mediaStatus === MediaPlayer.EndOfMedia) { MediaLibrary.setVideoPosition(vp.video.path, 0); vp.chrome = true; }
        }
        onErrorOccurred: (e, msg) => errText.text = msg
    }
    Timer { interval: 10000; repeat: true; running: mp.playbackState === MediaPlayer.PlayingState; onTriggered: MediaLibrary.setVideoPosition(vp.video.path, mp.position) }
    VideoOutput { id: out; anchors.fill: parent; fillMode: VideoOutput.PreserveAspectFit }
    MText { id: errText; anchors.centerIn: parent; visible: text !== ""; color: "#ffffff"; role: "title" }
    Timer { id: idle; interval: 2800; running: vp.chrome && mp.playbackState === MediaPlayer.PlayingState; onTriggered: vp.chrome = false }
    function wake() { chrome = true; idle.restart(); }
    HoverHandler { onPointChanged: vp.wake(); cursorShape: vp.chrome ? Qt.ArrowCursor : Qt.BlankCursor }
    TapHandler { onTapped: Posture.touch ? (vp.chrome = !vp.chrome) : (mp.playbackState === MediaPlayer.PlayingState ? mp.pause() : mp.play()); onDoubleTapped: vp.fullScreenRequested(true) }
    Keys.onPressed: e => {
        wake();
        if (e.key === Qt.Key_Escape || e.key === Qt.Key_Back) { leave(); e.accepted = true; }
        else if (e.key === Qt.Key_Space || e.key === Qt.Key_K) { mp.playbackState === MediaPlayer.PlayingState ? mp.pause() : mp.play(); e.accepted = true; }
        else if (e.key === Qt.Key_Left || e.key === Qt.Key_J) { mp.position = Math.max(0, mp.position - 10000); e.accepted = true; }
        else if (e.key === Qt.Key_Right || e.key === Qt.Key_L) { mp.position = Math.min(mp.duration, mp.position + 10000); e.accepted = true; }
        else if (e.key === Qt.Key_Up) { Player.volume = Math.min(1, Player.volume + .05); e.accepted = true; }
        else if (e.key === Qt.Key_Down) { Player.volume = Math.max(0, Player.volume - .05); e.accepted = true; }
        else if (e.key === Qt.Key_M) { Player.muted = !Player.muted; e.accepted = true; }
        else if (e.key === Qt.Key_F) { vp.fullScreenRequested(true); e.accepted = true; }
    }
    // ---------- top ----------
    Rectangle {
        anchors.left: parent.left; anchors.right: parent.right; height: 96
        opacity: vp.chrome ? 1 : 0; visible: opacity > 0
        Behavior on opacity { NumberAnimation { duration: Theme.motion(Theme.durMedium) } }
        gradient: Gradient { GradientStop { position: 0; color: Qt.rgba(0, 0, 0, .7) } GradientStop { position: 1; color: "transparent" } }
        RowLayout { anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top; anchors.margins: 16; spacing: 12
            Btn { icon: "arrow_back"; tip: qsTr("Back"); onClicked: vp.leave() }
            MText { Layout.fillWidth: true; text: vp.video.title || ""; role: "title"; color: "#ffffff"; elide: Text.ElideRight }
            Btn { visible: mp.subtitleTracks.length > 0; icon: mp.activeSubtitleTrack >= 0 ? "subtitles" : "subtitles_off"; tip: qsTr("Subtitles"); onClicked: subMenu.popup() }
            Btn { visible: mp.audioTracks.length > 1; icon: "audiotrack"; tip: qsTr("Audio track"); onClicked: audioMenu.popup() } }
    }
    Menu {
        id: subMenu
        MenuItem { text: qsTr("Off"); checkable: true; checked: mp.activeSubtitleTrack < 0; onTriggered: mp.activeSubtitleTrack = -1 }
        Repeater { model: mp.subtitleTracks.length
            MenuItem { required property int index; text: mp.subtitleTracks[index].stringValue(MediaMetaData.Language) || qsTr("Track %1").arg(index + 1); checkable: true; checked: mp.activeSubtitleTrack === index; onTriggered: mp.activeSubtitleTrack = index } }
    }
    Menu {
        id: audioMenu
        Repeater { model: mp.audioTracks.length
            MenuItem { required property int index; text: mp.audioTracks[index].stringValue(MediaMetaData.Language) || qsTr("Track %1").arg(index + 1); checkable: true; checked: mp.activeAudioTrack === index; onTriggered: mp.activeAudioTrack = index } }
    }
    // ---------- bottom ----------
    Rectangle {
        anchors.left: parent.left; anchors.right: parent.right; anchors.bottom: parent.bottom; height: 150
        opacity: vp.chrome ? 1 : 0; visible: opacity > 0
        Behavior on opacity { NumberAnimation { duration: Theme.motion(Theme.durMedium) } }
        gradient: Gradient { GradientStop { position: 0; color: "transparent" } GradientStop { position: 1; color: Qt.rgba(0, 0, 0, .75) } }
        ColumnLayout {
            anchors.left: parent.left; anchors.right: parent.right; anchors.bottom: parent.bottom; anchors.margins: 20; spacing: 8
            Item {
                Layout.fillWidth: true; implicitHeight: 20
                property real drag: -1
                Rectangle { anchors.verticalCenter: parent.verticalCenter; width: parent.width; height: 5; radius: 3; color: Qt.rgba(1, 1, 1, .3)
                    Rectangle { width: parent.width * (parent.parent.drag >= 0 ? parent.parent.drag : mp.duration > 0 ? mp.position / mp.duration : 0); height: parent.height; radius: 3; color: Theme.primary } }
                MouseArea { anchors.fill: parent; anchors.margins: -8; cursorShape: Qt.PointingHandCursor
                    function at(x) { return Math.max(0, Math.min(1, x / width)); }
                    onPressed: m => parent.drag = at(m.x); onPositionChanged: m => { if (pressed) parent.drag = at(m.x); }
                    onReleased: m => { mp.position = at(m.x) * mp.duration; parent.drag = -1; } }
                Accessible.role: Accessible.Slider; Accessible.name: qsTr("Position")
            }
            RowLayout { Layout.fillWidth: true; spacing: 10
                Btn { icon: "replay_10"; tip: qsTr("Back 10 seconds"); onClicked: mp.position = Math.max(0, mp.position - 10000) }
                PlayButton { size: 52; playing: mp.playbackState === MediaPlayer.PlayingState; busy: false; onClicked: playing ? mp.pause() : mp.play() }
                Btn { icon: "forward_10"; tip: qsTr("Forward 10 seconds"); onClicked: mp.position = Math.min(mp.duration, mp.position + 10000) }
                MText { text: MediaLibrary.formatDuration(mp.position) + " / " + MediaLibrary.formatDuration(mp.duration); role: "label"; color: "#ffffff"; Layout.leftMargin: 6 }
                Item { Layout.fillWidth: true }
                Btn { icon: Player.muted || Player.volume === 0 ? "volume_off" : "volume_up"; tip: qsTr("Mute"); onClicked: Player.muted = !Player.muted }
                ExpressiveSlider { Layout.preferredWidth: 140; from: 0; to: 100; value: Player.volume * 100; compact: true; showIcon: false; onMoved: Player.volume = value / 100; Accessible.name: qsTr("Volume") }
                Btn { icon: "fullscreen"; tip: qsTr("Full screen"); onClicked: vp.fullScreenRequested(true) }
            }
        }
    }
    component Btn: Rectangle {
        id: b
        property string icon
        property string tip
        signal clicked()
        implicitWidth: 44; implicitHeight: 44; radius: 22
        color: bHover.hovered ? Qt.rgba(1, 1, 1, .18) : "transparent"
        Glyph { anchors.centerIn: parent; name: b.icon; color: "#ffffff" }
        HoverHandler { id: bHover; cursorShape: Qt.PointingHandCursor }
        TapHandler { onTapped: b.clicked() }
        Accessible.role: Accessible.Button; Accessible.name: tip
    }
}
