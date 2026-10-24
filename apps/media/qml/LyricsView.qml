import QtQuick
import QtQuick.Controls
import Opal.Material
import Opal.Media
// Lyrics: synced lines follow the song and a click seeks there; plain lyrics scroll freely.
ListView {
    id: lv
    property var s: null
    readonly property bool synced: Player.lyrics.length > 0 && Player.lyrics[0].time >= 0
    clip: true; spacing: 10
    boundsBehavior: Flickable.StopAtBounds
    model: Player.lyrics
    highlightRangeMode: synced ? ListView.ApplyRange : ListView.NoHighlightRange
    preferredHighlightBegin: height * .35; preferredHighlightEnd: height * .45
    highlightMoveDuration: Theme.motion(400)
    currentIndex: synced ? Player.lyricIndex : -1
    ScrollBar.vertical: ScrollBar {}
    delegate: MText {
        required property var modelData
        required property int index
        readonly property bool cur: lv.synced && index === Player.lyricIndex
        width: lv.width
        text: modelData.text || " "
        wrapMode: Text.Wrap
        font.pixelSize: lv.synced ? 24 : 17; font.weight: cur ? Font.Black : Font.DemiBold
        color: lv.s ? (cur ? lv.s.on_surface : lv.s.on_surface_variant) : (cur ? Theme.text : Theme.subtext)
        opacity: !lv.synced || cur ? 1 : index < Player.lyricIndex ? .45 : .7
        Behavior on opacity { NumberAnimation { duration: Theme.motion(Theme.durShort) } }
        TapHandler { enabled: lv.synced; onTapped: Player.seek(parent.modelData.time) }
    }
    Column {
        anchors.centerIn: parent; visible: lv.count === 0; spacing: 8
        Glyph { anchors.horizontalCenter: parent.horizontalCenter; name: "lyrics"; font.pixelSize: 40; color: lv.s ? lv.s.on_surface_variant : Theme.subtext }
        MText { anchors.horizontalCenter: parent.horizontalCenter; text: qsTr("No lyrics for this song"); color: lv.s ? lv.s.on_surface_variant : Theme.subtext }
        MText { anchors.horizontalCenter: parent.horizontalCenter; text: qsTr("Put an .lrc file with the same name next to it"); role: "labelS"; color: lv.s ? lv.s.on_surface_variant : Theme.subtext }
    }
}
