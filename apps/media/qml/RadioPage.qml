import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Opal.Material
import Opal.Media
// Radio: your stations, then the directory (popular, or what you search). A station of your own takes a name and a stream link.
Item {
    id: page
    property var app
    property bool adding: false
    readonly property bool phone: width < 600
    Component.onCompleted: if (Radio.results.length === 0) Radio.popular()
    ColumnLayout {
        anchors.fill: parent; anchors.topMargin: page.phone ? 8 : 18; spacing: 12
        RowLayout {
            Layout.fillWidth: true; Layout.leftMargin: page.phone ? 16 : 8; Layout.rightMargin: 16; spacing: 10
            MText { text: qsTr("Radio"); role: page.phone ? "headline" : "displayS"; Layout.fillWidth: page.phone }
            Item { Layout.fillWidth: !page.phone }
            SearchField { id: search; Layout.preferredWidth: page.phone ? -1 : Math.min(360, page.width * .34); Layout.fillWidth: page.phone; placeholder: qsTr("Search stations"); onAccepted: Radio.search(text)
                MButton { visible: search.text !== ""; icon: "close"; compact: true; tooltip: qsTr("Clear"); onClicked: { search.text = ""; Radio.popular(); } } }
            MButton { icon: "add_link"; compact: true; tooltip: qsTr("Add a station by its link"); onClicked: page.adding = true }
        }
        ListView {
            id: list
            Layout.fillWidth: true; Layout.fillHeight: true; Layout.leftMargin: page.phone ? 8 : 0; Layout.rightMargin: 12
            clip: true; spacing: 4; boundsBehavior: Flickable.StopAtBounds; bottomMargin: 100
            model: Radio.results
            ScrollBar.vertical: ScrollBar {}
            header: ColumnLayout {
                width: list.width; spacing: 10
                MText { visible: Radio.favourites.length > 0; text: qsTr("Your stations"); role: "title"; Layout.leftMargin: 8 }
                Flow {
                    visible: Radio.favourites.length > 0
                    Layout.fillWidth: true; spacing: 10
                    Repeater { model: Radio.favourites
                        Station { required property var modelData; station: modelData; width: page.phone ? list.width : Math.min(360, (list.width - 20) / 3) } }
                }
                RowLayout { Layout.fillWidth: true; Layout.topMargin: 6
                    MText { text: search.text ? qsTr("Results") : qsTr("Popular stations"); role: "title"; Layout.leftMargin: 8 }
                    WavyProgress { visible: Radio.busy; Layout.preferredWidth: 100; indeterminate: true } }
                MText { visible: Radio.error !== ""; text: Radio.error; color: Theme.error; Layout.leftMargin: 8 }
            }
            delegate: Station { required property var modelData; station: modelData; width: list.width }
        }
    }
    Sheet {
        open: page.adding
        sheetHeight: 320
        onDismissed: page.adding = false
        ColumnLayout {
            anchors.fill: parent; anchors.margins: 24; spacing: 12
            MText { text: qsTr("Add a station"); role: "title" }
            TextField { id: stName; Layout.fillWidth: true; placeholderText: qsTr("Name"); color: Theme.text; font.family: Theme.font }
            TextField { id: stUrl; Layout.fillWidth: true; placeholderText: qsTr("Stream link, like https://…/stream.mp3"); color: Theme.text; font.family: Theme.font }
            Item { Layout.fillHeight: true }
            RowLayout { Layout.alignment: Qt.AlignRight; spacing: 8
                MButton { text: qsTr("Cancel"); onClicked: page.adding = false }
                MButton { text: qsTr("Add"); filled: true; enabled: stUrl.text.length > 8
                    onClicked: { if (Radio.addCustom(stName.text, stUrl.text)) { page.adding = false; stName.text = ""; stUrl.text = ""; } else page.app.toast(qsTr("That is not a stream link")); } } }
        }
    }
    component Station: Rectangle {
        id: st
        property var station: ({})
        readonly property bool fav: Radio.favourites.length >= 0 && Radio.isFavourite(station.url || "")
        readonly property bool on: Player.hasMedia && Player.current.url === station.url
        height: 68; radius: 34
        color: on ? Theme.secondaryContainer : stHover.hovered ? Theme.surfaceHigh : Theme.surface
        RowLayout {
            anchors.fill: parent; anchors.leftMargin: 10; anchors.rightMargin: 8; spacing: 12
            Cover { Layout.preferredWidth: 48; Layout.preferredHeight: 48; r: 24; source: st.station.cover || ""; title: st.station.name || ""; icon: "radio" }
            ColumnLayout { Layout.fillWidth: true; spacing: 0
                MText { Layout.fillWidth: true; text: st.station.name || ""; role: "titleS"; elide: Text.ElideRight }
                MText { Layout.fillWidth: true; text: st.on && Player.current.nowPlaying ? Player.current.nowPlaying : [st.station.artist, st.station.country, st.station.bitrate ? st.station.bitrate + " kbps" : ""].filter(x => !!x).join(" · ")
                    role: "labelS"; color: Theme.subtext; elide: Text.ElideRight } }
            MButton { icon: "favorite"; compact: true; checked: st.fav; ink: st.fav ? Theme.primary : Theme.subtext; tooltip: st.fav ? qsTr("Remove from your stations") : qsTr("Add to your stations"); onClicked: Radio.toggleFavourite(st.station) }
            MButton { icon: st.on && Player.playing ? "stop" : "play_arrow"; compact: true; tonal: true; tooltip: st.on && Player.playing ? qsTr("Stop") : qsTr("Play")
                onClicked: st.on && Player.playing ? Player.pause() : Player.playRadio(st.station) }
        }
        HoverHandler { id: stHover }
        TapHandler { onTapped: st.on && Player.playing ? Player.pause() : Player.playRadio(st.station) }
        Accessible.role: Accessible.Button; Accessible.name: station.name || ""
    }
}
