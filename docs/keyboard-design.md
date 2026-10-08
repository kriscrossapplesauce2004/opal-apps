# Borealis keyboard — design decisions

Borealis lives in the shell repo (`staging/keyboard`, binary `opal-keyboard`). Prototypes: `prototypes/keyboard/`, renders: `artifacts/keyboard/`.

## Round 1 (2026-10-08)
- Keep: clipboard panel (Opal clipboard history), terminal mode (Esc/Tab/Ctrl/Alt/arrows row, digits on long-press, shell symbols, latched Ctrl, no autocorrect), hold-space trackpad, split layout on tablets/landscape.
- Key style is a setting: tonal keys (default) or borderless.
- Tools must live in their own little menu, not a chip strip that replaces suggestions.
- "More improvement is needed" → round 2 explores more.
- Glide typing was not picked in round 1 (open question).
