import QtQuick
import QtQuick.Layouts
import Opal.Material
import Opal.Files

// Debug interface for the Files backend: places on the left, the folder (or search results) in the middle, operations at the bottom.
Rectangle {
    id: view
    implicitWidth: 1180
    implicitHeight: 760
    color: Theme.base

    readonly property string kindGlyphs: "folder image movie music_note picture_as_pdf folder_zip description code apps draft"
    function glyphFor(kind) {
        const map = { folder: "folder", image: "image", video: "movie", audio: "music_note", pdf: "picture_as_pdf", archive: "folder_zip", text: "description", code: "code", app: "apps" };
        return map[kind] || "draft";
    }
    readonly property bool searching: field.text.length > 0
    readonly property var shown: searching ? search : folder

    FolderModel { id: folder; location: FilesEnv.startLocation; thumbnailSize: 128 }
    Search { id: search; root: folder.location; query: field.text; mode: contentChip.selected ? "content" : "name" }
    Places { id: places }

    Timer { id: searchTimer; interval: 250; onTriggered: if (view.searching) search.start(); else search.clear() }
    Timer { id: thumbTimer; interval: 40; onTriggered: {
        const first = Math.max(0, list.indexAt(10, list.contentY));
        const last = list.indexAt(10, list.contentY + list.height - 1);
        view.shown.requestThumbnails(first, last < 0 ? view.shown.count - 1 : last);
    } }
    Component.onCompleted: {
        field.text = FilesEnv.query;
        if (FilesEnv.demoDest !== "") {
            FileOperations.copy([FilesEnv.startLocation + "/photos", FilesEnv.startLocation + "/report.txt"], FilesEnv.demoDest);
        }
    }

    RowLayout {
        anchors.fill: parent; anchors.margins: 12; spacing: 12

        // Places
        ListView {
            id: placeList
            Layout.preferredWidth: 248; Layout.fillHeight: true
            model: places; clip: true
            section.property: "section"
            section.delegate: MText { required property string section; text: section === "places" ? "" : section.charAt(0).toUpperCase() + section.slice(1); role: "label"; color: Theme.subtext; height: text === "" ? 6 : 30; verticalAlignment: Text.AlignBottom; leftPadding: 16; bottomPadding: 4 }
            delegate: ListRow {
                required property int index
                required property string name
                required property string uri
                required property string glyph
                required property string detail
                required property string kind
                required property bool canEject
                width: ListView.view.width
                title: name; icon: glyph; trailing: detail
                selected: uri === folder.location.toString() || (kind === "home" && folder.location.toString() === "file://" + uri.replace("file://", ""))
                onClicked: folder.location = uri
                Glyph { visible: canEject; name: "eject"; color: Theme.subtext; font.pixelSize: 18 }
                Accessible.name: name
            }
        }

        ColumnLayout {
            Layout.fillWidth: true; Layout.fillHeight: true; spacing: 10

            SearchField {
                id: field
                Layout.fillWidth: true
                placeholder: "Search in " + folder.locationName
                onTextChanged: searchTimer.restart()
            }
            RowLayout {
                spacing: 8
                Chip { text: "Hidden"; icon: "visibility"; selected: folder.showHidden; onClicked: folder.showHidden = !folder.showHidden }
                Chip { text: "Images"; icon: "image"; selected: folder.filterKind.length > 0; onClicked: folder.filterKind = folder.filterKind.length ? [] : ["image", "folder"] }
                Chip { text: "Modified"; icon: "schedule"; selected: folder.sortRole === FolderModel.Modified; onClicked: folder.sortRole = folder.sortRole === FolderModel.Modified ? FolderModel.Name : FolderModel.Modified }
                Chip { id: contentChip; visible: search.contentSupported; text: "Content"; icon: "manage_search"; selected: false; onClicked: selected = !selected }
                Chip { text: "New folder"; icon: "create_new_folder"; onClicked: FileOperations.createFolder(folder.location.toString()) }
                Chip { text: "Trash"; icon: "delete"; onClicked: FileOperations.trash(folder.selectedUris()) }
                Chip { text: FileOperations.canUndo ? FileOperations.undoText : "Undo"; icon: "undo"; selected: FileOperations.canUndo; onClicked: FileOperations.undo() }
                Item { Layout.fillWidth: true }
            }
            RowLayout {
                spacing: 8
                MText { text: view.shown.count + " items" + (view.searching && search.running ? " · searching…" : "") + (folder.error !== "" ? " · " + folder.error : ""); role: "label"; color: Theme.subtext }
                Item { Layout.fillWidth: true }
                MText { visible: view.shown.selectedCount > 0; text: view.shown.selectedCount + " selected"; role: "label"; color: Theme.primary }
            }

            ListView {
                id: list
                Layout.fillWidth: true; Layout.fillHeight: true
                model: view.shown; clip: true; spacing: 2
                reuseItems: true
                onContentYChanged: thumbTimer.restart()
                onHeightChanged: thumbTimer.restart()
                onCountChanged: thumbTimer.restart()
                delegate: ListRow {
                    required property int index
                    required property string name
                    required property string displayName
                    required property string uri
                    required property string kind
                    required property bool isDir
                    required property bool selected
                    required property bool starred
                    required property string sizeText
                    required property string modifiedText
                    required property string snippet
                    required property url thumbnail
                    width: ListView.view.width
                    title: displayName
                    subtitle: snippet !== "" ? snippet : modifiedText
                    icon: view.glyphFor(kind)
                    iconBadge: true
                    iconTint: isDir ? Theme.primary : Theme.subtext
                    image: thumbnail.toString()
                    trailing: sizeText
                    grouped: false
                    onClicked: view.shown.select(index, FolderModel.Replace)
                    onActivated: if (isDir) { field.text = ""; folder.location = uri; }
                    Glyph { visible: starred; name: "star"; fill: 1; color: Theme.primary; font.pixelSize: 18 }
                    Accessible.name: displayName
                }
            }

            // Operations
            Rectangle {
                visible: FileOperations.operations.count > 0
                Layout.fillWidth: true; Layout.preferredHeight: Math.min(opsList.contentHeight, 168) + 16
                radius: 24; color: Theme.surface
                ListView {
                    id: opsList
                    anchors.fill: parent; anchors.margins: 8; clip: true; spacing: 6
                    model: FileOperations.operations
                    delegate: Rectangle {
                        required property string title
                        required property string detail
                        required property string state
                        required property double progress
                        required property bool active
                        required property int id
                        width: ListView.view.width; height: 52; radius: 18
                        color: state === "failed" ? Theme.errorContainer : Theme.inverseSurface
                        RowLayout {
                            anchors.fill: parent; anchors.leftMargin: 16; anchors.rightMargin: 12; spacing: 12
                            ColumnLayout {
                                Layout.fillWidth: true; spacing: 0
                                MText { Layout.fillWidth: true; text: title; role: "titleS"; color: state === "failed" ? Theme.errorInk : Theme.inverseInk }
                                MText { Layout.fillWidth: true; text: detail; role: "labelS"; color: state === "failed" ? Theme.errorInk : Theme.inverseInk; opacity: .8 }
                            }
                            WavyProgress { visible: active; Layout.preferredWidth: 120; value: progress < 0 ? 0.15 : progress }
                            Glyph { visible: active; name: "close"; color: Theme.inverseInk; font.pixelSize: 20; TapHandler { onTapped: FileOperations.cancel(id) } }
                            Glyph { visible: !active; name: state === "done" ? "check_circle" : "error"; color: state === "failed" ? Theme.errorInk : Theme.inverseInk; font.pixelSize: 22 }
                        }
                    }
                }
            }
        }
    }
}
