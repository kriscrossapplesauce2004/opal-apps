import QtQuick
import Opal.Material
// Justified photo rows: photos keep their aspect ratio and every full row fills the width exactly.
Column {
    id: j
    property var photos: []
    property real rowHeight: 180
    property real gap: 4
    property real tileRadius: 12
    property var selected: []
    property int focusIndex: -1
    spacing: gap
    readonly property var rows: {
        const out = []; let row = [], sum = 0;
        for (let i = 0; i < photos.length; i++) {
            const p = photos[i], a = p.w / p.h;
            row.push({p: p, a: a, i: i}); sum += a;
            if (sum * rowHeight + (row.length - 1) * gap >= width) { out.push({items: row, h: (width - (row.length - 1) * gap) / sum}); row = []; sum = 0; }
        }
        if (row.length) out.push({items: row, h: rowHeight});
        return out;
    }
    Repeater {
        model: j.rows
        Row {
            id: rowItem
            required property var modelData
            spacing: j.gap
            Repeater {
                model: modelData.items
                Item {
                    id: tile
                    required property var modelData
                    readonly property bool sel: j.selected.indexOf(modelData.i) >= 0
                    readonly property bool foc: modelData.i === j.focusIndex
                    width: rowItem.modelData.h * modelData.a; height: rowItem.modelData.h
                    Rectangle { anchors.fill: parent; anchors.margins: -5; radius: j.tileRadius + 5; color: "transparent"; border.width: 3; border.color: Theme.primary; visible: tile.foc }
                    RoundedImage { anchors.centerIn: parent; width: parent.width * (tile.sel ? .9 : 1); height: parent.height * (tile.sel ? .88 : 1); radius: tile.sel ? j.tileRadius * 2 : j.tileRadius; source: tile.modelData.p.u; sourceSize: Qt.size(Math.round(width * 1.5), Math.round(height * 1.5)) }
                    Rectangle { visible: j.selected.length > 0; x: 6; y: 6; width: 24; height: 24; radius: 12; color: tile.sel ? Theme.primary : Qt.rgba(0, 0, 0, .3); border.width: 2; border.color: tile.sel ? Theme.base : "#ffffff"
                        Glyph { anchors.centerIn: parent; visible: tile.sel; name: "check"; font.pixelSize: 16; color: Theme.onPrimary } }
                }
            }
        }
    }
}
