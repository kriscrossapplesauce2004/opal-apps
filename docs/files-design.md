# Files — design decisions (round 1, 2026-10-08)

Source renders: `artifacts/files/` (contact sheet + per-posture PNGs), QML in `prototypes/files/`.

## Desktop (pointer) — base: A "Workbench"
The user screenshotted and approved, exactly as rendered:
- Left places panel: New FAB, Home/Recent/Starred, Folders, Devices (eject buttons), Network (SMB host, Connect to server), Trash with size, storage meter at the bottom.
- Tabs row (pill tabs with icons, close on the current tab, +).
- Toolbar: back / forward / up, breadcrumb chips (current folder filled).
- Details list as the default view: Name / Size / Modified (sorted, arrow) / Kind headers; type marks are tonal squircles or rounded thumbnails; starred mark; grouped selection with connected rounded rows; hover row.
- Status strip: item count, selection chip ("3 selected · 7.7 MB"), zoom slider.

Also kept (round-1 "must survive"): the inverse **operation pill** with wavy progress for running copies/moves, and **filter chips** (type filters + sort chip) above the content.

Split view is opt-in (toolbar toggle / F3), off by default — the approved crop is single-pane. Grid and Columns stay available from the view switcher.

## Phone (touch) — approved as rendered
- List: search pill with menu and overflow, breadcrumb chips, filter chips, 72 px rows with 52 px thumbnails, FAB, bottom nav (Browse / Recent / Starred / Devices).
- Selecting: contextual top bar ("3 selected", select all, overflow), three-column grid, selected tiles shrink and round with a check badge, floating icon toolbar replaces the FAB.

## Couch — approved as rendered ("ZOO WEE MAMA")
LB/RB tabs (Home / Recent / Devices / Network), breadcrumb chips, places shelf, big rounded tiles with a focus ring and grow, hint bar (A open, B up, X actions, Y search, LT page).
