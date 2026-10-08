import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Opal.Material
import Opal.Files
// The app's sheets: properties, conflicts, all operations, server credentials, delete confirmation and connect to server.
Item {
    id: dialogs
    anchors.fill: parent
    property var places
    property string propertiesUri: ""
    property var conflict: null
    property var credentials: null
    property var pendingDelete: []
    function showProperties(uri) { propertiesUri = uri; props.open = true; }
    function confirmDelete(uris) { pendingDelete = uris; del.open = true; }
    function showOperations() { ops.open = true; }
    function connectServer() { srv.open = true; srvField.forceActiveFocus(); }
    Connections {
        target: FileOperations
        function onConflictRequested(id, name, sourceUri, destUri, isDir) { dialogs.conflict = {id: id, name: name, isDir: isDir}; conf.open = true; }
    }
    Connections {
        target: dialogs.places
        function onCredentialsRequested(id, message, defaultUser, defaultDomain, needUsername, needDomain, needPassword, canAnonymous, canRemember) {
            dialogs.credentials = {id: id, message: message, user: defaultUser, needUsername: needUsername, needPassword: needPassword, canAnonymous: canAnonymous, canRemember: canRemember};
            userField.text = defaultUser; passField.text = ""; cred.open = true;
        }
    }
    component Title: MText { role: "headline"; Layout.fillWidth: true; wrapMode: Text.Wrap }
    component Field: TextField {
        Layout.fillWidth: true; color: Theme.text; font.family: Theme.font; font.pixelSize: 15; placeholderTextColor: Theme.subtext; leftPadding: 16; implicitHeight: 48
        background: Rectangle { radius: 16; color: Theme.surfaceHigh; border.width: parent.activeFocus ? 2 : 0; border.color: Theme.primary }
    }
    // Properties.
    Sheet {
        id: props; sheetHeight: 520
        FileInfo { id: info; uri: dialogs.propertiesUri }
        ColumnLayout {
            anchors.fill: parent; anchors.margins: 24; spacing: 14
            RowLayout { spacing: 16
                FileMark { kind: info.isDir ? "folder" : "other"; size: 56 }
                ColumnLayout { spacing: 2; Layout.fillWidth: true; Title { text: info.name } MText { text: info.kindName; role: "body"; color: Theme.subtext } } }
            GridLayout { columns: 2; columnSpacing: 24; rowSpacing: 10; Layout.fillWidth: true
                Repeater { model: [[qsTr("Size"), info.sizeText], [qsTr("Dimensions"), info.dimensionsText], [qsTr("Where"), info.path], [qsTr("Modified"), info.modifiedText], [qsTr("Owner"), info.owner + (info.group ? " · " + info.group : "")], [qsTr("Link to"), info.symlinkTarget]]
                    RowLayout { required property var modelData; visible: modelData[1] !== "" && modelData[1] !== undefined; Layout.columnSpan: 2; spacing: 0
                        MText { Layout.preferredWidth: 110; text: modelData[0]; role: "bodyS"; color: Theme.subtext }
                        MText { Layout.fillWidth: true; text: modelData[1]; role: "bodyS"; elide: Text.ElideMiddle } } } }
            MText { text: qsTr("Permissions · %1").arg(info.permissionsOctal); role: "label"; color: Theme.primary }
            GridLayout { columns: 4; columnSpacing: 8; rowSpacing: 6; enabled: info.canSetPermissions
                Repeater { model: [qsTr("Owner"), qsTr("Group"), qsTr("Others")]
                    RowLayout { id: who; required property string modelData; required property int index; Layout.columnSpan: 4; spacing: 8
                        MText { text: who.modelData; role: "bodyS"; Layout.preferredWidth: 70 }
                        Repeater { model: [[qsTr("Read"), 4], [qsTr("Write"), 2], [qsTr("Run"), 1]]
                            Chip { required property var modelData; readonly property int bit: modelData[1] << (3 * (2 - who.index))
                                text: modelData[0]; glass: false; selected: (info.permissions & bit) !== 0
                                onClicked: info.setPermissions(info.permissions ^ bit) } } } } }
            Item { Layout.fillHeight: true }
            RowLayout { Layout.fillWidth: true; Item { Layout.fillWidth: true } MButton { text: qsTr("Close"); filled: true; onClicked: props.open = false } }
        }
    }
    // Conflict.
    Sheet {
        id: conf; sheetHeight: 300
        ColumnLayout {
            anchors.fill: parent; anchors.margins: 24; spacing: 14
            Title { text: dialogs.conflict ? qsTr("“%1” already exists").arg(dialogs.conflict.name) : "" }
            MText { Layout.fillWidth: true; text: qsTr("Choose what to do with this item."); role: "body"; color: Theme.subtext }
            Chip { id: allChip; text: qsTr("Do this for all conflicts"); glass: false; selected: false; onClicked: selected = !selected }
            Item { Layout.fillHeight: true }
            RowLayout { Layout.fillWidth: true; spacing: 8
                MButton { text: qsTr("Cancel all"); contained: true; onClicked: { FileOperations.resolveConflict(dialogs.conflict.id, FileOperations.CancelAll, true); conf.open = false; } }
                Item { Layout.fillWidth: true }
                MButton { text: qsTr("Skip"); tonal: true; onClicked: { FileOperations.resolveConflict(dialogs.conflict.id, FileOperations.Skip, allChip.selected); conf.open = false; } }
                MButton { text: qsTr("Keep both"); tonal: true; onClicked: { FileOperations.resolveConflict(dialogs.conflict.id, FileOperations.KeepBoth, allChip.selected); conf.open = false; } }
                MButton { text: qsTr("Replace"); filled: true; onClicked: { FileOperations.resolveConflict(dialogs.conflict.id, FileOperations.Replace, allChip.selected); conf.open = false; } } }
        }
    }
    // All operations.
    Sheet {
        id: ops; sheetHeight: 460
        ColumnLayout {
            anchors.fill: parent; anchors.margins: 20; spacing: 8
            RowLayout { Layout.fillWidth: true; Title { text: qsTr("File operations") } MButton { text: qsTr("Clear finished"); contained: true; onClicked: FileOperations.operations.clearFinished() } }
            ListView {
                Layout.fillWidth: true; Layout.fillHeight: true; clip: true; spacing: 6
                model: FileOperations.operations
                delegate: Rectangle {
                    id: o
                    required property int id
                    required property string title
                    required property string detail
                    required property string state
                    required property real progress
                    required property bool active
                    required property bool canPause
                    required property bool paused
                    required property string error
                    width: ListView.view.width; height: 76; radius: 22; color: Theme.surfaceHigh
                    RowLayout { anchors.fill: parent; anchors.margins: 14; spacing: 12
                        Glyph { name: o.state === "failed" ? "error" : o.state === "done" ? "check_circle" : o.state === "cancelled" ? "cancel" : "sync"; fill: 1; color: o.state === "failed" ? Theme.error : Theme.primary }
                        ColumnLayout { Layout.fillWidth: true; spacing: 4
                            MText { Layout.fillWidth: true; text: o.title; role: "titleS" }
                            WavyProgress { visible: o.active; Layout.fillWidth: true; value: o.progress >= 0 ? o.progress : 0; animate: !o.paused; amplitude: 2; wavelength: 16; thickness: 3 }
                            MText { Layout.fillWidth: true; text: o.error !== "" ? o.error : o.detail; role: "labelS"; color: o.error !== "" ? Theme.error : Theme.subtext } }
                        MButton { visible: o.canPause; icon: o.paused ? "play_arrow" : "pause"; compact: true; onClicked: o.paused ? FileOperations.resume(o.id) : FileOperations.pause(o.id) }
                        MButton { icon: o.active ? "close" : "done"; compact: true; onClicked: o.active ? FileOperations.cancel(o.id) : FileOperations.operations.dismiss(o.id) } }
                }
            }
        }
    }
    // Server credentials.
    Sheet {
        id: cred; sheetHeight: 380
        ColumnLayout {
            anchors.fill: parent; anchors.margins: 24; spacing: 12
            Title { text: qsTr("Sign in") }
            MText { Layout.fillWidth: true; text: dialogs.credentials ? dialogs.credentials.message : ""; role: "body"; color: Theme.subtext; wrapMode: Text.Wrap }
            Field { id: userField; visible: !!dialogs.credentials && dialogs.credentials.needUsername; placeholderText: qsTr("User name") }
            Field { id: passField; visible: !!dialogs.credentials && dialogs.credentials.needPassword; placeholderText: qsTr("Password"); echoMode: TextInput.Password; onAccepted: signIn.clicked() }
            Chip { id: remember; visible: !!dialogs.credentials && dialogs.credentials.canRemember; text: qsTr("Remember password"); glass: false; onClicked: selected = !selected }
            Item { Layout.fillHeight: true }
            RowLayout { Layout.fillWidth: true; spacing: 8
                MButton { text: qsTr("Cancel"); contained: true; onClicked: { dialogs.places.cancelPrompt(dialogs.credentials.id); cred.open = false; } }
                MButton { visible: !!dialogs.credentials && dialogs.credentials.canAnonymous; text: qsTr("Guest"); tonal: true; onClicked: { dialogs.places.replyCredentials(dialogs.credentials.id, "", "", "", false, true); cred.open = false; } }
                Item { Layout.fillWidth: true }
                MButton { id: signIn; text: qsTr("Sign in"); filled: true; onClicked: { dialogs.places.replyCredentials(dialogs.credentials.id, userField.text, passField.text, "", remember.selected, false); cred.open = false; } } }
        }
    }
    // Delete for good.
    Sheet {
        id: del; sheetHeight: 240
        ColumnLayout {
            anchors.fill: parent; anchors.margins: 24; spacing: 12
            Title { text: qsTr("Delete %n item(s) for good?", "", dialogs.pendingDelete.length) }
            MText { Layout.fillWidth: true; text: qsTr("They will not go to the Trash. This cannot be undone."); role: "body"; color: Theme.subtext }
            Item { Layout.fillHeight: true }
            RowLayout { Layout.fillWidth: true; Item { Layout.fillWidth: true }
                MButton { text: qsTr("Cancel"); contained: true; onClicked: del.open = false }
                MButton { text: qsTr("Delete"); filled: true; accent: Theme.error; fillInk: Theme.role("on_error", "#601410"); onClicked: { FileOperations.remove(dialogs.pendingDelete); del.open = false; } } }
        }
    }
    // Connect to server.
    Sheet {
        id: srv; sheetHeight: 280
        ColumnLayout {
            anchors.fill: parent; anchors.margins: 24; spacing: 12
            Title { text: qsTr("Connect to server") }
            Field { id: srvField; placeholderText: "smb://nas.local/share  ·  sftp://user@host  ·  ftp://host"; onAccepted: go.clicked() }
            MText { text: qsTr("Supports SMB, SFTP, FTP, WebDAV and NFS through GIO."); role: "labelS"; color: Theme.subtext }
            Item { Layout.fillHeight: true }
            RowLayout { Layout.fillWidth: true; Item { Layout.fillWidth: true }
                MButton { text: qsTr("Cancel"); contained: true; onClicked: srv.open = false }
                MButton { id: go; text: qsTr("Connect"); filled: true; onClicked: { if (srvField.text.trim()) dialogs.places.connectToServer(srvField.text.trim()); srv.open = false; } } }
        }
    }
}
