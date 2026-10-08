# Borealis keyboard — design decisions

Borealis lives in the shell repo (`staging/keyboard`, binary `opal-keyboard`). Prototypes: `prototypes/keyboard/`, renders: `artifacts/keyboard/`.

## Round 1 (2026-10-08)
- Keep: clipboard panel (Opal clipboard history), terminal mode (Esc/Tab/Ctrl/Alt/arrows row, digits on long-press, shell symbols, latched Ctrl, no autocorrect), hold-space trackpad, split layout on tablets/landscape.
- Key style is a setting: tonal keys (default) or borderless.
- Tools must live in their own little menu, not a chip strip that replaces suggestions.
- "More improvement is needed" → round 2 explores more.
- Glide typing was not picked in round 1 (open question).

## Round 2 (2026-10-08) — LOCKED ("I LOVE IT ALL")
- **Tools menu**: the apps button at the left of the suggestion bar morphs into a 4×3 tool grid (Clipboard with count badge, Emoji, Text editing, Trackpad, Terminal keys, One-handed, Floating, Resize, Languages, Symbols, Theme, Settings). No chip strip.
- **Panels** replace the key area with a title bar (keyboard button to return, title, settings): Text editing (arrow pad around a Select toggle; line start/end, word ←/→, select all; cut/copy/paste/undo/redo), Emoji (search, category tabs, recent, grid), Clipboard (Opal clipboard history cards: pinned, code, links, images, colours), Trackpad (also on hold-space).
- **Terminal mode**: Esc/Tab/Ctrl/Alt/arrow row replaces the digit row (digits as hints on long-press), shell symbols in the bottom row, latched Ctrl shown filled, suggestion bar becomes Paste / Ctrl C / Tab.
- **Floating keyboard** on tablet/desktop: compact card with drag handle and resize corner; double-tap handle docks.
- **Docked bar**: with a physical keyboard attached the deck folds into a slim bar (suggestions, emoji, clipboard, show-keyboard).
- **Controller keyboard** (couch/handheld): big keys, growing focus ring, suggestions on LB/RB, hint bar A type / B delete / X space / Y shift / LT symbols / RT cursor / Start done.
- **Split** on tablets and landscape. Key style setting: tonal (default) or borderless.
- Glide typing: later.
