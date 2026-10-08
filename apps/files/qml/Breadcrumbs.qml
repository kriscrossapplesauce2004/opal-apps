import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Opal.Material
// Path as connected chips; the current folder is filled. Click the empty end or press Ctrl+L to type a path.
Rectangle {
    id: crumbs
    property var path: []
    property string location: ""
    property bool editing: false
    signal navigate(string uri)
    implicitHeight: Theme.couch ? 56 : Theme.touch ? 44 : 40
    implicitWidth: editing ? 420 : row.implicitWidth + 8
    radius: height / 2
    color: Theme.surfaceHigh
    clip: true
    function startEdit() { editing = true; field.text = location.startsWith("file://") ? decodeURIComponent(location.slice(7)) : location; field.forceActiveFocus(); field.selectAll(); }
    Flickable {
        id: flick
        visible: !crumbs.editing
        anchors.fill: parent
        contentWidth: row.implicitWidth + 8
        interactive: contentWidth > width
        onContentWidthChanged: contentX = Math.max(0, contentWidth - width)
        RowLayout {
            id: row
            anchors.verticalCenter: parent.verticalCenter; x: 4; spacing: 2
            Repeater {
                model: crumbs.path
                RowLayout {
                    id: seg
                    required property var modelData
                    required property int index
                    readonly property bool last: index === crumbs.path.length - 1
                    spacing: 2
                    Rectangle {
                        implicitHeight: crumbs.height - 8; implicitWidth: segRow.implicitWidth + 22
                        radius: height / 2
                        color: seg.last ? Theme.secondaryContainer : hover.hovered ? Theme.alpha(Theme.text, .06) : "transparent"
                        RowLayout { id: segRow; anchors.centerIn: parent; spacing: 6
                            Glyph { visible: !!seg.modelData.glyph; name: seg.modelData.glyph || ""; font.pixelSize: Theme.px(18); fill: 1; color: seg.last ? Theme.onSecondaryContainer : Theme.subtext }
                            MText { text: seg.modelData.name; role: "label"; font.pixelSize: Theme.px(13); font.weight: seg.last ? Font.DemiBold : Font.Medium; color: seg.last ? Theme.onSecondaryContainer : Theme.subtext } }
                        HoverHandler { id: hover }
                        TapHandler { onTapped: crumbs.navigate(seg.modelData.uri) }
                        Accessible.role: Accessible.Button; Accessible.name: seg.modelData.name
                    }
                    Glyph { visible: !seg.last; name: "chevron_right"; font.pixelSize: Theme.px(16); color: Theme.muted }
                }
            }
            // Empty end: a click here turns the bar into a text field.
            Item { Layout.preferredWidth: 24; Layout.fillHeight: true; implicitHeight: crumbs.height
                TapHandler { onTapped: crumbs.startEdit() } }
        }
    }
    TextField {
        id: field
        visible: crumbs.editing
        anchors.fill: parent; anchors.leftMargin: 16; anchors.rightMargin: 12
        background: null; color: Theme.text
        font.family: Theme.font; font.pixelSize: Theme.px(14)
        selectionColor: Theme.alpha(Theme.primary, .4)
        onAccepted: { const t = text.trim(); crumbs.editing = false; if (t) crumbs.navigate(t.startsWith("/") ? "file://" + t : t.startsWith("~") ? "file://" + Qt.resolvedUrl("file:///").toString() + t : t); }
        onActiveFocusChanged: if (!activeFocus) crumbs.editing = false
        Keys.onEscapePressed: crumbs.editing = false
    }
    Accessible.role: Accessible.ToolBar; Accessible.name: qsTr("Location")
}
