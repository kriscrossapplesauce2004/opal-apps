import QtQuick
import Opal.Photos
// Render check: the viewer on the newest photo.
PhotosView { id: v; Timer { interval: 1500; running: true; onTriggered: v.openViewer(v.timelineView().paths(), 0) } }
