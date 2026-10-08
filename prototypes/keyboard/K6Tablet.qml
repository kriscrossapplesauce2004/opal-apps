import QtQuick
import QtQuick.Layouts
import Opal.Material
// Tablet landscape: split keyboard so thumbs reach every key.
Rectangle {
    color: Theme.base
    MText { x: 40; y: 40; text: "Split on tablets and landscape"; role: "displayS" }
    Kbd { anchors.left: parent.left; anchors.right: parent.right; anchors.bottom: parent.bottom; split: true; keyH: 58; bar: "suggest" }
}
