import QtQuick
import Opal.Media
// Render check: the videos grid.
MediaView { id: v; Timer { interval: 2500; running: true; onTriggered: { v.go("videos"); } } }
