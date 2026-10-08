# opal-apps — instructions for contributors and agents

Read `docs/SPEC.md` first; per-app decisions live in `docs/<app>-design.md`.

## Layout

- `material/` — `Opal.Material` QML module (C++ singletons `Palette`, `Posture`, `Gamepad`; QML components). Every app imports it. Reuse its components and `Theme` tokens; do not add unrelated styling.
- `apps/<name>/` — one app per directory: `CMakeLists.txt`, `src/` (C++), `qml/`, `tests/`, `data/` (desktop file, metainfo, icon), `flatpak/`.
- `prototypes/` — design-round QML renders. Not shipped. Use them as the visual reference for the real app.
- `tools/render.sh` + `build/tools/opal-render` — render any QML file to PNG with the GPU scene graph and no visible window: `tools/render.sh file.qml out.png 1440x900 pointer|touch|couch`.

## Build and test

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build
ctest --test-dir build --output-on-failure
```

Qt 6.11, C++20, CMake + Ninja. Register QML with `qt_add_qml_module`; C++ types use `QML_ELEMENT` / `QML_SINGLETON`. Tests use Qt Test (`QTEST_MAIN` / `QTEST_GUILESS_MAIN`) and are added with `add_test`.

## Code rules

- A code comment uses one line only, written in ASD-STE100 Simplified Technical English. Comment the why, not the obvious.
- Match the surrounding style: 4-space indent, `m_` members, `QStringLiteral`, no exceptions, no raw `new` without a parent or owner.
- Never block the GUI thread on disk, network or process I/O. Use worker threads (`QThread`/`QtConcurrent`) or async GIO.
- Use argument arrays for subprocesses, never a shell string.
- Missing optional tools or hardware degrade gracefully (feature hidden, not a crash).
- No telemetry, no network access unless the feature needs it, no persistence of user content beyond what the feature needs.
- Accessibility: every interactive QML item sets `Accessible.role` and `Accessible.name`.

## Git

Commit in small logical steps with clear messages. End commit messages with:

```
Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>
```
