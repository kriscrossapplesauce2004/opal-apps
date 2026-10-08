import QtQuick
import QtQml.Models
import QtQuick.Layouts
import Opal.Material
import Opal.Files
// The newest active operation as an inverse pill with wavy progress. Clicking it opens the operations sheet.
Rectangle {
    id: pill
    property var operations: FileOperations.operations
    property var ops: []
    readonly property var op: { for (let i = ops.length - 1; i >= 0; i--) if (ops[i] && ops[i].active) return ops[i]; return null; }
    signal openSheet()
    Instantiator {
        model: pill.operations
        delegate: QtObject {
            required property int id
            required property string kind
            required property string title
            required property string detail
            required property string state
            required property real progress
            required property bool active
        }
        onObjectAdded: (i, o) => { const a = pill.ops.slice(); a.splice(i, 0, o); pill.ops = a; }
        onObjectRemoved: (i, o) => { const a = pill.ops.slice(); a.splice(i, 1); pill.ops = a; }
    }
    implicitHeight: Theme.couch ? 72 : 56
    implicitWidth: 380
    radius: height / 2
    color: Theme.inverseSurface
    visible: op !== null
    readonly property string glyph: !op ? "content_copy" : op.kind === "move" ? "drive_file_move" : op.kind === "trash" || op.kind === "delete" || op.kind === "emptyTrash" ? "delete" : op.kind === "extract" || op.kind === "compress" ? "folder_zip" : op.kind === "restore" ? "restore_from_trash" : "content_copy"
    RowLayout {
        anchors.fill: parent; anchors.leftMargin: 8; anchors.rightMargin: 10; spacing: 12
        Rectangle { Layout.preferredWidth: pill.height - 16; Layout.preferredHeight: pill.height - 16; radius: width / 2; color: Theme.inversePrimary
            Glyph { anchors.centerIn: parent; name: pill.glyph; font.pixelSize: Theme.px(20); color: Theme.inverseSurface } }
        ColumnLayout { Layout.fillWidth: true; spacing: 3
            MText { Layout.fillWidth: true; text: pill.op ? pill.op.title : ""; role: "label"; font.weight: Font.DemiBold; color: Theme.inverseInk }
            RowLayout { Layout.fillWidth: true; spacing: 10
                WavyProgress { Layout.fillWidth: true; value: pill.op && pill.op.progress >= 0 ? pill.op.progress : 0; amplitude: 2.2; wavelength: 18; thickness: 3; color: Theme.inversePrimary; trackColor: Theme.alpha(Theme.inverseInk, .25) }
                MText { text: pill.op ? pill.op.detail : ""; role: "labelS"; color: Theme.alpha(Theme.inverseInk, .75) } } }
        MButton { icon: "close"; compact: true; ink: Theme.inverseInk; implicitWidth: 36; implicitHeight: 36; tooltip: qsTr("Cancel"); onClicked: if (pill.op) FileOperations.cancel(pill.op.id) }
    }
    TapHandler { onTapped: pill.openSheet() }
    Accessible.role: Accessible.ProgressBar; Accessible.name: op ? op.title : ""
}
