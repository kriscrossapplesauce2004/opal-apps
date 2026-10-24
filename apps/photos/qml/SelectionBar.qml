import QtQuick
import QtQuick.Layouts
import Opal.Material
import Opal.Photos
// Actions for the selected photos, floating over the bottom of the timeline.
Rectangle {
    id: bar
    property var view: null
    readonly property int count: view ? view.selectedCount : 0
    signal message(string text)
    visible: opacity > 0
    opacity: count > 0 ? 1 : 0
    Behavior on opacity { NumberAnimation { duration: Theme.motion(Theme.durShort) } }
    anchors.horizontalCenter: parent.horizontalCenter
    y: parent.height - (count > 0 ? height + 20 : -10)
    Behavior on y { NumberAnimation { duration: Theme.motion(Theme.durMedium); easing.type: Easing.BezierSpline; easing.bezierCurve: Theme.emphasizedDecelerate } }
    implicitHeight: 64; width: Math.min(parent.width - 24, row.implicitWidth + 24); radius: 32
    color: Theme.surfaceHighest
    z: 20
    function paths() { return view ? view.selectedPaths() : []; }
    RowLayout {
        id: row
        anchors.centerIn: parent; spacing: 4
        MButton { icon: "close"; compact: true; tooltip: qsTr("Clear selection"); onClicked: bar.view.clearSelection() }
        MText { text: qsTr("%n selected", "", bar.count); role: "titleS"; Layout.rightMargin: 8 }
        MButton { icon: "select_all"; compact: true; tooltip: qsTr("Select all"); onClicked: bar.view.selectAll() }
        MButton { icon: "favorite"; compact: true; tooltip: qsTr("Add to favourites"); onClicked: { PhotoLibrary.setFavourite(bar.paths(), true); bar.message(qsTr("Added %n to favourites", "", bar.count)); } }
        MButton { icon: "heart_minus"; compact: true; tooltip: qsTr("Remove from favourites"); onClicked: PhotoLibrary.setFavourite(bar.paths(), false) }
        MButton { icon: "content_copy"; compact: true; tooltip: qsTr("Copy"); onClicked: PhotoActions.copy(bar.paths()) }
        MButton { icon: "rotate_right"; compact: true; tooltip: qsTr("Rotate"); onClicked: PhotoActions.rotate(bar.paths(), 1) }
        MButton { icon: "delete"; compact: true; tooltip: qsTr("Move to the trash"); onClicked: { PhotoActions.trash(bar.paths()); bar.view.clearSelection(); } }
    }
}
