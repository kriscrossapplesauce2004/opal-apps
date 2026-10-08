import QtQuick
import QtQuick.Shapes
import Opal.Material
// M3 Expressive wavy linear progress: the active part is a sine wave, the rest is a flat track with a stop dot.
Item {
    id: bar
    property real value: .5
    property real amplitude: 3
    property real wavelength: 28
    property real thickness: 4
    property bool animate: true
    property real phase: 0
    property color color: Theme.primary
    property color trackColor: Theme.secondaryContainer
    implicitHeight: amplitude * 2 + thickness + 4
    implicitWidth: 240
    NumberAnimation on phase { running: bar.animate && !Theme.reduceMotion && bar.visible; from: 0; to: 2 * Math.PI; duration: 1600; loops: Animation.Infinite }
    readonly property real split: Math.max(0, Math.min(width, width * value))
    Shape {
        anchors.fill: parent
        preferredRendererType: Shape.CurveRenderer
        ShapePath {
            strokeColor: bar.color; strokeWidth: bar.thickness; fillColor: "transparent"; capStyle: ShapePath.RoundCap
            PathPolyline {
                path: {
                    const pts = [], mid = bar.height / 2, end = Math.max(0, bar.split - bar.thickness);
                    for (let x = bar.thickness / 2; x <= end; x += 2)
                        pts.push(Qt.point(x, mid + Math.sin(x / bar.wavelength * 2 * Math.PI - bar.phase) * bar.amplitude));
                    return pts.length > 1 ? pts : [Qt.point(0, mid), Qt.point(0.1, mid)];
                }
            }
        }
    }
    Rectangle { x: bar.split + bar.thickness * 1.5; width: Math.max(0, bar.width - x); height: bar.thickness; radius: height / 2; anchors.verticalCenter: parent.verticalCenter; color: bar.trackColor }
    Rectangle { visible: bar.value < .97; width: bar.thickness; height: width; radius: width / 2; x: bar.width - width; anchors.verticalCenter: parent.verticalCenter; color: bar.color }
    Accessible.role: Accessible.ProgressBar
}
