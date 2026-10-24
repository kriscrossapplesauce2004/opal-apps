import QtQuick
import Opal.Media
// Render check: the songs list.
MediaView { id: v; Timer { interval: 2500; running: true; onTriggered: { v.go("music"); } } }
