import QtQuick
import QtQuick.Layouts
import Opal.Material
// Adaptive app scaffold. Couch: big centred tabs on top and a hint bar. Expanded: an expanded rail. Medium: a rail. Compact: a bottom bar.
Rectangle {
    id: frame
    property var destinations: []
    property int currentIndex: 0
    property string fabIcon: ""
    property string fabText: ""
    property var hints: [{b: "A", t: qsTr("Open")}, {b: "B", t: qsTr("Back")}, {b: "Y", t: qsTr("Search")}]
    property bool showNav: destinations.length > 1
    // Lets the app put extra items (storage meters, a now-playing tile) under the rail destinations.
    property alias railFooter: rail.footer
    default property alias content: body.data
    signal fabClicked()
    readonly property string widthClass: width < 600 ? "compact" : width < 840 ? "medium" : "expanded"
    readonly property string navStyle: !showNav ? "none" : Posture.couch ? "couch" : widthClass === "compact" ? "bar" : widthClass === "medium" ? "rail" : "railExpanded"
    readonly property bool hintsVisible: Posture.couch || Posture.controllerActive
    color: Theme.base
    function step(delta) { if (destinations.length) currentIndex = (currentIndex + delta + destinations.length) % destinations.length; }
    Keys.onPressed: e => {
        if ((e.modifiers & Qt.ControlModifier) && e.key === Qt.Key_PageDown) { step(1); e.accepted = true; }
        else if ((e.modifiers & Qt.ControlModifier) && e.key === Qt.Key_PageUp) { step(-1); e.accepted = true; }
    }
    RowLayout {
        anchors.fill: parent
        anchors.bottomMargin: frame.navStyle === "bar" ? navBar.height : 0
        spacing: 0
        NavRail {
            id: rail
            visible: frame.navStyle === "rail" || frame.navStyle === "railExpanded"
            expanded: frame.navStyle === "railExpanded"
            Layout.fillHeight: true; Layout.preferredWidth: implicitWidth
            destinations: frame.destinations; currentIndex: frame.currentIndex
            fabIcon: frame.fabIcon; fabText: frame.fabText
            onPicked: i => frame.currentIndex = i
            onFabClicked: frame.fabClicked()
        }
        ColumnLayout {
            Layout.fillWidth: true; Layout.fillHeight: true; spacing: 0
            CouchTabs {
                visible: frame.navStyle === "couch"
                Layout.fillWidth: true
                destinations: frame.destinations; currentIndex: frame.currentIndex
                onPicked: i => frame.currentIndex = i
            }
            Item { id: body; Layout.fillWidth: true; Layout.fillHeight: true; clip: true }
        }
    }
    NavBar {
        id: navBar
        visible: frame.navStyle === "bar"
        anchors.left: parent.left; anchors.right: parent.right; anchors.bottom: parent.bottom
        destinations: frame.destinations; currentIndex: frame.currentIndex
        onPicked: i => frame.currentIndex = i
    }
    HintBar {
        visible: frame.hintsVisible && frame.hints.length > 0
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom; anchors.bottomMargin: frame.navStyle === "bar" ? navBar.height + 12 : 20
        items: frame.hints.map(h => ({b: Gamepad.label(h.b.toLowerCase()).length === 1 && "ABXY".indexOf(h.b) >= 0 ? Gamepad.label(h.b.toLowerCase()) : h.b, t: h.t}))
        z: 40
    }
}
