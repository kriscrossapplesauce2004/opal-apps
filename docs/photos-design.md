# Photos — design decisions (round 1, 2026-10-08) — LOCKED

Renders: `artifacts/photos/`, QML: `prototypes/photos/` (Justified.qml is the justified-row layout).

User picked all three desktop directions; they are the app's three views:
- **Timeline** (home): justified rows that keep aspect ratios, grouped by day, month scrubber on the right, search incl. text in screenshots, Years/Months/Days switch, slideshow.
- **Albums**: folders as albums with 2×2 mosaic covers, source chips (this PC, phone over Opal Link/MTP, USB, network).
- **Viewer + details**: dark stage, floating action toolbar (favourite, edit, crop, share, set wallpaper, info, delete), filmstrip, details panel (caption, date, file, folder, dominant colours, albums, Show in Files).
- **Editor**: tool rail (Auto, Crop, Adjust, Filters, Markup), expressive adjustment sliders, before/after split, Cancel / Save copy / Save.
- Phone timeline + viewer and couch as rendered (not objected to).
