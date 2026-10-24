import QtQuick
import Opal.Photos
// Render check: the albums page.
PhotosView { id: v; Timer { interval: 300; running: true; onTriggered: v.go(1) } }
