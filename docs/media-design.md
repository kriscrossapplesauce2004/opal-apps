# Media — design decisions

Renders: `artifacts/media/`, QML: `prototypes/media/`.

## Round 1 (2026-10-08)
- Desktop base: **A "Library + now playing panel"** — "GORGEOUS". Expanded rail (Home, Music, Videos, Radio, Audiobooks, servers), segmented Albums/Artists/Songs/Playlists, filter chips, album grid with the playing album marked, right now-playing panel (art, title, wavy seek, transport with shape-morphing play button, output chip, up next).
- Phone home: approved except the **floating mini-player pill clashes** — the mini player must belong to the bottom bar surface.
- Phone full player: "could be better" → round 2.
- Immersive (full-screen player): the super-rounded control pill is "bleh".
- Audiobook player and couch: approved.

## Round 2–3 (2026-10-08) — LOCKED
- Full player: **v2 full bleed** (art edge to edge fading into an art-tinted background, huge title), colours calmed (`Palette.schemeFor(seed, light, .5)`), and output / lyrics / queue as **one pill split into three buttons** (ChromeOS style, `prototypes/media/SplitPill.qml`). Render: `artifacts/media/PlayerBleed3.png`.
- Phone home: **docked mini player** on the bottom bar's surface, fully rounded (also at the bottom), progress on its top edge. Render: `PhoneHome3.png`.
- Immersive (desktop full screen): **cover fills the height on the left; title, wavy seek, transport, volume, split pill and the queue share one column on the right**; calm art tint over blurred art. Render: `DesktopImmersive3.png`.
- Album-art tinting applies **only** to the full player and immersive view; the rest of the app stays in Opal colours.
