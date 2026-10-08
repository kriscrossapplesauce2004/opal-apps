import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Opal.Material
import Opal.TerminalUi
// Command palette: snippets, SSH hosts and actions in one search. Up/Down pick, Enter runs, Escape closes.
Item {
    id: pal
    property var view
    property bool open: false
    anchors.fill: parent
    visible: open
    z: 40
    onOpenChanged: if (open) { field.text = ""; field.input.forceActiveFocus(); list.currentIndex = 0; }
    readonly property var items: {
        const q = field.text.toLowerCase();
        const all = [];
        for (const s of TerminalConfig.snippets) all.push({group: qsTr("Snippets"), icon: "bolt", title: s.name, detail: s.command, run: () => view.runInFocused(s.command)});
        for (const h of TerminalHelpers.sshHosts()) all.push({group: qsTr("Hosts"), icon: "dns", title: h.name, detail: "ssh " + (h.user ? h.user + "@" : "") + h.host, run: () => view.newTab("ssh " + h.name)});
        const acts = [["Split right", "vertical_split", "Ctrl+Shift+D", () => view.split("h")], ["Split down", "horizontal_split", "Ctrl+Shift+E", () => view.split("v")],
                      ["New tab", "tab", "Ctrl+Shift+T", () => view.newTab()], ["Close pane", "close", "Ctrl+Shift+W", () => view.closePane()],
                      ["Sessions", "view_sidebar", "Ctrl+Shift+S", () => view.sidebar = !view.sidebar], ["Appearance", "palette", "Ctrl+,", () => view.appearance = true],
                      ["Edit terminal.conf", "edit_note", "", () => TerminalHelpers.openFile(TerminalConfig.path)], ["Zoom in", "zoom_in", "Ctrl+=", () => view.zoom(1)], ["Zoom out", "zoom_out", "Ctrl+-", () => view.zoom(-1)]];
        for (const a of acts) all.push({group: qsTr("Actions"), icon: a[1], title: qsTr(a[0]), detail: "", key: a[2], run: a[3]});
        // A query that matches nothing still runs as a command in the focused pane.
        const hits = q ? all.filter(i => (i.title + " " + i.detail).toLowerCase().indexOf(q) >= 0) : all;
        if (q && !hits.length) hits.push({group: qsTr("Run"), icon: "terminal", title: field.text, detail: qsTr("Run in this pane"), run: () => view.runInFocused(field.text)});
        return hits;
    }
    function run(i) { const it = items[i]; open = false; if (it) it.run(); view.refocus(); }
    Rectangle { anchors.fill: parent; color: "#000000"; opacity: .35; TapHandler { onTapped: { pal.open = false; pal.view.refocus(); } } }
    Rectangle {
        anchors.horizontalCenter: parent.horizontalCenter; y: Math.min(110, parent.height * .12)
        width: Math.min(640, parent.width - 32); height: Math.min(480, parent.height - y - 24); radius: 28; color: Theme.surfaceHigh
        ColumnLayout {
            anchors.fill: parent; anchors.margins: 12; spacing: 6
            SearchField { id: field; Layout.fillWidth: true; leadingIcon: "terminal"; placeholder: qsTr("Run a snippet, open a host, or type a command"); color: Theme.surfaceHighest
                input.Keys.onDownPressed: list.currentIndex = Math.min(list.count - 1, list.currentIndex + 1)
                input.Keys.onUpPressed: list.currentIndex = Math.max(0, list.currentIndex - 1)
                input.Keys.onEscapePressed: { pal.open = false; pal.view.refocus(); }
                onAccepted: pal.run(list.currentIndex)
                onTextChanged: list.currentIndex = 0 }
            ListView {
                id: list
                Layout.fillWidth: true; Layout.fillHeight: true; clip: true
                model: pal.items
                section.property: "group"
                section.delegate: MText { required property string section; text: section; role: "label"; color: Theme.primary; leftPadding: 14; topPadding: 8; bottomPadding: 4 }
                delegate: ListRow {
                    required property var modelData
                    required property int index
                    width: ListView.view.width
                    icon: modelData.icon; iconBadge: true; iconTint: Theme.primary
                    title: modelData.title; subtitle: modelData.detail
                    trailing: modelData.key || (index === list.currentIndex ? "↵" : "")
                    selected: index === list.currentIndex
                    onClicked: pal.run(index)
                }
            }
        }
    }
}
