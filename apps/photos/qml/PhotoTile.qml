import QtQuick
import Opal.Material
// One photo in a justified row. A selected photo shrinks into a rounder tile with a check, like Google Photos.
Item {
    id: tile
    property var item: ({})
    property real radius: 12
    property bool selecting: false
    property bool focused: false
    signal activated(var mouse)
    signal held()
    readonly property bool sel: !!item.selected
    Rectangle {
        anchors.fill: parent; anchors.margins: -5; radius: tile.radius + 5; color: "transparent"
        border.width: 3; border.color: Theme.primary; visible: tile.focused
    }
    Rectangle {
        anchors.fill: parent; radius: tile.radius
        color: tile.sel ? Theme.secondaryContainer : Theme.surfaceHigh
    }
    RoundedImage {
        id: img
        anchors.centerIn: parent
        width: parent.width - (tile.sel ? Math.min(28, parent.width * .14) : 0)
        height: parent.height - (tile.sel ? Math.min(28, parent.height * .14) : 0)
        Behavior on width { NumberAnimation { duration: Theme.motion(Theme.durShort); easing.type: Easing.OutCubic } }
        Behavior on height { NumberAnimation { duration: Theme.motion(Theme.durShort); easing.type: Easing.OutCubic } }
        radius: tile.sel ? tile.radius * 1.6 : tile.radius
        source: tile.item.thumb || ""
        sourceSize: Qt.size(Math.min(512, Math.round(tile.width * 1.5)), Math.min(512, Math.round(tile.height * 1.5)))
        opacity: status === Image.Ready ? 1 : 0
        Behavior on opacity { NumberAnimation { duration: Theme.motion(Theme.durShort) } }
    }
    Rectangle {
        anchors.fill: parent; radius: tile.radius; color: Theme.stateLayer
        opacity: tap.pressed ? .14 : hover.hovered ? .06 : 0
    }
    // Videos show their kind; favourites show a small heart.
    Rectangle {
        visible: !!tile.item.video
        anchors { right: parent.right; top: parent.top; margins: 8 }
        width: 28; height: 28; radius: 14; color: Qt.rgba(0, 0, 0, .5)
        Glyph { anchors.centerIn: parent; name: "play_arrow"; fill: 1; font.pixelSize: 18; color: "#ffffff" }
    }
    Glyph {
        visible: !!tile.item.favourite && !tile.sel
        anchors { left: parent.left; bottom: parent.bottom; margins: 8 }
        name: "favorite"; fill: 1; font.pixelSize: 18; color: "#ffffff"
        style: Text.Outline; styleColor: Qt.rgba(0, 0, 0, .25)
    }
    Rectangle {
        visible: tile.selecting || hover.hovered
        x: 7; y: 7; width: 24; height: 24; radius: 12
        color: tile.sel ? Theme.primary : Qt.rgba(0, 0, 0, .28)
        border.width: 2; border.color: tile.sel ? Theme.base : "#ffffff"
        Glyph { anchors.centerIn: parent; visible: tile.sel; name: "check"; font.pixelSize: 16; color: Theme.onPrimary }
        TapHandler { onTapped: tile.activated({modifiers: Qt.ControlModifier}) }
    }
    HoverHandler { id: hover; cursorShape: Qt.PointingHandCursor }
    TapHandler {
        id: tap
        acceptedModifiers: Qt.KeyboardModifierMask
        onTapped: (point, button) => tile.activated({modifiers: point.modifiers})
        onLongPressed: tile.held()
    }
    Accessible.role: Accessible.Button
    Accessible.name: (tile.item.video ? qsTr("Video ") : qsTr("Photo ")) + (tile.item.path || "").split("/").pop()
    Accessible.checked: sel
}
