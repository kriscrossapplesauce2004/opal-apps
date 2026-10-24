import QtQuick
import Opal.Material
// Month scrubber on the right of the timeline. Drag or click to jump; the current month shows as a pill.
Item {
    id: sc
    property var marks: []
    property real contentY: 0
    property real contentHeight: 1
    property real viewHeight: 1
    signal jump(int row, real y)
    readonly property int current: {
        let c = 0;
        for (let i = 0; i < marks.length; i++) if (marks[i].y <= contentY + 8) c = i;
        return c;
    }
    readonly property bool active: drag.active || hover.hovered
    width: 56
    visible: marks.length > 1 && contentHeight > viewHeight * 1.5
    function yFor(m) { return Math.max(0, Math.min(height - 24, m.y / Math.max(1, contentHeight - viewHeight) * (height - 24))); }
    function at(py) {
        const t = Math.max(0, Math.min(1, py / height)) * Math.max(1, contentHeight - viewHeight);
        let best = 0;
        for (let i = 0; i < marks.length; i++) if (marks[i].y <= t) best = i;
        return best;
    }
    // Labels where there is room; years always win over months.
    Repeater {
        model: sc.marks
        Item {
            required property var modelData
            required property int index
            readonly property bool cur: index === sc.current
            readonly property bool roomy: {
                if (modelData.year) return true;
                const prev = index > 0 ? sc.yFor(sc.marks[index - 1]) : -100;
                return sc.yFor(modelData) - prev > 30;
            }
            y: sc.yFor(modelData); width: sc.width; height: 24
            Rectangle {
                anchors.right: parent.right; anchors.rightMargin: 14; anchors.verticalCenter: parent.verticalCenter
                width: cur ? label.implicitWidth + 18 : 6; height: cur ? 24 : 6; radius: height / 2
                color: cur ? Theme.primary : Theme.alpha(Theme.subtext, sc.active ? .6 : .35)
                Behavior on width { NumberAnimation { duration: Theme.motion(Theme.durShort) } }
                MText { id: label; anchors.centerIn: parent; visible: parent.parent.cur; text: modelData.label; role: "labelS"; font.weight: Font.Bold; color: Theme.onPrimary }
            }
            MText {
                visible: !cur && roomy && (sc.active || modelData.year)
                anchors.right: parent.right; anchors.rightMargin: 28; anchors.verticalCenter: parent.verticalCenter
                text: modelData.label; role: "labelS"; color: modelData.year ? Theme.text : Theme.subtext
                font.weight: modelData.year ? Font.Bold : Font.Normal
            }
        }
    }
    HoverHandler { id: hover }
    TapHandler { onTapped: point => { const i = sc.at(point.position.y); sc.jump(sc.marks[i].row, sc.marks[i].y); } }
    DragHandler {
        id: drag; target: null; yAxis.enabled: true; xAxis.enabled: false
        onCentroidChanged: if (active) { const i = sc.at(centroid.position.y); sc.jump(sc.marks[i].row, sc.marks[i].y); }
    }
    Accessible.role: Accessible.ScrollBar; Accessible.name: qsTr("Jump to a month")
}
