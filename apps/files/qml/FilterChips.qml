import QtQuick
import QtQuick.Layouts
import Opal.Material
import Opal.Files
import "Kinds.js" as K
// Type filter chips and the sort chip above the content.
RowLayout {
    id: chips
    property var folder
    spacing: 8
    readonly property var active: folder ? folder.filterKind : []
    Chip { text: qsTr("All"); selected: chips.active.length === 0; glass: false; onClicked: chips.folder.filterKind = [] }
    Repeater {
        model: K.filters
        Chip {
            required property var modelData
            text: modelData.name; icon: modelData.icon; glass: false
            selected: chips.active.indexOf(modelData.id) >= 0
            onClicked: { let k = chips.active.slice(); const ids = [modelData.id].concat(modelData.also || []); if (selected) k = k.filter(x => ids.indexOf(x) < 0); else k = k.concat(ids); chips.folder.filterKind = k; }
        }
    }
    Item { Layout.fillWidth: true }
    Chip {
        readonly property var names: [qsTr("Name"), qsTr("Size"), qsTr("Modified"), qsTr("Kind")]
        text: names[chips.folder ? chips.folder.sortRole : 0]; icon: "sort"; trailingIcon: chips.folder && chips.folder.sortOrder === Qt.AscendingOrder ? "arrow_upward" : "arrow_downward"; glass: false
        onClicked: { const f = chips.folder; f.sortRole = (f.sortRole + 1) % 4; }
    }
}
