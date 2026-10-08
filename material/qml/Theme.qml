pragma Singleton
import QtQuick
import Opal.Material

// Shared design tokens. The API matches the shell's Theme.qml; colours come from Palette and sizes follow Posture.
QtObject {
    id: theme
    readonly property bool light: Palette.light
    readonly property bool reduceMotion: Palette.reduceMotion
    function motion(ms) { return reduceMotion ? 0 : ms; }
    readonly property var roles: Palette.roles
    function role(name, fallback) { return roles[name] || fallback; }

    // M3 colour roles.
    readonly property color primary: role("primary", "#cfbdff")
    readonly property color primaryInk: role("on_primary", "#31205f")
    readonly property alias onPrimary: theme.primaryInk
    readonly property color primaryContainer: role("primary_container", "#4a3a7a")
    readonly property color containerInk: role("on_primary_container", "#e9ddff")
    readonly property alias onContainer: theme.containerInk
    readonly property color secondary: role("secondary", "#cbc2db")
    readonly property color secondaryContainer: role("secondary_container", "#4a4458")
    // QML reads "on" + capital names as signal handlers, so the colours live under "Ink" names with aliases.
    readonly property color secondaryInk: role("on_secondary_container", "#e8def8")
    readonly property alias onSecondaryContainer: theme.secondaryInk
    readonly property color tertiary: role("tertiary", "#efb8c8")
    readonly property color tertiaryContainer: role("tertiary_container", "#633b48")
    readonly property color tertiaryInk: role("on_tertiary_container", "#ffd8e4")
    readonly property alias onTertiaryContainer: theme.tertiaryInk
    readonly property color error: role("error", "#ffb4ab")
    readonly property color errorContainer: role("error_container", "#93000a")
    readonly property color errorInk: role("on_error_container", "#ffdad6")
    readonly property color base: role("background", "#17141f")
    readonly property color text: role("on_surface", "#e7e0ec")
    readonly property color subtext: role("on_surface_variant", "#cac4d0")
    readonly property color muted: alpha(subtext, .72)
    readonly property color outlineStrong: role("outline", "#938f99")
    readonly property color outline: alpha(role("outline_variant", "#49454f"), light ? .9 : .8)
    readonly property color inverseSurface: role("inverse_surface", "#e6e0e9")
    readonly property color inverseInk: role("inverse_on_surface", "#322f35")
    readonly property color inversePrimary: role("inverse_primary", "#6750a4")

    // M3 surface container ramp.
    readonly property color surfaceLowest: role("surface_container_lowest", "#0f0d13")
    readonly property color surfaceLow: role("surface_container_low", "#1d1b20")
    readonly property color surface: role("surface_container", "#211f26")
    readonly property color surfaceHigh: role("surface_container_high", "#2b2930")
    readonly property color surfaceHighest: role("surface_container_highest", "#36343b")
    readonly property color surfaceBright: role("surface_bright", "#3b383e")
    // Tonal elevation: level 0 is the base and level 5 is the highest container.
    function level(n) { return [base, surfaceLow, surface, surfaceHigh, surfaceHighest, mix(surfaceHighest, primaryContainer, light ? .35 : .25)][Math.max(0, Math.min(5, n))]; }

    // Opal glass roles, the same recipes as the shell.
    readonly property color glassTint: mix(base, light ? primaryContainer : primary, light ? .26 : .19)
    readonly property color glassEdge: alpha(light ? "#ffffff" : primary, light ? .48 : .18)
    readonly property color glassIsland: mix(mix(base, "#ffffff", light ? .25 : .06), light ? primaryContainer : primary, light ? .5 : .24)
    readonly property color glassRaised: mix(mix(base, "#ffffff", light ? .3 : .08), light ? "#ffffff" : primaryContainer, light ? .62 : .42)
    readonly property color glassHighlight: alpha("#ffffff", light ? .6 : .12)
    readonly property color glassShade: alpha("#000000", light ? .05 : .22)
    readonly property color tile: mix(base, light ? primaryContainer : primary, light ? .32 : .24)
    readonly property color tileActive: primary
    readonly property alias onTileActive: theme.primaryInk
    readonly property color stateLayer: text
    readonly property real glassFloor: .9
    function panelAlpha(t) { return Math.max(glassFloor, Math.min(1, t)); }

    // Posture scale: couch makes type and targets bigger for a 10-foot view.
    readonly property bool couch: Posture.couch
    readonly property bool touch: Posture.touch
    readonly property real scale: couch ? 1.4 : 1
    // Minimum hit target for interactive controls.
    readonly property int target: couch ? 64 : touch ? 48 : 36
    // Row height for list items with one line of text.
    readonly property int row: couch ? 72 : touch ? 56 : 44
    function px(n) { return Math.round(n * scale); }

    // M3 shape scale.
    readonly property int shapeXs: 8
    readonly property int shapeSm: 12
    readonly property int shapeMd: 16
    readonly property int shapeLg: 24
    readonly property int shapeXl: 32
    readonly property int shapeXxl: 48

    // M3 expressive motion: standard and spatial curves as cubic Bezier lists.
    readonly property var emphasized: [0.2, 0, 0, 1, 1, 1]
    readonly property var emphasizedDecelerate: [0.05, 0.7, 0.1, 1, 1, 1]
    readonly property var emphasizedAccelerate: [0.3, 0, 0.8, 0.15, 1, 1]
    readonly property var spatial: [0.2, 0, 0, 1, 1, 1]
    readonly property var standard: [0.2, 0, 0, 1, 1, 1]
    // Spring with overshoot for shape changes that should feel physical.
    readonly property var springy: [0.34, 1.56, 0.64, 1, 1, 1]
    readonly property int durShort: 140
    readonly property int durMedium: 240
    readonly property int durLong: 360

    // Type scale. Display roles use the wide, round axes of Google Sans Flex.
    readonly property string font: "Google Sans Flex"
    readonly property string mono: "monospace"
    readonly property var type: ({
        displayL: {size: px(64), weight: Font.Black, width: 125, round: 100},
        displayM: {size: px(48), weight: Font.ExtraBold, width: 120, round: 100},
        displayS: {size: px(36), weight: Font.Bold, width: 115, round: 80},
        headline: {size: px(26), weight: Font.Bold, width: 110, round: 60},
        title: {size: px(18), weight: Font.DemiBold, width: 100, round: 40},
        titleS: {size: px(15), weight: Font.DemiBold, width: 100, round: 30},
        body: {size: px(14), weight: Font.Normal, width: 100, round: 0},
        bodyS: {size: px(13), weight: Font.Normal, width: 100, round: 0},
        label: {size: px(12), weight: Font.Medium, width: 100, round: 20},
        labelS: {size: px(11), weight: Font.Medium, width: 100, round: 20}
    })

    // Standard spacing steps.
    readonly property int gapXs: 4
    readonly property int gapSm: 8
    readonly property int gapMd: 12
    readonly property int gapLg: 16
    readonly property int gapXl: 24

    function mix(a, b, amount) { const x = Qt.color(a), y = Qt.color(b); return Qt.rgba(x.r + (y.r - x.r) * amount, x.g + (y.g - x.g) * amount, x.b + (y.b - x.b) * amount, 1); }
    function alpha(c, a) { const v = Qt.color(c); return Qt.rgba(v.r, v.g, v.b, a); }

    // Short names for Material Symbols. Any other string is used as the symbol name.
    readonly property var glyphs: ({apps: "apps", search: "search", close: "close", back: "arrow_back", next: "chevron_right", home: "home", settings: "tune", folder: "folder", file: "draft", terminal: "terminal", music: "music_note", video: "movie", photo: "photo", grid: "grid_view", list: "view_list", calendar: "calendar_month", add: "add", check: "check", trash: "delete", copy: "content_copy", play: "play_arrow", pause: "pause", previous: "skip_previous", skip: "skip_next", more: "more_vert", menu: "menu", expand: "expand_more", sort: "sort", share: "share", star: "star", heart: "favorite", download: "download", cloud: "cloud", usb: "usb", phone: "smartphone", desktop: "desktop_windows", game: "sports_esports", link: "link", lock: "lock", edit: "edit", info: "info", split: "vertical_split", tab: "tab", radio: "radio", book: "auto_stories", podcast: "podcasts"})
}
