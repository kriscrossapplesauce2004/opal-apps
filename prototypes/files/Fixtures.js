.pragma library
// Static fixture data for the Files prototypes. Thumbnails are the user's own wallpapers, read locally only.
var wall = "file:///home/melissa/Pictures/Wallpapers/";
var places = [
    {section: "", items: [
        {id: "home", title: "Home", icon: "home"},
        {id: "recent", title: "Recent", icon: "schedule"},
        {id: "starred", title: "Starred", icon: "star"}]},
    {section: "Folders", items: [
        {id: "desktop", title: "Desktop", icon: "desktop_windows"},
        {id: "documents", title: "Documents", icon: "description"},
        {id: "downloads", title: "Downloads", icon: "download", badge: "3"},
        {id: "pictures", title: "Pictures", icon: "image"},
        {id: "music", title: "Music", icon: "music_note"},
        {id: "videos", title: "Videos", icon: "movie"}]},
    {section: "Devices", items: [
        {id: "t7", title: "Samsung T7", icon: "usb", detail: "412 GB free", eject: true},
        {id: "op7", title: "OnePlus 7", icon: "smartphone", detail: "Opal Link", eject: true}]},
    {section: "Network", items: [
        {id: "nas", title: "nas.local", icon: "dns", detail: "SMB"},
        {id: "add", title: "Connect to server", icon: "add_link"}]},
    {section: "", items: [{id: "trash", title: "Trash", icon: "delete", detail: "1.4 GB"}]}
];
var path = [{t: "Home", i: "home"}, {t: "Pictures"}, {t: "Wallpapers"}];
var files = [
    {n: "Deltarune", k: "folder", s: "38 items", m: "Today 14:02", c: "primary"},
    {n: "Screenshots", k: "folder", s: "1,204 items", m: "Today 11:40", c: "tertiary"},
    {n: "Phone camera", k: "folder", s: "Synced", m: "Yesterday", c: "secondary", sync: true},
    {n: "dr5-keyart.png", k: "image", s: "4.8 MB", m: "Today 13:58", t: wall + "dr5-keyart-YzI66DkClTeJ5jBQ.png", star: true},
    {n: "0j1z5q.jpeg", k: "image", s: "2.1 MB", m: "Today 09:12", t: wall + "0j1z5q.jpeg"},
    {n: "heat-death.webp", k: "image", s: "880 KB", m: "Yesterday", t: wall + "had-a-dream-the-heat-death-of-the-universe-happened-and-the-v0-y6au4moeftoh1.webp"},
    {n: "0pmme3.jpeg", k: "image", s: "3.3 MB", m: "Mon 22:41", t: wall + "0pmme3.jpeg"},
    {n: "3kg8qd.jpeg", k: "image", s: "1.9 MB", m: "Mon 22:40", t: wall + "3kg8qd.jpeg"},
    {n: "45jjo8.jpeg", k: "image", s: "2.7 MB", m: "Sun 18:03", t: wall + "45jjo8.jpeg"},
    {n: "47yz3e.jpeg", k: "image", s: "2.2 MB", m: "Sun 18:02", t: wall + "47yz3e.jpeg"},
    {n: "md6271.jpeg", k: "image", s: "1.6 MB", m: "2 Oct", t: wall + "md6271.jpeg"},
    {n: "mdwg31.jpeg", k: "image", s: "2.9 MB", m: "2 Oct", t: wall + "mdwg31.jpeg"},
    {n: "nrwe8w.jpeg", k: "image", s: "3.0 MB", m: "28 Sep", t: wall + "nrwe8w.jpeg"},
    {n: "qrodyl.jpeg", k: "image", s: "2.4 MB", m: "28 Sep", t: wall + "qrodyl.jpeg"},
    {n: "wallpaper-pack.zip", k: "archive", s: "212 MB", m: "27 Sep"},
    {n: "credits.txt", k: "text", s: "2 KB", m: "27 Sep"}
];
var downloads = [
    {n: "dlc040_nomads.zip", k: "archive", s: "1.2 GB", m: "Today 12:20", c: "tertiary"},
    {n: "AU1388.pdf", k: "pdf", s: "640 KB", m: "Today 10:03"},
    {n: "Hopebound", k: "folder", s: "12 items", m: "Yesterday", c: "primary"},
    {n: "chatgpt.x86_64", k: "app", s: "96 MB", m: "Mon"},
    {n: "Factorio-SteamRIP", k: "folder", s: "4 items", m: "Sun", c: "secondary"},
    {n: "7576-kris-deltarune.png", k: "image", s: "320 KB", m: "Sun", t: "file:///home/melissa/Pictures/7576-kris-deltarune.png"},
    {n: "click.webp", k: "image", s: "48 KB", m: "Sat", t: "file:///home/melissa/Pictures/click.webp"},
    {n: "opal-dock-gerbers.zip", k: "archive", s: "3.1 MB", m: "Fri"}
];
var glyph = {folder: "folder", image: "image", video: "movie", audio: "music_note", pdf: "picture_as_pdf", archive: "folder_zip", text: "description", app: "deployed_code", code: "code"};
var kindName = {folder: "Folder", image: "Image", video: "Video", audio: "Audio", pdf: "PDF document", archive: "Archive", text: "Text", app: "Program", code: "Source code"};
