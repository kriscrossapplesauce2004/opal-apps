import QtQuick
import Opal.Material
import "Fixtures.js" as F
// File-type mark: a tonal squircle with a Material Symbol, or the thumbnail when there is one.
Rectangle {
    id: g
    property var file: ({})
    property real size: 40
    property bool thumb: true
    readonly property bool hasThumb: thumb && !!file.t
    readonly property color tone: file.c === "tertiary" ? Theme.tertiary : file.c === "secondary" ? Theme.secondary : file.k === "folder" ? Theme.primary : file.k === "pdf" ? Theme.error : file.k === "archive" ? Theme.tertiary : Theme.secondary
    implicitWidth: size; implicitHeight: size
    radius: Math.round(size * .28)
    color: hasThumb ? Theme.surfaceHighest : Theme.alpha(tone, file.k === "folder" ? .22 : .16)
    RoundedImage { anchors.fill: parent; radius: g.radius; visible: g.hasThumb; source: g.hasThumb ? g.file.t : ""; sourceSize: Qt.size(g.size * 2, g.size * 2) }
    Glyph { anchors.centerIn: parent; visible: !g.hasThumb; name: F.glyph[g.file.k] || "draft"; fill: g.file.k === "folder" ? 1 : 0; color: g.tone; font.pixelSize: Math.round(g.size * .52) }
    Glyph { visible: !!g.file.sync; anchors.right: parent.right; anchors.bottom: parent.bottom; anchors.margins: g.size * .08; name: "sync"; font.pixelSize: Math.round(g.size * .3); color: g.tone }
}
