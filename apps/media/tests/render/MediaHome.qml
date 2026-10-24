import QtQuick
import Opal.Media
// Render check: home with an album queued.
MediaView { id: v; Timer { interval: 2500; running: true; onTriggered: { const a = MediaLibrary.albums(); if (a.length) Player.enqueue(MediaLibrary.album(a[0].key).tracks); } } }
