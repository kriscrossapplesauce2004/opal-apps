.pragma library
.import "Fixtures.js" as F
// Grouping helpers for the Photos prototypes.
var months = ["Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"];
function label(d) {
    if (d === "2026-10-08") return "Today";
    if (d === "2026-10-07") return "Yesterday";
    var p = d.split("-"); return parseInt(p[2]) + " " + months[parseInt(p[1]) - 1];
}
function groups(list) {
    var out = [], cur = null;
    for (var i = 0; i < list.length; i++) {
        var p = list[i];
        if (!cur || cur.d !== p.d) { cur = {d: p.d, t: label(p.d), photos: []}; out.push(cur); }
        cur.photos.push(p);
    }
    return out;
}
function byAlbum(a) { return F.photos.filter(function (p) { return p.a === a; }); }
