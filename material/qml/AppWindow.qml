import QtQuick
import QtQuick.Controls
import Opal.Material
// Top-level window for an Opal app. seed is the app's signature colour, used when neither Opal nor the system gives one.
ApplicationWindow {
    id: window
    property color seed: "#7355ad"
    property bool allowCouch: true
    width: 1180; height: 760
    minimumWidth: 320; minimumHeight: 400
    visible: true
    color: Theme.base
    font.family: Theme.font
    Component.onCompleted: { Palette.seed = seed; Posture.allowCouch = allowCouch; }
    onSeedChanged: Palette.seed = seed
    onAllowCouchChanged: Posture.allowCouch = allowCouch
    // Couch on a handheld or TV fills the screen.
    visibility: Posture.couch ? Window.FullScreen : Window.AutomaticVisibility
}
