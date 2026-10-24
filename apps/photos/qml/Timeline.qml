import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Opal.Material
import Opal.Photos
// A justified timeline of one filter: all photos, an album, screenshots, favourites or videos. Header, search, groups, rows and scrubber.
Item {
    id: tl
    property alias filter: pv.filter
    property alias album: pv.album
    property string heading: qsTr("Photos")
    property string subheading: ""
    property bool showBack: false
    property string emptyIcon: "photo_library"
    property string emptyTitle: qsTr("No photos yet")
    property string emptyText: ""
    readonly property alias view: pv
    readonly property alias query: search.text
    property int current: -1
    property int anchor: -1
    property real zoom: 1
    signal back()
    signal openPhoto(int index)
    signal slideshow()
    signal addFolder()
    readonly property bool phone: width < 600
    readonly property bool couch: Posture.couch
    readonly property bool selecting: pv.selectedCount > 0
    readonly property real baseRow: couch ? 260 : phone ? 118 : 190
    function focusSearch() { search.input.forceActiveFocus(); search.input.selectAll(); }
    function clearSearch() { search.text = ""; }
    function reveal(i) { if (i >= 0) list.positionViewAtIndex(pv.rowOf(i), ListView.Contain); }
    function move(dir) {
        const n = current < 0 ? (pv.count ? 0 : -1) : pv.neighbour(current, dir);
        if (n >= 0) { current = n; reveal(n); }
    }
    // Clicks: plain opens (or toggles while selecting), Ctrl toggles, Shift selects a range.
    function activate(i, mods) {
        if (mods & Qt.ShiftModifier && anchor >= 0) { pv.selectRange(anchor, i); }
        else if (mods & Qt.ControlModifier || selecting) { pv.toggle(i); anchor = i; }
        else { current = i; tl.openPhoto(i); }
    }
    PhotoView {
        id: pv
        width: list.width
        group: groupPick.current
        query: search.text
        rowHeight: tl.baseRow * tl.zoom
        gap: tl.couch ? 14 : tl.phone ? 3 : 4
        headerHeight: tl.couch ? 76 : tl.phone ? 44 : 56
        onFilterChanged: { tl.current = -1; list.positionViewAtBeginning(); }
    }
    Keys.onPressed: e => {
        const k = e.key;
        if (k === Qt.Key_Left) { move("left"); e.accepted = true; }
        else if (k === Qt.Key_Right) { move("right"); e.accepted = true; }
        else if (k === Qt.Key_Up) { move("up"); e.accepted = true; }
        else if (k === Qt.Key_Down) { move("down"); e.accepted = true; }
        else if ((k === Qt.Key_Return || k === Qt.Key_Enter) && current >= 0) { tl.openPhoto(current); e.accepted = true; }
        else if (k === Qt.Key_Space && current >= 0) { pv.toggle(current); e.accepted = true; }
        else if (k === Qt.Key_Menu && current >= 0) { PhotoLibrary.toggleFavourite(pv.path(current)); e.accepted = true; }
        else if (k === Qt.Key_Search) { tl.slideshow(); e.accepted = true; }
        else if (k === Qt.Key_Escape && selecting) { pv.clearSelection(); e.accepted = true; }
        else if (k === Qt.Key_A && e.modifiers & Qt.ControlModifier) { pv.selectAll(); e.accepted = true; }
        else if (k === Qt.Key_Plus || k === Qt.Key_Equal) { tl.zoom = Math.min(1.8, tl.zoom + .15); e.accepted = true; }
        else if (k === Qt.Key_Minus) { tl.zoom = Math.max(.55, tl.zoom - .15); e.accepted = true; }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.topMargin: tl.phone ? 8 : 14
        spacing: tl.phone ? 6 : 12
        // ---------- header ----------
        RowLayout {
            Layout.fillWidth: true; Layout.leftMargin: tl.phone ? 12 : 4; Layout.rightMargin: tl.phone ? 8 : 16
            spacing: 10
            MButton { visible: tl.showBack; icon: "arrow_back"; tooltip: qsTr("Back"); compact: true; onClicked: tl.back() }
            ColumnLayout {
                spacing: 0; Layout.fillWidth: tl.phone
                MText { text: tl.heading; role: tl.phone ? "headline" : tl.couch ? "displayS" : "headline"; elide: Text.ElideRight; Layout.maximumWidth: tl.phone ? tl.width - 120 : 420 }
                MText { visible: text !== ""; text: tl.subheading || (pv.count ? qsTr("%n item(s)", "", pv.count) : ""); role: "labelS"; color: Theme.subtext }
            }
            Item { Layout.fillWidth: !tl.phone }
            SearchField {
                id: search
                visible: !tl.phone || searchOpen.checked || text !== ""
                Layout.preferredWidth: tl.phone ? -1 : Math.min(440, tl.width * .38)
                Layout.fillWidth: tl.phone
                placeholder: PhotoLibrary.ocrEnabled ? qsTr("Search photos, folders, text in screenshots") : qsTr("Search photos and folders")
                MButton { visible: search.text !== ""; icon: "close"; compact: true; tooltip: qsTr("Clear"); onClicked: search.text = "" }
                Keys.onEscapePressed: { search.text = ""; tl.forceActiveFocus(); }
                Keys.onDownPressed: { tl.forceActiveFocus(); if (tl.current < 0) tl.move("right"); }
            }
            MButton { id: searchOpen; visible: tl.phone && !search.visible; icon: "search"; compact: true; tooltip: qsTr("Search"); onClicked: { checked = true; search.input.forceActiveFocus(); } }
            SegmentedButtons {
                id: groupPick
                visible: !tl.phone
                current: "day"
                options: [{id: "year", name: qsTr("Years")}, {id: "month", name: qsTr("Months")}, {id: "day", name: qsTr("Days")}]
                onPicked: id => current = id
            }
            MButton { icon: "slideshow"; compact: true; tooltip: qsTr("Slideshow"); enabled: pv.count > 0; onClicked: tl.slideshow() }
        }
        // Scan progress while the first scan runs, and OCR progress after.
        RowLayout {
            visible: PhotoLibrary.scanning && PhotoLibrary.count > 0
            Layout.leftMargin: tl.phone ? 16 : 6; spacing: 10
            WavyProgress { Layout.preferredWidth: 120; indeterminate: true }
            MText { text: qsTr("Looking for new photos… %1").arg(PhotoLibrary.scanFound || ""); role: "labelS"; color: Theme.subtext }
        }

        // ---------- rows ----------
        Item {
            Layout.fillWidth: true; Layout.fillHeight: true
            ListView {
                id: list
                anchors.fill: parent
                anchors.leftMargin: tl.phone ? 4 : 4
                anchors.rightMargin: scrubber.visible ? scrubber.width : tl.phone ? 4 : 16
                clip: true
                model: pv
                cacheBuffer: 1200
                boundsBehavior: Flickable.StopAtBounds
                ScrollBar.vertical: ScrollBar { policy: scrubber.visible ? ScrollBar.AlwaysOff : ScrollBar.AsNeeded }
                bottomMargin: tl.selecting ? 96 : 24
                delegate: Item {
                    id: row
                    required property string kind
                    required property string title
                    required property string subtitle
                    required property string key
                    required property real rowHeight
                    required property var items
                    required property int index
                    width: list.width; height: rowHeight
                    // Group header: date, count and a check that selects the whole group.
                    RowLayout {
                        visible: row.kind === "header"
                        anchors { left: parent.left; right: parent.right; bottom: parent.bottom; bottomMargin: tl.phone ? 6 : 10; leftMargin: tl.phone ? 8 : 4 }
                        spacing: 10
                        MText { text: row.title; role: tl.couch ? "headline" : tl.phone ? "titleS" : "title" }
                        MText { text: row.subtitle; role: "labelS"; color: Theme.subtext; elide: Text.ElideRight; Layout.fillWidth: true }
                        Rectangle {
                            id: groupCheck
                            readonly property bool on: tl.selecting && pv.groupSelected(row.key) && pv.selectedCount >= 0
                            visible: tl.selecting || headHover.hovered
                            implicitWidth: 28; implicitHeight: 28; radius: 14
                            color: on ? Theme.primary : "transparent"; border.width: on ? 0 : 2; border.color: Theme.subtext
                            Glyph { anchors.centerIn: parent; visible: groupCheck.on; name: "check"; font.pixelSize: 18; color: Theme.onPrimary }
                            TapHandler { onTapped: pv.toggleGroup(row.key) }
                            Accessible.role: Accessible.CheckBox; Accessible.name: qsTr("Select %1").arg(row.title); Accessible.checked: on
                        }
                    }
                    HoverHandler { id: headHover; enabled: row.kind === "header" }
                    Repeater {
                        model: row.kind === "photos" ? row.items : []
                        PhotoTile {
                            required property var modelData
                            item: modelData
                            x: modelData.x; width: modelData.w; height: row.rowHeight
                            radius: tl.couch ? 22 : tl.phone ? 6 : 12
                            selecting: tl.selecting
                            focused: tl.current === modelData.i && (tl.activeFocus || Posture.controllerActive)
                            onActivated: mouse => tl.activate(modelData.i, mouse.modifiers || 0)
                            onHeld: { pv.select(modelData.i, true); tl.anchor = modelData.i; }
                        }
                    }
                }
                // Ctrl + wheel and pinch change the row height.
                WheelHandler {
                    acceptedModifiers: Qt.ControlModifier
                    onWheel: e => tl.zoom = Math.max(.55, Math.min(1.8, tl.zoom + (e.angleDelta.y > 0 ? .1 : -.1)))
                }
                PinchHandler {
                    target: null
                    property real startZoom: 1
                    onActiveChanged: if (active) startZoom = tl.zoom
                    onActiveScaleChanged: if (active) tl.zoom = Math.max(.55, Math.min(1.8, startZoom * activeScale))
                }
            }
            Scrubber {
                id: scrubber
                visible: !tl.phone && marks.length > 1 && contentHeight > viewHeight * 1.5
                anchors { right: parent.right; top: parent.top; bottom: parent.bottom; bottomMargin: 24 }
                marks: pv.marks; contentY: list.contentY; contentHeight: pv.contentHeight; viewHeight: list.height
                onJump: (r, y) => list.positionViewAtIndex(r, ListView.Beginning)
            }

            // ---------- empty and loading ----------
            ColumnLayout {
                anchors.centerIn: parent; width: Math.min(parent.width - 48, 420); spacing: 14
                visible: pv.count === 0
                LoadingIndicator { visible: !PhotoLibrary.loaded || PhotoLibrary.scanning && PhotoLibrary.count === 0; Layout.alignment: Qt.AlignHCenter; implicitWidth: 64; implicitHeight: 64; contained: true }
                Rectangle {
                    visible: PhotoLibrary.loaded && !(PhotoLibrary.scanning && PhotoLibrary.count === 0)
                    Layout.alignment: Qt.AlignHCenter; implicitWidth: 96; implicitHeight: 96; radius: 34; color: Theme.secondaryContainer
                    Glyph { anchors.centerIn: parent; name: tl.query ? "search_off" : tl.emptyIcon; font.pixelSize: 48; color: Theme.onSecondaryContainer }
                }
                MText {
                    Layout.alignment: Qt.AlignHCenter
                    text: !PhotoLibrary.loaded || PhotoLibrary.scanning && PhotoLibrary.count === 0 ? qsTr("Looking for photos… %1").arg(PhotoLibrary.scanFound || "")
                        : tl.query ? qsTr("Nothing matches “%1”").arg(tl.query) : tl.emptyTitle
                    role: "title"
                }
                MText {
                    visible: text !== "" && PhotoLibrary.loaded
                    Layout.fillWidth: true; horizontalAlignment: Text.AlignHCenter; wrapMode: Text.Wrap
                    text: tl.query ? (PhotoLibrary.ocrEnabled ? qsTr("Search looks at names, folders, dates, captions, cameras and text in screenshots.") : qsTr("Search looks at names, folders, dates, captions and cameras.")) : tl.emptyText
                    role: "body"; color: Theme.subtext
                }
                MButton { visible: !tl.query && tl.filter === "all" && PhotoLibrary.loaded && !PhotoLibrary.scanning; Layout.alignment: Qt.AlignHCenter; text: qsTr("Add a folder"); icon: "create_new_folder"; filled: true; onClicked: tl.addFolder() }
            }
        }
    }
}
