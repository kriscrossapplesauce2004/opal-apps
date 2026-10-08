import QtQuick
import Opal.Material
// Album or book cover: the image when there is one, else a generated tonal cover with the title.
Rectangle {
    id: c
    property string source: ""
    property string title: ""
    property color tone: Theme.primaryContainer
    property real r: Theme.shapeLg
    radius: r
    color: source ? Theme.surfaceHighest : tone
    RoundedImage { anchors.fill: parent; radius: c.r; visible: c.source !== ""; source: c.source; sourceSize: Qt.size(Math.max(64, c.width * 2), Math.max(64, c.height * 2)) }
    MText { visible: c.source === ""; anchors.left: parent.left; anchors.right: parent.right; anchors.bottom: parent.bottom; anchors.margins: c.width * .1; text: c.title; wrapMode: Text.WordWrap; maximumLineCount: 3; fontSizeMode: Text.HorizontalFit; minimumPixelSize: 10; font.pixelSize: Math.max(11, c.width * .13); font.weight: Font.Bold; font.variableAxes: ({"wdth": 115, "ROND": 80}); color: "#ffffff" }
}
