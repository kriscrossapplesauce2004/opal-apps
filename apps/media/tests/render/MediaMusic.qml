import QtQuick
import Opal.Media
// Render check: the album grid with the now-playing panel.
MediaView { id: v; Timer { interval: 2500; running: true; onTriggered: { v.go("music"); const a = MediaLibrary.albums(); if (a.length) Player.enqueue(MediaLibrary.album(a[0].key).tracks); } } }
