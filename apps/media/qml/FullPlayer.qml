import QtQuick
import QtQuick.Controls
import QtQuick.Effects
import QtQuick.Layouts
import Opal.Material
import Opal.Media
// The full player, tinted by the art (calmed). Wide: the cover fills the height on the left and one column holds the rest.
// Narrow: the art runs edge to edge and fades into the tint, with the title on the fade. Lyrics and queue swap in place.
Rectangle {
    id: fp
    property string panel: ""
    signal close()
    signal openAlbum(string key)
    readonly property bool wide: width > 900 && height > 520
    readonly property color seed: Player.artColor.valid ? Player.artColor : Theme.primary
    readonly property var s: Palette.schemeFor(seed, Theme.light, .5)
    readonly property bool book: Player.current.kind === "book"
    color: s.surface
    focus: true
    Keys.onPressed: e => {
        if (e.key === Qt.Key_Escape || e.key === Qt.Key_Back) { if (panel && !wide) panel = ""; else close(); e.accepted = true; }
        else if (e.key === Qt.Key_Space) { Player.toggle(); e.accepted = true; }
        else if (e.key === Qt.Key_Left) { Player.seekBy(-10000); e.accepted = true; }
        else if (e.key === Qt.Key_Right) { Player.seekBy(10000); e.accepted = true; }
        else if (e.key === Qt.Key_L) { panel = panel === "lyrics" ? "" : "lyrics"; e.accepted = true; }
        else if (e.key === Qt.Key_Q) { panel = panel === "queue" ? "" : "queue"; e.accepted = true; }
    }
    readonly property var pillItems: [
        {id: "output", i: "speaker_group", t: (Player.outputs.find(x => x.id === Player.output) || {name: qsTr("This PC")}).name.split(" ").slice(0, 2).join(" ")},
        {id: "lyrics", i: "lyrics", t: qsTr("Lyrics"), on: panel === "lyrics"},
        {id: "queue", i: fp.book ? "toc" : "queue_music", t: fp.book ? qsTr("Chapters") : qsTr("Queue"), on: panel === "queue"}]
    function pick(id) { if (id === "output") outputMenu.popup(); else panel = panel === id ? "" : id; }
    Menu {
        id: outputMenu
        Repeater { model: Player.outputs
            MenuItem { required property var modelData; text: modelData.name; checkable: true; checked: modelData.id === Player.output; onTriggered: Player.setOutput(modelData.id) } }
    }
    Menu {
        id: moreMenu
        MenuItem { text: qsTr("Go to album"); enabled: !!Player.current.albumKey; onTriggered: { fp.openAlbum(Player.current.albumKey); fp.close(); } }
        MenuItem { text: MediaLibrary.likedRevision >= 0 && MediaLibrary.liked(Player.current.path || "") ? qsTr("Remove from liked songs") : qsTr("Like"); enabled: Player.current.kind === "music"; onTriggered: MediaLibrary.toggleLiked(Player.current.path) }
        Menu {
            title: Player.sleepRemaining !== 0 ? qsTr("Sleep timer (on)") : qsTr("Sleep timer")
            MenuItem { text: qsTr("Off"); onTriggered: Player.setSleep(0) }
            MenuItem { text: qsTr("15 minutes"); onTriggered: Player.setSleep(15) }
            MenuItem { text: qsTr("30 minutes"); onTriggered: Player.setSleep(30) }
            MenuItem { text: qsTr("1 hour"); onTriggered: Player.setSleep(60) }
            MenuItem { text: fp.book ? qsTr("End of chapter") : qsTr("End of this track"); onTriggered: Player.setSleep(-1) }
        }
        Menu {
            title: qsTr("Speed · %1×").arg(Player.rate.toFixed(2).replace(/\.?0+$/, ""))
            Repeater { model: [.75, 1, 1.25, 1.5, 1.75, 2]
                MenuItem { required property real modelData; text: modelData + "×"; checkable: true; checked: Math.abs(Player.rate - modelData) < .01; onTriggered: Player.rate = modelData } }
        }
    }

    // ---------- wide: immersive ----------
    Item {
        anchors.fill: parent; visible: fp.wide
        Image { id: bg; anchors.fill: parent; source: Player.current.cover || ""; fillMode: Image.PreserveAspectCrop; visible: false; sourceSize: Qt.size(720, 450); asynchronous: true }
        MultiEffect { anchors.fill: parent; source: bg; visible: bg.status === Image.Ready; blurEnabled: true; blur: 1; blurMax: 64; brightness: Theme.light ? .2 : -.45; saturation: -.2; opacity: .55 }
        RowLayout {
            anchors.fill: parent; anchors.margins: 40; spacing: 40
            Cover { Layout.preferredWidth: Math.min(fp.height - 80, fp.width * .42); Layout.preferredHeight: Layout.preferredWidth; Layout.alignment: Qt.AlignVCenter
                r: 36; source: Player.current.cover || ""; title: Player.current.album || Player.current.title || ""; icon: Player.current.kind === "radio" ? "radio" : "" }
            ColumnLayout {
                Layout.fillWidth: true; Layout.fillHeight: true; spacing: 12
                RowLayout { Layout.fillWidth: true
                    MText { text: Player.current.kind === "radio" ? qsTr("Live radio") : fp.book ? qsTr("Chapter %1 of %2").arg(Player.index + 1).arg(Player.queue.length) : Player.current.album ? qsTr("Playing from %1").arg(Player.current.album) : ""
                        role: "label"; color: fp.s.primary; Layout.fillWidth: true; elide: Text.ElideRight }
                    TintBtn { icon: "more_vert"; tip: qsTr("More"); onClicked: moreMenu.popup() }
                    TintBtn { icon: "close_fullscreen"; tip: qsTr("Close"); onClicked: fp.close() } }
                MText { Layout.fillWidth: true; text: Player.current.nowPlaying || Player.current.title || ""; font.pixelSize: Math.min(54, fp.width / 26); font.weight: Font.Black; font.variableAxes: ({"wdth": 115, "ROND": 60})
                    color: fp.s.on_surface; wrapMode: Text.WordWrap; maximumLineCount: 3; elide: Text.ElideRight }
                MText { text: Player.current.kind === "radio" ? (Player.current.title || "") : Player.current.artist || ""; role: "title"; color: fp.s.on_surface_variant }
                Transport { Layout.fillWidth: true; Layout.topMargin: 8; s: fp.s; size: 64 }
                RowLayout { Layout.fillWidth: true; spacing: 12
                    Glyph { name: Player.muted || Player.volume === 0 ? "volume_off" : "volume_up"; color: fp.s.on_surface_variant; TapHandler { onTapped: Player.muted = !Player.muted } }
                    ExpressiveSlider { Layout.preferredWidth: 200; from: 0; to: 100; value: Player.volume * 100; compact: true; showIcon: false; activeColor: fp.s.primary; inactiveColor: fp.s.secondary_container; onMoved: Player.volume = value / 100; Accessible.name: qsTr("Volume") }
                    Item { Layout.fillWidth: true }
                    SplitPill { Layout.preferredWidth: 420; s: fp.s; h: 44; items: fp.pillItems; onPicked: id => fp.pick(id) } }
                Rectangle {
                    Layout.fillWidth: true; Layout.fillHeight: true; Layout.topMargin: 6; radius: 28; color: Theme.alpha(fp.s.surface_container, .85)
                    ColumnLayout { anchors.fill: parent; anchors.margins: 14; spacing: 6
                        MText { text: fp.panel === "lyrics" ? qsTr("Lyrics") : fp.book ? qsTr("Chapters") : qsTr("Up next"); role: "titleS"; color: fp.s.on_surface; Layout.leftMargin: 8 }
                        Loader { Layout.fillWidth: true; Layout.fillHeight: true; sourceComponent: fp.panel === "lyrics" ? lyricsComp : queueComp } } }
            }
        }
    }

    // ---------- narrow: full bleed ----------
    Item {
        anchors.fill: parent; visible: !fp.wide
        Item {
            anchors.left: parent.left; anchors.right: parent.right; height: Math.min(parent.width * 1.05, parent.height * .6)
            visible: fp.panel === ""
            Cover { anchors.fill: parent; r: 0; source: Player.current.cover || ""; title: Player.current.album || Player.current.title || ""; icon: Player.current.kind === "radio" ? "radio" : "" }
            Rectangle { anchors.left: parent.left; anchors.right: parent.right; anchors.bottom: parent.bottom; height: parent.height * .55
                gradient: Gradient { GradientStop { position: 0; color: "transparent" } GradientStop { position: 1; color: fp.s.surface } } }
        }
        Loader {
            visible: fp.panel !== ""; active: visible
            anchors { left: parent.left; right: parent.right; top: parent.top; topMargin: 72; bottom: narrowCol.top; margins: 20 }
            sourceComponent: fp.panel === "lyrics" ? lyricsComp : queueComp
        }
        RowLayout {
            anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top; anchors.margins: 16
            TintBtn { icon: "keyboard_arrow_down"; tip: qsTr("Close"); dark: fp.panel === ""; onClicked: fp.close() }
            Item { Layout.fillWidth: true }
            TintBtn { icon: "more_vert"; tip: qsTr("More"); dark: fp.panel === ""; onClicked: moreMenu.popup() }
        }
        ColumnLayout {
            id: narrowCol
            anchors.left: parent.left; anchors.right: parent.right; anchors.bottom: parent.bottom; anchors.margins: 20; anchors.bottomMargin: 28; spacing: 12
            MText { text: Player.current.kind === "radio" ? qsTr("Live radio") : Player.current.album || ""; role: "label"; color: fp.s.primary; Layout.fillWidth: true; elide: Text.ElideRight }
            MText { Layout.fillWidth: true; text: Player.current.nowPlaying || Player.current.title || ""; font.pixelSize: fp.panel ? 22 : 34; font.weight: Font.Black; font.variableAxes: ({"wdth": 115, "ROND": 60})
                wrapMode: Text.WordWrap; maximumLineCount: 2; elide: Text.ElideRight; color: fp.s.on_surface }
            MText { text: Player.current.kind === "radio" ? (Player.current.title || "") : Player.current.artist || ""; role: "title"; color: fp.s.on_surface_variant; Layout.fillWidth: true; elide: Text.ElideRight }
            Transport { Layout.fillWidth: true; s: fp.s; size: 66 }
            SplitPill { Layout.fillWidth: true; s: fp.s; items: fp.pillItems; onPicked: id => fp.pick(id) }
        }
    }
    Component { id: lyricsComp; LyricsView { s: fp.s } }
    Component { id: queueComp; QueueList { s: fp.s } }
    component TintBtn: Rectangle {
        id: tb
        property string icon
        property string tip
        property bool dark: false
        signal clicked()
        implicitWidth: 40; implicitHeight: 40; radius: 20
        color: dark ? Qt.rgba(0, 0, 0, .35) : fp.s.surface_container_high
        Glyph { anchors.centerIn: parent; name: tb.icon; color: tb.dark ? "#ffffff" : fp.s.on_surface }
        HoverHandler { cursorShape: Qt.PointingHandCursor }
        TapHandler { onTapped: tb.clicked() }
        Accessible.role: Accessible.Button; Accessible.name: tip
    }
}
