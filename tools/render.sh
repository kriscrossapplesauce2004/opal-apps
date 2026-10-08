#!/usr/bin/env bash
# Renders prototypes with the GPU scene graph and no visible window: render.sh <file.qml> <out.png> <WxH> <posture> [VAR=value...]
set -euo pipefail
root=$(cd "$(dirname "$0")/.." && pwd)
file=$1 out=$2 size=$3 posture=$4; shift 4
mkdir -p "$(dirname "$out")"
env OPAL_POSTURE="$posture" OPAL_NO_GAMEPAD=1 DBUS_SESSION_BUS_ADDRESS=unix:path=/nonexistent "$@" \
    "$root/build/tools/opal-render" "$file" "$out" "$size" "${OPAL_DELAY:-1200}"
