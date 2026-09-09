#!/bin/bash
# v2 rollout round — kittyglow deploy + enable + kitty borderless rule (dom0)
# Note: This code is purely AI-generated.
# Runs as root in dom0 via: echo <b64> | base64 -d | sudo bash -s
set -u
P=/home/user/Projects/QubesOS/UI-Enhancements/Kwin/kitty-glow   # Dev-General path for qvm-run
echo "=== [1/5] install artifacts ==="
mkdir -p /usr/lib64/qt5/plugins/kwin/effects/plugins /usr/share/kwin/effects/kittyglow
rm -f /usr/lib64/qt5/plugins/kwin/effects/kittyglow.so
CURSHA=$(sha256sum /usr/lib64/qt5/plugins/kwin/effects/plugins/kittyglow.so 2>/dev/null | cut -d" " -f1)
if [ "$CURSHA" != "e2e9cef0b0423bf51df9851d19d8d7e77dec542b4849e4f957b04ebb0f920810" ]; then
  qvm-run -p Dev-General base64 -w0 "$P/dist/kittyglow.so" | base64 -d > /usr/lib64/qt5/plugins/kwin/effects/plugins/kittyglow.so
else
  echo ".so already correct, skip"
fi
qvm-run -p Dev-General base64 -w0 "$P/dist/kittyglow.json" | base64 -d > /usr/share/kwin/effects/kittyglow/metadata.json
qvm-run -p Dev-General base64 -w0 "$P/scripts/kittyglow-rollback.sh" | base64 -d > /tmp/kittyglow-rollback
install -m755 /tmp/kittyglow-rollback /usr/local/bin/kittyglow-rollback
chmod 644 /usr/lib64/qt5/plugins/kwin/effects/plugins/kittyglow.so /usr/share/kwin/effects/kittyglow/metadata.json
ls -la /usr/lib64/qt5/plugins/kwin/effects/plugins/kittyglow.so /usr/share/kwin/effects/kittyglow/metadata.json /usr/local/bin/kittyglow-rollback
echo "--- deployed .so sha256 (expect e2e9cef0...) ---"
sha256sum /usr/lib64/qt5/plugins/kwin/effects/plugins/kittyglow.so
echo "=== [2/5] kwinrc enable + kwinrulesrc rule (as chenpan) ==="
sudo -u chenpan env HOME=/home/chenpan bash -s <<'EOF'
kwriteconfig5 --file kwinrc --group Plugins --key kittyglowEnabled true || echo KWRITE_FAIL_kwinrc
kwriteconfig5 --file kwinrulesrc --group kitty-borderless --key noborderrule 2 || echo KWRITE_FAIL_rule
if kwriteconfig5 --help 2>&1 | grep -q -- --list-add; then
  kwriteconfig5 --file kwinrulesrc --group General --key rules --list-add kitty-borderless
else
  grep -q '^\[General\]' ~/.config/kwinrulesrc || printf '[General]\nrules=kitty-borderless\n' >> ~/.config/kwinrulesrc
fi
echo "--- kwinrc [Plugins] kittyglowEnabled ---"
kreadconfig5 --file kwinrc --group Plugins --key kittyglowEnabled
echo "--- kwinrulesrc after edit ---"
cat ~/.config/kwinrulesrc
EOF
echo "=== [3/5] KWin reconfigure (live reload, no restart) ==="
KPID=$(pgrep -x kwin_x11 | head -1)
[ -n "$KPID" ] || { echo KWIN_NOT_RUNNING; exit 1; }
eval "$(tr '\0' '\n' < /proc/$KPID/environ | grep -E '^(DBUS_SESSION_BUS_ADDRESS|XDG_RUNTIME_DIR)=' | sed 's/^/export /')"
echo "kwin pid: $KPID  bus: $DBUS_SESSION_BUS_ADDRESS"
sudo -u chenpan env DBUS_SESSION_BUS_ADDRESS="$DBUS_SESSION_BUS_ADDRESS" XDG_RUNTIME_DIR="$XDG_RUNTIME_DIR" \
  dbus-send --session --print-reply --dest=org.kde.KWin /KWin org.kde.kwin.KWin.reconfigure
sleep 2
echo "=== [4/5] verify effect loaded ==="
UOUT=$(sudo -u chenpan env DBUS_SESSION_BUS_ADDRESS="$DBUS_SESSION_BUS_ADDRESS" XDG_RUNTIME_DIR="$XDG_RUNTIME_DIR" \
  bash -c "dbus-send --session --print-reply --dest=org.kde.KWin /Effects org.kde.kwin.Effects.isEffectLoaded string:kittyglow 2>&1")
echo "$UOUT"
if ! echo "$UOUT" | grep -q "boolean true"; then
  echo "--- not loaded; explicit loadEffect retry ---"
  sudo -u chenpan env DBUS_SESSION_BUS_ADDRESS="$DBUS_SESSION_BUS_ADDRESS" XDG_RUNTIME_DIR="$XDG_RUNTIME_DIR" \
    bash -c "dbus-send --session --print-reply --dest=org.kde.KWin /Effects org.kde.kwin.Effects.loadEffect string:kittyglow 2>&1"
  sleep 1
  sudo -u chenpan env DBUS_SESSION_BUS_ADDRESS="$DBUS_SESSION_BUS_ADDRESS" XDG_RUNTIME_DIR="$XDG_RUNTIME_DIR" \
    bash -c "dbus-send --session --print-reply --dest=org.kde.KWin /Effects org.kde.kwin.Effects.isEffectLoaded string:kittyglow 2>&1"
fi
sudo -u chenpan env DBUS_SESSION_BUS_ADDRESS="$DBUS_SESSION_BUS_ADDRESS" XDG_RUNTIME_DIR="$XDG_RUNTIME_DIR" \
  bash -c "dbus-send --session --print-reply --dest=org.kde.KWin /Effects org.kde.kwin.Effects.loadedEffects 2>&1"
echo "=== [5/5] liveness + rule application ==="
pgrep -x kwin_x11 >/dev/null && echo KWIN_ALIVE || echo KWIN_DEAD
sudo -u chenpan env DBUS_SESSION_BUS_ADDRESS="$DBUS_SESSION_BUS_ADDRESS" XDG_RUNTIME_DIR="$XDG_RUNTIME_DIR" \
  bash -c "dbus-send --session --print-reply --dest=org.kde.KWin /KWin org.kde.kwin.KWin.supportInformation 2>/dev/null" \
  | grep -iE "kitty|noborder|border" | head -25
pgrep -a kitty || echo NO_KITTY_WINDOW_OPEN
echo ROUND_DONE
