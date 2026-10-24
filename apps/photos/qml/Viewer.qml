import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Opal.Material
import Opal.Photos
// The photo viewer: a dark stage that swipes between photos and zooms, a floating toolbar, a filmstrip and the details panel.
Rectangle {
    id: viewer
    property var paths: []
    property int index: 0
    property bool detailsOpen: false
    property bool chrome: true
    signal closed()
    signal edit(string path)
    signal slideshow(int index)
    signal message(string text)
    readonly property bool phone: width < 600
    readonly property string path: paths.length ? paths[Math.max(0, Math.min(index, paths.length - 1))] : ""
    readonly property var photo: path && PhotoLibrary.revision >= 0 ? PhotoLibrary.photo(path) : ({})
    property bool favourite: false
    function refreshFavourite() { favourite = path ? PhotoLibrary.isFavourite(path) : false; }
    onPathChanged: refreshFavourite()
    Connections { target: PhotoLibrary; function onFavouritesChanged() { viewer.refreshFavourite(); } }
    Component.onCompleted: { refreshFavourite(); stage.positionViewAtIndex(index, ListView.Center); stage.currentIndex = index; forceActiveFocus(); }
    color: "#0b0b0b"
    function go(delta) { const n = Math.max(0, Math.min(paths.length - 1, index + delta)); if (n !== index) { index = n; stage.currentIndex = n; } }
    function remove() {
        const p = path;
        PhotoActions.trash([p]);
        const list = paths.slice(); list.splice(index, 1); paths = list;
        if (!paths.length) { closed(); return; }
        index = Math.min(index, paths.length - 1); stage.currentIndex = index;
    }
    // The chrome fades after a few seconds of no input; any move or key brings it back.
    Timer { id: idle; interval: 3500; running: viewer.chrome && !viewer.detailsOpen && !viewer.phone; onTriggered: viewer.chrome = false }
    function wake() { chrome = true; idle.restart(); }
    HoverHandler { onPointChanged: viewer.wake() }
    Keys.onPressed: e => {
        wake();
        const k = e.key;
        if (k === Qt.Key_Left) { go(-1); e.accepted = true; }
        else if (k === Qt.Key_Right) { go(1); e.accepted = true; }
        else if (k === Qt.Key_Escape || k === Qt.Key_Back) { if (detailsOpen && phone) detailsOpen = false; else closed(); e.accepted = true; }
        else if (k === Qt.Key_Home) { index = 0; stage.currentIndex = 0; e.accepted = true; }
        else if (k === Qt.Key_End) { index = paths.length - 1; stage.currentIndex = index; e.accepted = true; }
        else if (k === Qt.Key_F || k === Qt.Key_Menu) { PhotoLibrary.toggleFavourite(path); e.accepted = true; }
        else if (k === Qt.Key_I) { detailsOpen = !detailsOpen; e.accepted = true; }
        else if (k === Qt.Key_E && !photo.video) { edit(path); e.accepted = true; }
        else if (k === Qt.Key_Delete) { remove(); e.accepted = true; }
        else if (k === Qt.Key_R) { PhotoActions.rotate([path], e.modifiers & Qt.ShiftModifier ? 3 : 1); e.accepted = true; }
        else if (k === Qt.Key_C && e.modifiers & Qt.ControlModifier) { PhotoActions.copy([path]); e.accepted = true; }
        else if (k === Qt.Key_Space || k === Qt.Key_Search) { slideshow(index); e.accepted = true; }
        else if (k === Qt.Key_0 || k === Qt.Key_Return && stage.currentItem) { if (stage.currentItem) stage.currentItem.toggleZoom(); e.accepted = true; }
    }

    RowLayout {
        anchors.fill: parent; spacing: 0
        Item {
            id: stageBox
            Layout.fillWidth: true; Layout.fillHeight: true
            // ---------- stage: one page per photo, swipe or arrow keys ----------
            ListView {
                id: stage
                anchors.fill: parent
                orientation: ListView.Horizontal
                snapMode: ListView.SnapOneItem
                highlightRangeMode: ListView.StrictlyEnforceRange
                highlightMoveDuration: Theme.motion(260)
                boundsBehavior: Flickable.StopAtBounds
                interactive: !(currentItem && currentItem.zoomed)
                model: viewer.paths
                cacheBuffer: 0
                onCurrentIndexChanged: if (currentIndex >= 0 && currentIndex !== viewer.index) viewer.index = currentIndex
                delegate: Item {
                    id: page
                    required property string modelData
                    required property int index
                    width: stage.width; height: stage.height
                    readonly property bool near: Math.abs(index - viewer.index) <= 1
                    readonly property var info: near ? PhotoLibrary.photo(modelData) : ({})
                    readonly property bool zoomed: flick.contentWidth > flick.width + 1 || flick.contentHeight > flick.height + 1
                    readonly property real reserve: viewer.phone ? 0 : (viewer.chrome ? 150 : 40)
                    // Fit size: the photo inside the page, minus room for the toolbar and the filmstrip.
                    readonly property real aspect: info.aspect || (full.implicitHeight > 0 ? full.implicitWidth / full.implicitHeight : 1.5)
                    readonly property real fitW: Math.min(width - (viewer.phone ? 0 : 96), (height - reserve - (viewer.phone ? 0 : 72)) * aspect)
                    readonly property real fitH: fitW / aspect
                    function toggleZoom() {
                        if (zoomed) { flick.resizeContent(flick.width, flick.height, Qt.point(flick.width / 2, flick.height / 2)); flick.returnToBounds(); }
                        else { const s = Math.max(2, (info.width || full.sourceSize.width) / fitW); flick.resizeContent(flick.width * s, flick.height * s, Qt.point(flick.width / 2, flick.height / 2)); }
                    }
                    Flickable {
                        id: flick
                        anchors.fill: parent
                        contentWidth: width; contentHeight: height
                        interactive: page.zoomed
                        boundsBehavior: Flickable.StopAtBounds
                        clip: true
                        Item {
                            width: flick.contentWidth; height: flick.contentHeight
                            readonly property real s: flick.contentWidth / flick.width
                            // A quick thumbnail first, then the full picture over it.
                            Image {
                                id: thumb
                                anchors.centerIn: parent; anchors.verticalCenterOffset: viewer.phone ? 0 : -page.reserve / 2 * parent.s
                                width: page.fitW * parent.s; height: page.fitH * parent.s
                                source: page.near ? (page.info.thumb || "") : ""
                                fillMode: Image.PreserveAspectFit; asynchronous: true
                                visible: full.status !== Image.Ready
                            }
                            Image {
                                id: full
                                anchors.fill: thumb
                                source: page.near && !page.info.video ? PhotoLibrary.uri(page.modelData) : ""
                                fillMode: Image.PreserveAspectFit; asynchronous: true; autoTransform: true; cache: false
                                // Decode at the screen size until zoomed past it.
                                sourceSize: Qt.size(Math.min(8192, Math.max(stage.width, stage.height) * (page.zoomed ? 3 : 1.5)), Math.min(8192, Math.max(stage.width, stage.height) * (page.zoomed ? 3 : 1.5)))
                                smooth: true; mipmap: true
                            }
                            // Videos play in the system player; Photos shows a play button.
                            Rectangle {
                                visible: !!page.info.video
                                anchors.centerIn: thumb; width: 88; height: 88; radius: 44; color: Qt.rgba(0, 0, 0, .55)
                                Glyph { anchors.centerIn: parent; name: "play_arrow"; fill: 1; font.pixelSize: 52; color: "#ffffff" }
                                TapHandler { onTapped: PhotoActions.openWith(page.modelData) }
                                Accessible.role: Accessible.Button; Accessible.name: qsTr("Play video")
                            }
                            TapHandler {
                                onDoubleTapped: page.toggleZoom()
                                onTapped: if (viewer.phone) viewer.chrome = !viewer.chrome; else viewer.wake()
                            }
                        }
                        PinchHandler {
                            target: null
                            property real startW: 0
                            onActiveChanged: { if (active) startW = flick.contentWidth; else flick.returnToBounds(); }
                            onActiveScaleChanged: if (active) { const w = Math.max(flick.width, Math.min(flick.width * 8, startW * activeScale)); flick.resizeContent(w, w * flick.height / flick.width, centroid.position); }
                        }
                        WheelHandler {
                            onWheel: e => {
                                const f = e.angleDelta.y > 0 ? 1.18 : 1 / 1.18;
                                const w = Math.max(flick.width, Math.min(flick.width * 8, flick.contentWidth * f));
                                flick.resizeContent(w, w * flick.height / flick.width, Qt.point(e.x + flick.contentX, e.y + flick.contentY));
                                flick.returnToBounds();
                            }
                        }
                    }
                }
            }

            // ---------- top bar ----------
            RowLayout {
                anchors { left: parent.left; right: parent.right; top: parent.top; margins: viewer.phone ? 8 : 14 }
                opacity: viewer.chrome ? 1 : 0; visible: opacity > 0
                Behavior on opacity { NumberAnimation { duration: Theme.motion(Theme.durMedium) } }
                spacing: 8
                RoundBtn { icon: "arrow_back"; tip: qsTr("Back"); onClicked: viewer.closed() }
                ColumnLayout { spacing: 0; Layout.leftMargin: 6; Layout.fillWidth: true
                    MText { Layout.fillWidth: true; text: viewer.photo.taken ? Qt.formatDate(viewer.photo.taken, "d MMMM yyyy") : (viewer.photo.name || ""); role: "titleS"; color: "#ffffff"; elide: Text.ElideRight }
                    MText { Layout.fillWidth: true; text: (viewer.photo.taken ? Qt.formatTime(viewer.photo.taken, "hh:mm") + " · " : "") + (viewer.photo.folderName || ""); role: "labelS"; color: Qt.rgba(1, 1, 1, .7); elide: Text.ElideRight } }
                MText { visible: !viewer.phone && viewer.paths.length > 1; text: qsTr("%1 of %2").arg(viewer.index + 1).arg(viewer.paths.length); role: "label"; color: Qt.rgba(1, 1, 1, .7) }
                RoundBtn { icon: "slideshow"; tip: qsTr("Slideshow"); visible: viewer.paths.length > 1; onClicked: viewer.slideshow(viewer.index) }
                RoundBtn { icon: "more_vert"; tip: qsTr("More"); onClicked: more.popup() }
            }
            Menu {
                id: more
                MenuItem { text: qsTr("Open with another app"); onTriggered: PhotoActions.openWith(viewer.path) }
                MenuItem { text: qsTr("Show in Files"); onTriggered: PhotoActions.showInFiles(viewer.path) }
                MenuItem { text: qsTr("Copy path"); onTriggered: PhotoActions.copyPath([viewer.path]) }
                MenuItem { text: qsTr("Rotate left"); enabled: !viewer.photo.video; onTriggered: PhotoActions.rotate([viewer.path], 3) }
                MenuItem { text: qsTr("Rotate right"); enabled: !viewer.photo.video; onTriggered: PhotoActions.rotate([viewer.path], 1) }
                MenuItem { text: qsTr("Set as wallpaper"); enabled: !viewer.photo.video && PhotoActions.wallpaperTarget !== ""; onTriggered: PhotoActions.setWallpaper(viewer.path) }
            }

            // Side arrows on the desktop.
            RoundBtn { visible: !viewer.phone && viewer.chrome && viewer.index > 0; anchors.left: parent.left; anchors.leftMargin: 16; anchors.verticalCenter: parent.verticalCenter; anchors.verticalCenterOffset: -50; icon: "chevron_left"; tip: qsTr("Previous"); onClicked: viewer.go(-1) }
            RoundBtn { visible: !viewer.phone && viewer.chrome && viewer.index < viewer.paths.length - 1; anchors.right: parent.right; anchors.rightMargin: 16; anchors.verticalCenter: parent.verticalCenter; anchors.verticalCenterOffset: -50; icon: "chevron_right"; tip: qsTr("Next"); onClicked: viewer.go(1) }

            // ---------- floating toolbar ----------
            Rectangle {
                id: toolbar
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.bottom: strip.visible ? strip.top : parent.bottom; anchors.bottomMargin: viewer.phone ? 22 : 16
                opacity: viewer.chrome ? 1 : 0; visible: opacity > 0
                Behavior on opacity { NumberAnimation { duration: Theme.motion(Theme.durMedium) } }
                implicitHeight: viewer.phone ? 64 : 60; implicitWidth: tools.implicitWidth + 16; radius: height / 2
                color: Theme.alpha(Theme.surfaceHigh, .94)
                RowLayout {
                    id: tools
                    anchors.centerIn: parent; spacing: 2
                    ToolBtn { icon: "favorite"; on: viewer.favourite; tip: viewer.favourite ? qsTr("Remove from favourites") : qsTr("Add to favourites"); onClicked: PhotoLibrary.toggleFavourite(viewer.path) }
                    ToolBtn { icon: "edit"; tip: qsTr("Edit"); visible: !viewer.photo.video; onClicked: viewer.edit(viewer.path) }
                    ToolBtn { icon: "rotate_right"; tip: qsTr("Rotate"); visible: !viewer.photo.video && !viewer.phone; onClicked: PhotoActions.rotate([viewer.path], 1) }
                    ToolBtn { icon: "content_copy"; tip: qsTr("Copy"); onClicked: PhotoActions.copy([viewer.path]) }
                    ToolBtn { icon: "wallpaper"; tip: qsTr("Set as wallpaper"); visible: !viewer.photo.video && !viewer.phone && PhotoActions.wallpaperTarget !== ""; onClicked: PhotoActions.setWallpaper(viewer.path) }
                    ToolBtn { icon: "info"; on: viewer.detailsOpen; tip: qsTr("Details"); onClicked: viewer.detailsOpen = !viewer.detailsOpen }
                    ToolBtn { icon: "delete"; tip: qsTr("Move to the trash"); onClicked: viewer.remove() }
                }
            }

            // ---------- filmstrip ----------
            ListView {
                id: strip
                visible: !viewer.phone && viewer.paths.length > 1 && viewer.chrome
                anchors.horizontalCenter: parent.horizontalCenter; anchors.bottom: parent.bottom; anchors.bottomMargin: 14
                width: Math.min(parent.width - 48, contentWidth); height: 58
                orientation: ListView.Horizontal; spacing: 6; clip: true
                model: viewer.paths
                currentIndex: viewer.index
                highlightRangeMode: ListView.ApplyRange; preferredHighlightBegin: width / 2 - 42; preferredHighlightEnd: width / 2 + 42
                highlightMoveDuration: Theme.motion(240)
                delegate: Item {
                    required property string modelData
                    required property int index
                    readonly property bool cur: index === viewer.index
                    width: cur ? 84 : 54; height: 58
                    Behavior on width { NumberAnimation { duration: Theme.motion(Theme.durShort) } }
                    RoundedImage {
                        anchors.fill: parent; anchors.topMargin: cur ? 0 : 4; anchors.bottomMargin: cur ? 0 : 4
                        radius: cur ? 12 : 9; opacity: cur ? 1 : .55
                        source: PhotoLibrary.thumb(parent.modelData)
                        sourceSize: Qt.size(170, 116)
                    }
                    TapHandler { onTapped: { viewer.index = index; stage.currentIndex = index; } }
                    Accessible.role: Accessible.Button; Accessible.name: qsTr("Photo %1").arg(index + 1)
                }
            }
        }
        // ---------- details: a side panel on the desktop, a sheet on a phone ----------
        DetailsPanel {
            visible: viewer.detailsOpen && !viewer.phone
            Layout.preferredWidth: 360; Layout.fillHeight: true
            path: viewer.path
            onClose: viewer.detailsOpen = false
            onMessage: t => viewer.message(t)
        }
    }
    Sheet {
        open: viewer.detailsOpen && viewer.phone
        sheetHeight: viewer.height * .72
        onDismissed: viewer.detailsOpen = false
        DetailsPanel { anchors.fill: parent; path: viewer.path; sheet: true; onClose: viewer.detailsOpen = false; onMessage: t => viewer.message(t) }
    }

    component RoundBtn: Rectangle {
        id: rb
        property string icon
        property string tip
        signal clicked()
        implicitWidth: 44; implicitHeight: 44; radius: 22
        color: rbHover.hovered ? Qt.rgba(1, 1, 1, .2) : Qt.rgba(0, 0, 0, .35)
        Glyph { anchors.centerIn: parent; name: rb.icon; color: "#ffffff" }
        HoverHandler { id: rbHover; cursorShape: Qt.PointingHandCursor }
        TapHandler { onTapped: rb.clicked() }
        ToolTip.visible: rbHover.hovered && tip !== ""; ToolTip.text: tip; ToolTip.delay: 600
        Accessible.role: Accessible.Button; Accessible.name: tip
    }
    component ToolBtn: Rectangle {
        id: tbtn
        property string icon
        property string tip
        property bool on: false
        signal clicked()
        implicitWidth: viewer.phone ? 56 : 48; implicitHeight: viewer.phone ? 48 : 44; radius: height / 2
        color: on ? Theme.secondaryContainer : tbHover.hovered ? Theme.alpha(Theme.text, .08) : "transparent"
        Glyph { anchors.centerIn: parent; name: tbtn.icon; fill: tbtn.on ? 1 : 0; color: tbtn.on ? Theme.onSecondaryContainer : Theme.text }
        HoverHandler { id: tbHover; cursorShape: Qt.PointingHandCursor }
        TapHandler { onTapped: tbtn.clicked() }
        ToolTip.visible: tbHover.hovered; ToolTip.text: tip; ToolTip.delay: 600
        Accessible.role: Accessible.Button; Accessible.name: tip; Accessible.checked: on
    }
}
