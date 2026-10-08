# Terminal — design decisions (round 1, 2026-10-08)

Renders: `artifacts/terminal/`, QML: `prototypes/terminal/`.

User verdict: "clean until you pop out the good session sidebar, and integrates into your shell normally but is EXTREMELY riceable".

- **Default chrome = A "Clean"**: pill tabs with status dots, tiled split panes with gaps and a focused-pane outline, shell-integration gutter marks (OSC 133: ok/fail/running), a result pill on the hovered command (copy output, rerun, bookmark), and a pane chip with the foreground program (OSC 7 cwd + process).
- **Sessions sidebar from C is hidden by default** and pops out (toolbar button / shortcut). Running and saved sessions; sessions survive closing the window. The command palette (snippets, SSH hosts, actions) from C is kept.
- It behaves as a normal terminal: plain shell integration, no custom shell, no account, no block-based UI.
- **Ricing depth = B, exactly**: the Appearance panel (theme gallery incl. Follow Opal, import kitty/alacritty/foot/ghostty themes, font + ligatures, size, opacity, cursor style, cursor trail, shaders incl. custom .glsl) and `~/.config/opal/terminal.conf` with live reload. Wallpaper/blur backgrounds, per-tab colours.
- **Phone**: key row + pinch-out pane pager are fine, but the on-screen keyboard (Borealis, `staging/keyboard`) must be massively improved, in general and with a terminal mode.
- No couch posture.
