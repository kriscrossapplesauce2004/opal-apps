import QtQuick
import Opal.Material
// One node of a tab's split tree: a pane leaf, or two children side by side or stacked with a draggable divider.
Item {
    id: node
    property var tree
    property var view
    readonly property bool leaf: !!tree && tree.pane !== undefined
    // A node loads children only once it has a real split; an empty node must not recurse.
    readonly property bool branch: !!tree && tree.split !== undefined
    readonly property bool horizontal: tree && tree.split === "h"
    property real ratio: tree && tree.ratio ? tree.ratio : .5
    readonly property real gap: 8
    Loader {
        anchors.fill: parent
        active: node.leaf
        sourceComponent: TermPane {
            session: node.view.sessionFor(node.tree.pane)
            focused: node.view.focusedPane === node.tree.pane
            background: node.view.paneBackground
            onClicked: node.view.focusPane(node.tree.pane)
            Component.onCompleted: if (focused) term.forceActiveFocus()
            onFocusedChanged: if (focused) term.forceActiveFocus()
        }
    }
    Loader {
        id: first
        active: node.branch
        x: 0; y: 0
        width: node.horizontal ? (node.width - node.gap) * node.ratio : node.width
        height: node.horizontal ? node.height : (node.height - node.gap) * node.ratio
        source: Qt.resolvedUrl("SplitNode.qml")
        onLoaded: { item.view = node.view; item.tree = Qt.binding(() => node.tree.a); }
    }
    Loader {
        id: second
        active: node.branch
        x: node.horizontal ? first.width + node.gap : 0
        y: node.horizontal ? 0 : first.height + node.gap
        width: node.horizontal ? node.width - x : node.width
        height: node.horizontal ? node.height : node.height - y
        source: Qt.resolvedUrl("SplitNode.qml")
        onLoaded: { item.view = node.view; item.tree = Qt.binding(() => node.tree.b); }
    }
    // Divider: drag to resize; it is the gap between the two children.
    Item {
        visible: node.branch
        x: node.horizontal ? first.width : 0; y: node.horizontal ? 0 : first.height
        width: node.horizontal ? node.gap : node.width; height: node.horizontal ? node.height : node.gap
        Rectangle { anchors.centerIn: parent; width: node.horizontal ? 4 : 48; height: node.horizontal ? 48 : 4; radius: 2; color: Theme.alpha(Theme.subtext, drag.active || hover.hovered ? .6 : 0)
            Behavior on color { ColorAnimation { duration: Theme.motion(Theme.durShort) } } }
        HoverHandler { id: hover; cursorShape: node.horizontal ? Qt.SplitHCursor : Qt.SplitVCursor }
        DragHandler {
            id: drag; target: null
            onCentroidChanged: if (active) {
                const p = parent.mapToItem(node, centroid.position.x, centroid.position.y);
                node.ratio = Math.max(.15, Math.min(.85, node.horizontal ? p.x / node.width : p.y / node.height));
                node.tree.ratio = node.ratio;
            }
        }
    }
}
