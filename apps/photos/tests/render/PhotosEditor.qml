import QtQuick
import Opal.Photos
// Render check: the editor on the Adjust tool.
PhotosView { id: v; Timer { interval: 1500; running: true; onTriggered: v.editPath = v.timelineView().paths()[2] } }
