import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Opal.Material
import Opal.Photos
// Details for one photo: caption, date, file, camera, place, dominant colours, text found in it, and Show in Files.
Rectangle {
    id: panel
    property string path: ""
    property bool sheet: false
    signal close()
    signal message(string text)
    color: sheet ? "transparent" : Theme.surfaceLow
    PhotoDetails { id: d; path: panel.path }
    Flickable {
        anchors.fill: parent; contentHeight: col.implicitHeight + 40; clip: true; boundsBehavior: Flickable.StopAtBounds
        ScrollBar.vertical: ScrollBar {}
        ColumnLayout {
            id: col
            x: 20; y: 16; width: parent.width - 40; spacing: 14
            RowLayout {
                Layout.fillWidth: true
                MText { text: qsTr("Details"); role: "title"; Layout.fillWidth: true }
                MButton { visible: !panel.sheet; icon: "close"; compact: true; tooltip: qsTr("Close"); onClicked: panel.close() }
            }
            // The caption lives in the file's xdg comment, which Dolphin and Nautilus show too.
            Rectangle {
                Layout.fillWidth: true; implicitHeight: Math.max(54, caption.implicitHeight + 28); radius: 18; color: Theme.surfaceHigh
                border.width: caption.activeFocus ? 2 : 0; border.color: Theme.primary
                TextArea {
                    id: caption
                    anchors.fill: parent; anchors.margins: 6
                    background: null; wrapMode: TextEdit.Wrap; color: Theme.text
                    placeholderText: qsTr("Add a caption"); placeholderTextColor: Theme.subtext
                    font.family: Theme.font; font.pixelSize: 14
                    text: d.caption
                    onEditingFinished: if (text !== d.caption) PhotoLibrary.setCaption(panel.path, text)
                    Keys.onReturnPressed: e => { if (e.modifiers & Qt.ShiftModifier) e.accepted = false; else { focus = false; PhotoLibrary.setCaption(panel.path, text); } }
                    Accessible.name: qsTr("Caption")
                }
            }
            InfoRow { icon: "calendar_today"; title: d.dateText ? d.dateText + (d.timeText ? ", " + d.timeText : "") : ""; detail: d.dateNote }
            InfoRow { icon: d.video ? "movie" : "image"; title: d.name; detail: [d.dimensions, d.megapixels, d.sizeText].filter(t => !!t).join(" · ") }
            InfoRow { icon: "folder"; title: d.folderName; detail: d.folderDisplay; action: "folder_open"; onActivated: PhotoActions.showInFiles(panel.path) }
            InfoRow { icon: "photo_camera"; title: d.camera; detail: [d.lens, d.exposure].filter(t => !!t).join(" · ") }
            InfoRow { icon: "location_on"; title: d.hasLocation ? d.locationText : ""; detail: d.hasLocation ? qsTr("Open in OpenStreetMap") : ""; action: "open_in_new"
                onActivated: Qt.openUrlExternally("https://www.openstreetmap.org/?mlat=" + d.latitude + "&mlon=" + d.longitude + "#map=15/" + d.latitude + "/" + d.longitude) }
            // Dominant colours: click one to copy its hex code.
            RowLayout {
                visible: d.colours.length > 0
                Layout.fillWidth: true; spacing: 14
                Glyph { name: "palette"; color: Theme.subtext; Layout.alignment: Qt.AlignTop }
                Flow { Layout.fillWidth: true; spacing: 8
                    Repeater { model: d.colours
                        Rectangle {
                            required property color modelData
                            width: 32; height: 32; radius: 16; color: modelData
                            border.width: 1; border.color: Theme.alpha(Theme.text, .15)
                            HoverHandler { id: swHover; cursorShape: Qt.PointingHandCursor }
                            TapHandler { onTapped: { PhotoActions.copyPath([String(parent.modelData)]); panel.message(qsTr("Copied %1").arg(String(parent.modelData))); } }
                            ToolTip.visible: swHover.hovered; ToolTip.text: String(modelData); ToolTip.delay: 400
                            Accessible.role: Accessible.Button; Accessible.name: qsTr("Copy colour %1").arg(String(modelData))
                        } } }
            }
            // Text that OCR found in a screenshot.
            ColumnLayout {
                visible: d.text !== ""
                Layout.fillWidth: true; spacing: 6
                RowLayout { Layout.fillWidth: true
                    MText { text: qsTr("Text in this picture"); role: "label"; color: Theme.primary; Layout.fillWidth: true }
                    MButton { icon: "content_copy"; compact: true; tooltip: qsTr("Copy text"); onClicked: { PhotoActions.copyPath([d.text]); panel.message(qsTr("Text copied")); } } }
                Rectangle { Layout.fillWidth: true; implicitHeight: ocr.implicitHeight + 24; radius: 14; color: Theme.surfaceHigh
                    MText { id: ocr; x: 12; y: 12; width: parent.width - 24; text: d.text.length > 900 ? d.text.slice(0, 900) + "…" : d.text; role: "bodyS"; wrapMode: Text.Wrap; color: Theme.subtext } }
            }
            MButton { Layout.fillWidth: true; Layout.topMargin: 6; text: qsTr("Show in Files"); icon: "folder_open"; tonal: true; onClicked: PhotoActions.showInFiles(panel.path) }
        }
    }
    component InfoRow: RowLayout {
        id: ir
        property string icon
        property string title
        property string detail
        property string action: ""
        signal activated()
        visible: title !== ""
        Layout.fillWidth: true; spacing: 14
        Glyph { name: ir.icon; color: Theme.subtext; Layout.alignment: Qt.AlignTop }
        ColumnLayout { Layout.fillWidth: true; spacing: 1
            MText { Layout.fillWidth: true; text: ir.title; role: "titleS"; font.pixelSize: 14; wrapMode: Text.WrapAnywhere; maximumLineCount: 3; elide: Text.ElideRight }
            MText { visible: ir.detail !== ""; Layout.fillWidth: true; text: ir.detail; role: "labelS"; color: Theme.subtext; wrapMode: Text.Wrap } }
        MButton { visible: ir.action !== ""; icon: ir.action; compact: true; tooltip: ir.detail || ir.title; onClicked: ir.activated() }
    }
}
