import QtQuick
import Opal.Material
// Album, book or station art: the image when there is one, else a tonal cover with the title on it.
Rectangle {
    id: c
    property string source: ""
    property string title: ""
    property string icon: ""
    property real r: Theme.shapeLg
    property bool playing: false
    readonly property bool hasImage: source !== "" && img.status !== Image.Error
    radius: r
    // A colour from the title, so covers without art still tell albums apart.
    readonly property color tone: {
        let h = 0;
        for (let i = 0; i < title.length; i++) h = (h * 31 + title.charCodeAt(i)) % 360;
        return Qt.hsla(h / 360, Theme.light ? .45 : .38, Theme.light ? .72 : .34, 1);
    }
    color: hasImage ? Theme.surfaceHighest : tone
    RoundedImage { id: img; anchors.fill: parent; radius: c.r; visible: c.hasImage; source: c.source; sourceSize: Qt.size(Math.max(96, c.width * 2), Math.max(96, c.height * 2)) }
    Glyph { visible: !c.hasImage && c.icon !== ""; anchors.centerIn: parent; name: c.icon; font.pixelSize: Math.min(c.width, c.height) * .38; color: Qt.rgba(1, 1, 1, .85) }
    MText {
        visible: !c.hasImage && c.icon === "" && c.width > 56
        anchors { left: parent.left; right: parent.right; bottom: parent.bottom; margins: c.width * .1 }
        text: c.title; wrapMode: Text.WordWrap; maximumLineCount: 3; elide: Text.ElideRight
        font.pixelSize: Math.max(11, c.width * .12); font.weight: Font.Bold; font.variableAxes: ({"wdth": 115, "ROND": 80}); color: "#ffffff"
    }
    // The playing album wears an equaliser badge.
    Rectangle {
        visible: c.playing && c.width > 80
        anchors { right: parent.right; bottom: parent.bottom; margins: Math.max(6, c.width * .05) }
        width: Math.min(44, c.width * .26); height: width; radius: width * .36; color: Theme.primary
        Glyph { anchors.centerIn: parent; name: "graphic_eq"; font.pixelSize: parent.width * .55; color: Theme.onPrimary }
    }
}
