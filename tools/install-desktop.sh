#!/bin/sh
# Register the apps from the build tree in the user's app drawer: tools/install-desktop.sh [--remove]
set -eu
root=$(cd "$(dirname "$0")/.." && pwd)
dest=${XDG_DATA_HOME:-$HOME/.local/share}/applications
icons=${XDG_DATA_HOME:-$HOME/.local/share}/icons/hicolor/scalable/apps
mkdir -p "$dest" "$icons"
for entry in "$root"/apps/*/data/*.desktop; do
    name=$(basename "$entry")
    icon="$(dirname "$entry")/${name%.desktop}.svg"
    if [ "${1:-}" = --remove ]; then
        rm -f "$dest/$name" "$icons/${name%.desktop}.svg"
        echo "removed $name"
        continue
    fi
    exe=$(sed -n 's/^Exec=\([^ ]*\).*/\1/p' "$entry")
    bin=$(find "$root/build/apps" -type f -name "$exe" -perm -u+x | head -n 1)
    # Skip apps that are not built, so the drawer shows no broken entries.
    if [ -z "$bin" ]; then
        echo "skipped $name: $exe is not built" >&2
        continue
    fi
    [ -f "$icon" ] && install -m 0644 "$icon" "$icons/"
    sed -e "s|^Exec=$exe|Exec=$bin|" -e "/^Exec=/a TryExec=$bin" "$entry" > "$dest/$name"
    echo "installed $name -> $bin"
done
command -v update-desktop-database >/dev/null && update-desktop-database "$dest" 2>/dev/null || true
command -v gtk-update-icon-cache >/dev/null && gtk-update-icon-cache -q -t "${icons%/scalable/apps}" 2>/dev/null || true
