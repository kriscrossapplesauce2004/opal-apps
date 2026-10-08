# Opal apps — suite specification

Decided in the 2026-10-07 grilling. This file is the source of truth for scope; change it when a decision changes.

## Goal

A set of delightful, independent Linux apps that share Opal's Material 3 Expressive language and work equally well with a mouse, a touchscreen and a game controller. They are not shell widgets: every app installs and runs on its own on any distro and any desktop (GNOME, KDE, phones running postmarketOS, handhelds).

## Lineup (this effort)

| App | ID | Scope |
|---|---|---|
| Files | `dev.opal.Files` | **Flagship — first priority** ("Dolphin is getting sucky"). Must match Dolphin's power: tabs, split panes, fast search, archives, network places (SMB/SFTP/MTP via gio/kio-free backends), batch rename, permissions, trash, thumbnails for everything. Grid/list/columns on desktop, touch grid on phone, shelf on couch. Shares the thumbnail engine with Photos. |
| Photos | `dev.opal.Photos` | Gallery and image viewer: timeline, albums (folders), viewer with zoom/pan, basic edits later. |
| Media | `dev.opal.Media` | Music + video + internet radio/streams + Jellyfin/Subsonic libraries + audiobooks. One queue, one now-playing surface, MPRIS. Video on libmpv. |
| Terminal | `dev.opal.Terminal` | **Second priority, and very riceable.** Plain-text config (`~/.config/opal/terminal.conf`) with live reload; colours follow Opal or any imported kitty/alacritty/foot/ghostty theme; font, ligatures, padding, opacity and blur, cursor shapes and trails, background images, custom post-process shaders, remappable keybinds and phone key row. Own VT core (libvterm) with a glyph-atlas scene-graph renderer. Phone extra-key row and gestures, desktop splits that become a pager on phone, tabs, sessions that survive closing the window. **No couch posture.** |
| Text Editor | `dev.opal.TextEditor` | Plain text and Markdown: tabs, find/replace, syntax highlighting. Phone opens into the document. **No couch posture.** |
| Documents | `dev.opal.Documents` | PDF (poppler), EPUB and comics (CBZ). Paged on desktop, reflow on phone, controller page turns. |
| Assistant | `dev.opal.Assistant` | Optional app for those who want it: all Opal assistant functionality (Claude chat, AT-SPI computer use, approvals, step view) moves out of the shell into this app. The shell keeps only a thin Super+A / Copilot-key hook that opens it, and hides the hook when the app is not installed. |
| Calendar | `dev.opal.Calendar` | Calendar + Contacts over CalDAV/CardDAV with a local store. Agenda on phone, week grid on desktop. |

Deferred (not this effort): Clock, Calculator, Notes (three separate small apps — not merged), Settings (would move out of the shell: generic pages plus Opal pages over IPC), Software (Flathub + pacman/apk updates).

Names are plain on purpose: "Files", "Photos", and so on. No cute or obscure names.

## Technology

- Qt 6 Quick + C++. No Python runtime in apps.
- Shared QML module `Opal.Material` (this repo, `material/`). The shell will import it too once it is stable, so shell and apps share one implementation.
- Gamepad input through SDL3 with HIDAPI disabled (evdev only, read-only; never grab devices).
- Flutter was rejected: there is no official Material 3 Expressive for Flutter.

## Colour

Resolution order:
1. Opal is present: Opal's live palette from `~/.local/state/opal/preferences.json` (matugen dynamic palette or the chosen static palette), live-updating.
2. Otherwise the system accent colour from `org.freedesktop.portal.Settings` (`org.freedesktop.appearance` `accent-color`), expanded to a full M3 tonal-spot scheme.
3. Otherwise the app's own signature seed colour.

Light/dark follows Opal when present, else the portal `color-scheme`. `OPAL_THEME=opal|portal|seed` and `OPAL_COLOR_SCHEME=light|dark` override for testing.

## Postures

Every app has three postures, chosen at runtime by `Posture`:

- **Pointer** — desktop density, hover, right-click menus, keyboard shortcuts.
- **Touch** — 48 dp targets, gestures, bottom sheets, bottom navigation when narrow.
- **Couch** — 10-foot layout for game mode, TVs and handhelds: big targets, horizontal shelves, hint bar, on-screen keyboard for text.

A controller used on a desktop or phone does not switch to couch: it drives the current layout with a focus ring and a hint bar. Couch is entered when the context is couch (Opal console/game mode, gamescope, a known handheld, or a TV-sized screen) or by `OPAL_POSTURE=couch`.

Width classes are separate from posture: compact (< 600), medium (< 840), expanded (>= 840).

## Design rules

- Real Material 3 Expressive components, Opal flavour (tonal glass, springy shape changes, morphing card → row → chip).
- Chips where they earn it — not everywhere.
- No gimmicks. Apps are real Linux apps (real files, real windows, portals).
- Every app gets a rendered design round (QML prototypes in three postures, contact sheet) before its code is written.

## Shipping

- One monorepo (`opal-apps`). Each app has its own Flatpak manifest (Flathub-ready) and native packaging (AUR PKGBUILD, postmarketOS APKBUILD).
- Fonts: Google Sans Flex (OFL) and Material Symbols Rounded (Apache-2.0), bundled in Flatpaks.

## Process

1. `Opal.Material` module + render harness.
2. Design round per app, Files first, Terminal second (wave 1: Files, Photos, Media, Terminal). The user picks and mixes.
3. Build in waves of at most three parallel workstreams: wave 1 Files+Photos / Media / Terminal, wave 2 Text Editor / Documents / Calendar.
