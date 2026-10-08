#!/usr/bin/env bash
# Renders QML with the GPU scene graph and no visible window: render.sh <file.qml> <out.png> <WxH> <posture> [VAR=value...]
# OPAL_RENDER_BIN picks another render binary, e.g. build/apps/files/opal-files-render.
set -euo pipefail
root=$(cd "$(dirname "$0")/.." && pwd)
file=$1 out=$2 size=$3 posture=$4; shift 4
mkdir -p "$(dirname "$out")"
env QT_FORCE_STDERR_LOGGING=1 OPAL_POSTURE="$posture" OPAL_NO_GAMEPAD=1 DBUS_SESSION_BUS_ADDRESS=unix:path=/nonexistent "$@" \
    "${OPAL_RENDER_BIN:-$root/build/tools/opal-render}" "$file" "$out" "$size" "${OPAL_DELAY:-1200}"
