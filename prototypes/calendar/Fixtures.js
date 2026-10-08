.pragma library
// Generic calendar fixtures for the week of Mon 5 – Sun 11 Oct 2026. Day 0 is Monday; times in hours.
var cals = [
    {id: "personal", name: "Personal", c: "#7cc4a4"},
    {id: "work", name: "Opal", c: "#a8c8ff"},
    {id: "uni", name: "Classes", c: "#e7c87e"},
    {id: "birthdays", name: "Birthdays", c: "#e2b6f2"}
];
var events = [
    {d: 0, s: 9, e: 10.5, t: "Design review", cal: "work", where: "Video call"},
    {d: 0, s: 13, e: 14, t: "Lunch with Sam", cal: "personal", where: "Café Lumen"},
    {d: 1, s: 10, e: 12, t: "Signals lecture", cal: "uni", where: "Room 2.14"},
    {d: 1, s: 15, e: 16, t: "Gym", cal: "personal"},
    {d: 2, s: 9.5, e: 10, t: "Stand-up", cal: "work"},
    {d: 2, s: 11, e: 13, t: "Files backend pairing", cal: "work", where: "Room 3"},
    {d: 2, s: 18, e: 20, t: "Game night", cal: "personal", where: "Alex's place"},
    {d: 3, s: 9.5, e: 10, t: "Stand-up", cal: "work"},
    {d: 3, s: 14, e: 16, t: "Lab: oscilloscopes", cal: "uni", where: "Lab B"},
    {d: 3, s: 17, e: 18, t: "Dentist", cal: "personal", where: "High St"},
    {d: 4, s: 10, e: 11, t: "Media design review", cal: "work"},
    {d: 4, s: 19, e: 22, t: "Concert", cal: "personal", where: "The Roundhouse"},
    {d: 5, s: 11, e: 13, t: "Market", cal: "personal"},
    {d: 6, s: 15, e: 17, t: "Call family", cal: "personal"}
];
var allDay = [{d: 3, t: "Jordan's birthday", cal: "birthdays"}, {d: 4, d2: 6, t: "Weekend away", cal: "personal"}];
var days = ["Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"];
var today = 3;
function cal(id) { for (var i = 0; i < cals.length; i++) if (cals[i].id === id) return cals[i]; return cals[0]; }
function fmt(h) { var m = Math.round((h % 1) * 60); return Math.floor(h) + ":" + (m < 10 ? "0" : "") + m; }
var contacts = [
    {n: "Alex Rivera", p: "+44 7700 900123", e: "alex@example.org", c: "#a8c8ff"},
    {n: "Jordan Lee", p: "+44 7700 900456", e: "jordan@example.org", c: "#e2b6f2", bday: "8 Oct"},
    {n: "Sam Okafor", p: "+44 7700 900789", e: "sam@example.org", c: "#7cc4a4"},
    {n: "Morgan Patel", p: "+44 7700 900321", e: "morgan@example.org", c: "#e7c87e"},
    {n: "Riley Chen", p: "+44 7700 900654", e: "riley@example.org", c: "#f2a59d"}
];
