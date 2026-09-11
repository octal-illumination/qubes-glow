#!/bin/bash
# kittyglow-rollback — one-command escape hatch for the kittyglow KWin effect.
# Note: This code is purely AI-generated.
#
# Removes every trace of the kitty customizations — the kittyglow compositor
# effect AND the kglowsync live-sync KWin script (which enforces the last
# commanded borderless state on every eligible window) plus the RETIRED
# kitty-toggle-border script (whose over-broad window matching stripped
# titlebars/borders from ALL windows, konsole included) — and hands you a
# working desktop. Re-audit 2 M5: kglowsync MUST be stripped too, or new
# windows keep spawning borderless after the "clean" rollback.
# Safe to run from ANY dom0 terminal or TTY, in ANY state (KWin up or down,
# effect loaded or not, xfwm4 running).
#
# Usage:
#   kittyglow-rollback          clean KWin restart WITHOUT kittyglow
#                               (if xfwm4 is running, this replaces it with KWin)
#   kittyglow-rollback xfwm4    strip the effect AND switch to xfwm4
#   kittyglow-rollback only     just strip the effect, touch no window manager
set -u

SO="/usr/lib64/qt5/plugins/kwin/effects/plugins/kittyglow.so"
KWINRC="$HOME/.config/kwinrc"

# --- inherit the graphical session env when run from a TTY/service ------
# Probe any live session process so this works even if KWin is NOT the
# current WM (e.g. xfwm4 fallback after a crash). HOME is included because
# kwinrc edits must target the SESSION USER's home, never the invoker's.
if [ -z "${DISPLAY:-}" ] || [ -z "${DBUS_SESSION_BUS_ADDRESS:-}" ]; then
    KP=""
    for PROC in kwin_x11 plasmashell xfwm4 xfce4-session Xorg; do
        KP="$(pgrep -x "$PROC" | head -1)"
        [ -n "$KP" ] && break
    done
    if [ -n "$KP" ]; then
        eval "$(tr '\0' '\n' < "/proc/$KP/environ" \
            | grep -E '^(DISPLAY|DBUS_SESSION_BUS_ADDRESS|XAUTHORITY|HOME)=' \
            | sed 's/^/export /')"
    fi
fi

echo "[1/4] unload effect + border script from running KWin (if loaded)…"
if dbus-send --session --print-reply --dest=org.kde.KWin /Effects \
        org.kde.kwin.Effects.unloadEffect string:kittyglow >/dev/null 2>&1; then
    echo "      glow effect unloaded"
else
    echo "      glow effect not loaded (nothing to do)"
fi
if dbus-send --session --print-reply --dest=org.kde.KWin /Scripting \
        org.kde.kwin.Scripting.unloadScript string:kglowsync >/dev/null 2>&1; then
    echo "      kglowsync script unloaded"
else
    echo "      kglowsync script not loaded (nothing to do)"
fi
if dbus-send --session --print-reply --dest=org.kde.KWin /Scripting \
        org.kde.kwin.Scripting.unloadScript string:kitty-toggle-border >/dev/null 2>&1; then
    echo "      border script unloaded"
else
    echo "      border script not loaded (nothing to do)"
fi
# Reconfigure so KWin re-applies default decorations where possible.
dbus-send --session --print-reply --dest=org.kde.KWin /KWin \
    org.kde.KWin.reconfigure >/dev/null 2>&1 || true

echo "[2/4] strip auto-enable flags + plugin file + script dir…"
for KEY in kittyglowEnabled kglowsyncEnabled kitty-toggle-borderEnabled; do
    if command -v kwriteconfig5 >/dev/null 2>&1; then
        kwriteconfig5 --file "$KWINRC" --group Plugins --key "$KEY" --delete
    elif [ -f "$KWINRC" ]; then
        sed -i "/^${KEY}/d" "$KWINRC"
    fi
done
for SDIR in "$HOME/.local/share/kwin/scripts/kglowsync" \
            "$HOME/.local/share/kwin/scripts/kitty-toggle-border"; do
    if [ -d "$SDIR" ]; then
        rm -rf "$SDIR" && echo "      removed: $SDIR"
    fi
done
if sudo rm -f "$SO"; then
    echo "      removed: $SO"
else
    echo "      WARN: sudo failed — $SO left on disk but stays inert"
fi

echo "[3/4] restore window manager…"
case "${1:-}" in
    only)
        echo "      skipped (mode: only) — effect fully stripped"
        echo "      NOTE: windows the border script already stripped recover their"
        echo "      titlebar when their app reopens a window or on next KWin start;"
        echo "      run without arguments for the fully-decorated KWin now."
        ;;
    xfwm4)
        pkill -x kwin_x11 2>/dev/null || true
        ( setsid xfwm4 --replace >/dev/null 2>&1 & )
        echo "      xfwm4 now owns the desktop"
        ;;
    *)
        ( setsid kwin_x11 --replace >/tmp/kwin-relaunch.log 2>&1 & )
        echo "      KWin restarted — kittyglow CANNOT come back:"
        echo "      config flag gone + plugin file gone"
        echo "      (relaunch errors, if any, are in /tmp/kwin-relaunch.log)"
        ;;
esac
echo "Rollback complete."
