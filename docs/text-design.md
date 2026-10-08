# Text Editor — design decisions

Renders: `artifacts/text/`, QML: `prototypes/text/`.

## Round 1 (2026-10-08)
- **A "Writer" and B "Folder" both, with mode switching**: pill tabs, Markdown source / split / preview, status chips; open-folder mode with file tree + git marks, find in files, floating find/replace with regex.
- C "Focus" dropped.
- **Add IDE features** → round 2 explores them (LSP completion, diagnostics, hover, go to definition, symbols, git, integrated terminal on the Opal Terminal engine, tasks).
- Phone as rendered (Markdown tool row above the keyboard). No couch posture.
