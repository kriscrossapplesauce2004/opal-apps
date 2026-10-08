.pragma library
// Terminal fixture sessions. Each line is a list of [text, colour] spans; colour names map to the ANSI palette in TermView.
var build = [
    [["~/.local/state/opal/apps", "blue"], ["  main", "magenta"], [" +2 ?1", "yellow"]],
    [["❯ ", "green"], ["git", "fg"], [" status --short", "fg"]],
    [[" M ", "yellow"], ["material/qml/AppFrame.qml", "fg"]],
    [[" M ", "yellow"], ["prototypes/files/Couch.qml", "fg"]],
    [["?? ", "red"], ["prototypes/terminal/", "fg"]],
    [["", "fg"]],
    [["❯ ", "green"], ["cmake", "fg"], [" --build build", "fg"]],
    [["[1/6] ", "dim"], ["Automatic MOC and UIC for target OpalMaterial", "fg"]],
    [["[4/6] ", "dim"], ["Building CXX object material/…/palette.cpp.o", "fg"]],
    [["[6/6] ", "dim"], ["Linking CXX shared library ", "fg"], ["libOpalMaterial.so", "cyan"]],
    [["", "fg"]],
    [["❯ ", "red"], ["ctest", "fg"], [" --test-dir build", "fg"]],
    [["    Start 1: tonal", "fg"]],
    [["1/2 Test #1: tonal .................   ", "fg"], ["Passed", "green"], ["    0.12 sec", "dim"]],
    [["2/2 Test #2: posture ...............", "fg"], ["***Failed", "red"], ["  0.31 sec", "dim"]],
    [["50% tests passed, ", "fg"], ["1 tests failed", "red"], [" out of 2", "fg"]],
    [["", "fg"]],
    [["❯ ", "green"], ["nvim", "fg"], [" material/src/posture.cpp", "fg"]]
];
var ssh = [
    [["melissa@nas", "green"], [":", "fg"], ["~", "blue"], ["$ ", "fg"], ["zpool status -x", "fg"]],
    [["all pools are healthy", "green"]],
    [["melissa@nas", "green"], [":", "fg"], ["~", "blue"], ["$ ", "fg"], ["df -h /tank", "fg"]],
    [["Filesystem  Size  Used Avail Use% Mounted on", "dim"]],
    [["tank        7.2T  4.1T  3.1T  57% /tank", "fg"]],
    [["melissa@nas", "green"], [":", "fg"], ["~", "blue"], ["$ ", "fg"]]
];
var logo = [
    "                  -`",
    "                 .o+`",
    "                `ooo/",
    "               `+oooo:",
    "              `+oooooo:",
    "              -+oooooo+:",
    "            `/:-:++oooo+:",
    "           `/++++/+++++++:",
    "          `/++++++++++++++:",
    "         `/+++ooooooooooooo/`",
    "        ./ooosssso++osssssso+`",
    "       .oossssso-````/ossssss+`",
    "      -osssssso.      :ssssssso.",
    "     :osssssss/        osssso+++.",
    "    /ossssssss/        +ssssooo/-",
    "  `/ossssso+/:-        -:/+osssso+-",
    " `+sso+:-`                 `.-/+oso:"
];
var info = [
    [["melissa", "cyan"], ["@", "fg"], ["terra", "cyan"]],
    [["-------------", "dim"]],
    [["OS", "cyan"], [": CachyOS x86_64", "fg"]],
    [["Kernel", "cyan"], [": 7.2.8-1-cachyos-bore", "fg"]],
    [["Shell", "cyan"], [": fish 4.9.3", "fg"]],
    [["WM", "cyan"], [": Hyprland 0.56.2", "fg"]],
    [["Desktop", "cyan"], [": Opal", "fg"]],
    [["Terminal", "cyan"], [": Opal Terminal", "fg"]],
    [["CPU", "cyan"], [": Ryzen 5 5600X (12)", "fg"]],
    [["GPU", "cyan"], [": Radeon RX 6600 XT", "fg"]],
    [["Memory", "cyan"], [": 6.2 GiB / 15 GiB", "fg"]],
    [["", "fg"]],
    [["███", "red"], ["███", "green"], ["███", "yellow"], ["███", "blue"], ["███", "magenta"], ["███", "cyan"]]
];
function fastfetch() {
    var out = [[["❯ ", "green"], ["fastfetch", "fg"]]];
    for (var i = 0; i < logo.length; i++) {
        var pad = logo[i] + Array(Math.max(1, 38 - logo[i].length)).join(" ");
        out.push([[pad, "cyan"]].concat(info[i] || []));
    }
    return out;
}
