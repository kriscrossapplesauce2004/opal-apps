import QtQuick
import Opal.Photos
// Render check: the viewer with the details panel.
PhotosView { id: v; Timer { interval: 1500; running: true; onTriggered: { v.openViewer(v.timelineView().paths(), 2); v.viewerDetails = true; } } }
