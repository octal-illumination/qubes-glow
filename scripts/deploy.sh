#!/usr/bin/env bash
# Note: This code is purely AI-generated.
#
# Deploys the built effect into dom0 and enables it in kwinrc.
# Does NOT restart KWin -- that is a separate, explicit step (see HANDBOOK).
#
# Transfer uses `qvm-run` (dom0 -> Dev-General exec) instead of `qvm-copy`,
# because copying a file INTO dom0 via the Filecopy RPC was refused in practice.
set -euo pipefail

export PATH="$HOME/.local/bin:$PATH"
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
DIST="$ROOT/dist"
SO="$DIST/kittyglow.so"
JSON="$DIST/kittyglow.json"
SO_DIR=/usr/lib64/qt5/plugins/kwin/effects
META_DIR=/usr/share/kwin/effects/kittyglow

[ -f "$SO" ] || { echo "Build first: scripts/build.sh"; exit 1; }

dom0 "sudo bash -lc 'mkdir -p $SO_DIR $META_DIR; qvm-run -p Dev-General base64 -w0 $SO | base64 -d > $SO_DIR/kittyglow.so; qvm-run -p Dev-General base64 -w0 $JSON | base64 -d > $META_DIR/metadata.json; chmod 644 $SO_DIR/kittyglow.so $META_DIR/metadata.json'"

dom0 "kwriteconfig5 --file kwinrc --group Plugins --key kittyglowEnabled true"

echo "Deployed to dom0 and enabled in kwinrc."
echo "Activate with a KWin restart (requires your approval):"
echo "  eval \$(cat /proc/\$(pgrep -x kwin_x11)/environ | tr '\\0' '\\n' | grep -E 'DISPLAY|XAUTHORITY|PATH|XDG' | sed 's/^/export /')"
echo "  setsid nohup kwin_x11 --replace >/tmp/kwin-restart.log 2>&1 &"
