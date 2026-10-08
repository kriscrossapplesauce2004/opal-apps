import QtQuick
import Opal.Material
// M3 card: filled (tonal), elevated or outlined. Interactive cards get a state layer and a focus ring.
Rectangle {
    id: card
    property string variant: "filled"
    property bool interactive: false
    signal clicked()
    radius: Theme.shapeLg
    color: variant === "outlined" ? "transparent" : variant === "elevated" ? Theme.surfaceLow : Theme.surfaceHighest
    border.width: variant === "outlined" ? 1 : 0
    border.color: Theme.outline
    Rectangle { anchors.fill: parent; radius: parent.radius; color: Theme.text; visible: card.interactive; opacity: tap.pressed ? .12 : hover.hovered ? .06 : 0 }
    HoverHandler { id: hover; enabled: card.interactive }
    TapHandler { id: tap; enabled: card.interactive; onTapped: card.clicked() }
    activeFocusOnTab: interactive
    Keys.onReturnPressed: clicked()
    FocusRing { visible: card.interactive && card.activeFocus }
}
