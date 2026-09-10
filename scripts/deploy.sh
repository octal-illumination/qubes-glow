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
SO_DIR=/usr/lib64/qt5/plugins/kwin/effects/plugins
META_DIR=/usr/share/kwin/effects/kittyglow

[ -f "$SO" ] || { echo "Build first: scripts/build.sh"; exit 1; }

dom0 "sudo bash -lc 'mkdir -p $SO_DIR $META_DIR; qvm-run -p Dev-General base64 -w0 $SO | base64 -d > $SO_DIR/kittyglow.so; qvm-run -p Dev-General base64 -w0 $JSON | base64 -d > $META_DIR/metadata.json; chmod 644 $SO_DIR/kittyglow.so $META_DIR/metadata.json'"

# LL-022 gate (2026-09-10): a swallowed password-dialog failure used to exit 0
# with the PREVIOUS build left on dom0 (cost a full restart cycle on a phantom
# build). Verify the artifact identity before anything else claims success.
LOCAL_SHA=$(sha256sum "$SO" | cut -d' ' -f1)
DOM0_SHA=$(dom0 "sha256sum $SO_DIR/kittyglow.so" | grep -oE '^[0-9a-f]{64}' | head -1)
if [ "$LOCAL_SHA" != "$DOM0_SHA" ]; then
  echo "DEPLOY MISMATCH: local=$LOCAL_SHA dom0=${DOM0_SHA:-<none>}" >&2
  exit 1
fi
echo "Deploy verified: $DOM0_SHA"

dom0 "kwriteconfig5 --file kwinrc --group Plugins --key kittyglowEnabled true"

# kglowsync KWin script package — live-apply channel for the seamless
# Meta+Shift+B toggle (PoC-7: installed schema with X-Plasma-API/MainScript
# auto-runs at kwin start). Installed into the desktop user's KPackage path
# as chenpan (kwin scans the user scripts dir, not root's).
PKG_SRC="$ROOT/src/kwin-script/kglowsync"
PKG_DST=/home/chenpan/.local/share/kwin/scripts/kglowsync
dom0 "mkdir -p $PKG_DST/contents/code && \
qvm-run -p Dev-General base64 -w0 $PKG_SRC/metadata.json | base64 -d > $PKG_DST/metadata.json && \
qvm-run -p Dev-General base64 -w0 $PKG_SRC/contents/code/main.js | base64 -d > $PKG_DST/contents/code/main.js && \
chmod 644 $PKG_DST/metadata.json $PKG_DST/contents/code/main.js && \
kwriteconfig5 --file kwinrc --group Plugins --key kglowsyncEnabled true && \
kbuildsycoca5 --noincremental >/dev/null 2>&1 || true"

echo "Deployed to dom0 and enabled in kwinrc (incl. kglowsync script package; auto-runs at next kwin start)."
echo "Activate live (no restart), from dom0 as the desktop user:"
echo "  sudo -u chenpan env DBUS_SESSION_BUS_ADDRESS=unix:path=/run/user/1000/bus \\"
echo "    XDG_RUNTIME_DIR=/run/user/1000 \\"
echo "    dbus-send --session --print-reply --dest=org.kde.KWin /Effects \\"
echo "    org.kde.kwin.Effects.loadEffect string:kittyglow   # expect: boolean true"
echo "Full procedure and tuning: HANDBOOK.md sections 5 and 9."
