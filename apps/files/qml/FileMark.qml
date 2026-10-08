import QtQuick
import Opal.Material
import "Kinds.js" as K
// File-type mark: the thumbnail when one is ready, else a tonal squircle with the kind's glyph.
Rectangle {
    id: mark
    property string kind: "other"
    property url thumbnail
    property bool symlink: false
    property real size: 40
    readonly property bool hasThumb: thumbnail.toString() !== ""
    readonly property color tone: K.tone(Theme, kind)
    implicitWidth: size; implicitHeight: size
    radius: Math.round(size * .28)
    color: hasThumb ? Theme.surfaceHighest : Theme.alpha(tone, kind === "folder" ? .22 : .16)
    RoundedImage { anchors.fill: parent; radius: mark.radius; visible: mark.hasThumb; source: mark.thumbnail; sourceSize: Qt.size(mark.size * 2, mark.size * 2) }
    Glyph { anchors.centerIn: parent; visible: !mark.hasThumb; name: K.glyph[mark.kind] || "draft"; fill: mark.kind === "folder" ? 1 : 0; color: mark.tone; font.pixelSize: Math.round(mark.size * .52) }
    Glyph { visible: mark.symlink; anchors.right: parent.right; anchors.bottom: parent.bottom; anchors.margins: -2; name: "link"; font.pixelSize: Math.max(12, Math.round(mark.size * .32)); color: Theme.text }
}
