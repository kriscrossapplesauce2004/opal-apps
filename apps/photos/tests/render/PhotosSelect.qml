import QtQuick
import Opal.Photos
// Render check: three photos selected and the selection bar.
PhotosView { id: v; Timer { interval: 1600; running: true; onTriggered: { for (let i = 1; i < 4; i++) v.timelineView().select(i, true); } } }
