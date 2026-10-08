.pragma library
// Media fixtures: the user's own soundtracks and videos, with local Steam art. Radio and audiobooks use public names and generated covers.
var art = "file:///home/melissa/.local/state/opal/apps/artifacts/media/art/";
var wall = "file:///home/melissa/Pictures/Wallpapers/";
var albums = [
    {t: "DELTARUNE Chapter 1", a: "Toby Fox", y: "2018", n: 40, img: art + "dr-capsule.jpg"},
    {t: "DELTARUNE Chapter 2", a: "Toby Fox", y: "2021", n: 47, img: art + "dr-hero.jpg"},
    {t: "DELTARUNE Chapters 3+4", a: "Toby Fox", y: "2025", n: 78, img: wall + "dr5-keyart-YzI66DkClTeJ5jBQ.png"},
    {t: "UNDERTALE", a: "Toby Fox", y: "2015", n: 101, img: art + "ut-capsule.jpg"},
    {t: "Mario & Luigi: Dream Team", a: "Yoko Shimomura", y: "2013", n: 1, img: art + "ml-dream.jpg"},
    {t: "UNDERTALE (Hero)", a: "Toby Fox", y: "2015", n: 3, img: art + "ut-hero.jpg"}
];
var now = {t: "Field of Hopes and Dreams", a: "Toby Fox", al: "DELTARUNE Chapter 1", img: art + "dr-capsule.jpg", pos: "1:27", len: "3:12", p: .45};
var queue = [
    {n: 13, t: "Field of Hopes and Dreams", d: "3:12", cur: true},
    {n: 14, t: "Fanfare", d: "0:09"},
    {n: 15, t: "Lantern", d: "1:01"},
    {n: 16, t: "I'm Very Bad", d: "0:58"},
    {n: 17, t: "Checker Dance", d: "1:35"},
    {n: 18, t: "Quiet Autumn", d: "1:16"},
    {n: 19, t: "Scarlet Forest", d: "2:43"},
    {n: 20, t: "Thrash Machine", d: "1:20"}
];
var videos = [
    {t: "game.mp4", m: "Videos · 12:40", p: .62, img: art + "video1.jpg"},
    {t: "clip_1784327291046", m: "Videos · 0:48", p: 0, img: art + "video2.jpg"},
    {t: "8mb.video-mom", m: "Videos · 1:05", p: .2, img: art + "video3.jpg"}
];
var radio = [
    {t: "Groove Salad", a: "SomaFM · ambient", c: "#7cc4a4", i: "radio"},
    {t: "NTS 1", a: "London · live", c: "#e7c87e", i: "graphic_eq"},
    {t: "KEXP", a: "Seattle · indie", c: "#a8c8ff", i: "podcasts"},
    {t: "Lofi Girl", a: "stream · lofi", c: "#e2b6f2", i: "headphones"}
];
var books = [
    {t: "Frankenstein", a: "Mary Shelley · LibriVox", p: .38, ch: "Chapter 9 of 24", left: "5 h 12 m left", c: "#5a4a7a"},
    {t: "Dracula", a: "Bram Stoker · LibriVox", p: .07, ch: "Chapter 2 of 27", left: "14 h left", c: "#7a3b3b"},
    {t: "The Time Machine", a: "H. G. Wells · LibriVox", p: .9, ch: "Chapter 11 of 12", left: "22 m left", c: "#2f5d62"}
];
