import QtQuick
import QtQuick.Layouts
import Opal.Material
// Transport drawn with an art-derived scheme s instead of the app theme.
ColumnLayout {
    id: t
    property var s
    property real size: 72
    property string pos: "1:27"
    property string len: "3:12"
    property real p: .45
    spacing: 8
    WavyProgress { id: wave; Layout.fillWidth: true; value: t.p; animate: false; phase: .8; amplitude: 3; thickness: 5; wavelength: 30; color: t.s.primary; trackColor: t.s.secondary_container }
    RowLayout { Layout.fillWidth: true
        MText { text: t.pos; role: "labelS"; color: t.s.on_surface_variant }
        Item { Layout.fillWidth: true }
        MText { text: t.len; role: "labelS"; color: t.s.on_surface_variant } }
    RowLayout { Layout.alignment: Qt.AlignHCenter; spacing: t.size * .2
        Glyph { name: "shuffle"; font.pixelSize: t.size * .34; color: t.s.primary }
        Rectangle { implicitWidth: t.size * .82; implicitHeight: t.size * .82; radius: width * .32; color: t.s.secondary_container
            Glyph { anchors.centerIn: parent; name: "skip_previous"; fill: 1; font.pixelSize: t.size * .42; color: t.s.on_secondary_container } }
        Rectangle { implicitWidth: t.size * 1.35; implicitHeight: t.size; radius: t.size * .3; color: t.s.primary
            Glyph { anchors.centerIn: parent; name: "pause"; fill: 1; font.pixelSize: t.size * .48; color: t.s.on_primary } }
        Rectangle { implicitWidth: t.size * .82; implicitHeight: t.size * .82; radius: width * .32; color: t.s.secondary_container
            Glyph { anchors.centerIn: parent; name: "skip_next"; fill: 1; font.pixelSize: t.size * .42; color: t.s.on_secondary_container } }
        Glyph { name: "repeat"; font.pixelSize: t.size * .34; color: t.s.on_surface_variant } }
}
