import QtQuick
import QtQuick.Controls
import Opal.Material
import Opal.Files
// Inline rename: selects the name without its extension; Enter renames, Escape cancels.
TextField {
    id: field
    property string uri: ""
    property string original: ""
    signal done()
    text: original
    color: Theme.text
    font.family: Theme.font; font.pixelSize: 13
    selectionColor: Theme.alpha(Theme.primary, .4)
    background: Rectangle { radius: 8; color: Theme.surfaceHighest; border.width: 2; border.color: Theme.primary }
    Component.onCompleted: { forceActiveFocus(); const dot = original.lastIndexOf("."); select(0, dot > 0 ? dot : original.length); }
    onAccepted: { if (text !== original && text.trim() !== "") FileOperations.rename(uri, text.trim()); done(); }
    Keys.onEscapePressed: done()
    onActiveFocusChanged: if (!activeFocus) done()
}
