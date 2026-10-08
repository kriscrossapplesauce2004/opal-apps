import QtQuick
import QtQuick.Layouts
import Opal.Material
// M3 Expressive FAB. size is small (56), medium (80) or large (96); text makes it an extended FAB. Press squeezes the corners.
Rectangle {
    id: fab
    property string icon: "add"
    property string text: ""
    property string size: "medium"
    property string tone: "primary"
    signal clicked()
    readonly property int edge: size === "small" ? 56 : size === "large" ? 96 : 80
    readonly property color ink: tone === "tertiary" ? Theme.onTertiaryContainer : tone === "secondary" ? Theme.onSecondaryContainer : Theme.onContainer
    implicitHeight: Theme.couch ? Math.max(edge, 72) : edge
    implicitWidth: text !== "" ? row.implicitWidth + 2 * Math.round(edge * .3) : implicitHeight
    radius: tap.pressed ? Math.round(edge * .22) : Math.round(edge * (size === "small" ? .3 : .28))
    color: tone === "tertiary" ? Theme.tertiaryContainer : tone === "secondary" ? Theme.secondaryContainer : Theme.primaryContainer
    Behavior on radius { NumberAnimation { duration: Theme.motion(Theme.durMedium); easing.type: Easing.BezierSpline; easing.bezierCurve: Theme.springy } }
    Behavior on implicitWidth { NumberAnimation { duration: Theme.motion(Theme.durLong); easing.type: Easing.BezierSpline; easing.bezierCurve: Theme.emphasized } }
    Rectangle { anchors.fill: parent; radius: parent.radius; color: fab.ink; opacity: tap.pressed ? .12 : hover.hovered ? .08 : 0 }
    RowLayout { id: row; anchors.centerIn: parent; spacing: 12
        Glyph { name: fab.icon; color: fab.ink; font.pixelSize: Math.round(fab.edge * (fab.size === "small" ? .43 : .36)) }
        MText { visible: fab.text !== ""; text: fab.text; role: fab.size === "small" ? "titleS" : "title"; color: fab.ink } }
    HoverHandler { id: hover }
    TapHandler { id: tap; onTapped: fab.clicked() }
    activeFocusOnTab: true
    Keys.onReturnPressed: clicked()
    FocusRing {}
    Accessible.role: Accessible.Button; Accessible.name: text || icon; Accessible.onPressAction: clicked()
}
