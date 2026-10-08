# Text Editor — design decisions

Renders: `artifacts/text/`, QML: `prototypes/text/`.

## Round 1 (2026-10-08)
- **A "Writer" and B "Folder" both, with mode switching**: pill tabs, Markdown source / split / preview, status chips; open-folder mode with file tree + git marks, find in files, floating find/replace with regex.
- C "Focus" dropped.
- **Add IDE features** → round 2 explores them (LSP completion, diagnostics, hover, go to definition, symbols, git, integrated terminal on the Opal Terminal engine, tasks).
- Phone as rendered (Markdown tool row above the keyboard). No couch posture.

## Round 2 (2026-10-08) — LOCKED
- **Write / Code mode switch** (top-left segmented). Write = A (Markdown source/split/preview). Code = B + IDE.
- Code mode IDE scope, as rendered in `artifacts/text/DesktopIDE.png`: LSP client for any server (clangd, pyright, rust-analyzer, …) — completion popup with kind badges, diagnostics with squiggles + hover card + quick fix + go to definition, symbol breadcrumbs; sidebar tabs Files / Symbols / Git / Search with git marks and a branch card; Build task button (cmake/meson/cargo/npm detection, configurable tasks); bottom panel Terminal (the real Opal Terminal engine) / Problems / Git / Tasks; status chips incl. LSP state and error count.
- **MCP server for coding agents** (user request): the editor exposes an MCP endpoint (stdio + local socket, opt-in, per-workspace approval) with tools to list/open/read buffers and the selection, apply edits as reviewable diffs (accept/reject in the UI), query LSP (diagnostics, symbols, definition, references, hover), run tasks and read their output, and run commands in the integrated terminal. Edits from agents are highlighted and undoable as one step.
