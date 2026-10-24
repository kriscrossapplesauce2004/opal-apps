import QtQuick
import Opal.Media
// Render check: an album page.
MediaView { id: v; Timer { interval: 2500; running: true; onTriggered: { const a = MediaLibrary.albums(); if (a.length) { Player.enqueue(MediaLibrary.album(a[0].key).tracks); v.openAlbum(a[0].key); } } } }
