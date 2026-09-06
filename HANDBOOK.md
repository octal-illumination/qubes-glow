<!-- HTML sibling: regenerate ONLY via bash ~/Projects/scripts/generate-docs-html.sh QubesOS/UI-Enhancements/Kwin/kitty-glow/HANDBOOK.md — Rule 18c left-aligned CSS. Never hand-roll pandoc. -->
# kitty-glow — Handbook

> **Note:** All documentation and code in this project are purely AI-generated.

## 1. Purpose & Features

A KWin effect that draws a soft yellow halo around every `kitty` terminal window.
Features: automatic tracking of kitty windows, 8-layer alpha falloff, fullscreen
suppression, OpenGL-compositing only.

## 2. Prerequisites

- A running `dom0-replica-fed37` Podman container (Fedora 37, KWin 5.27.8 devel).
  Create/recreate it with `container/setup-build-container.sh` if absent.
- `dom0` helper on the Dev-General host (`~/.local/bin/dom0`).
- `qvm-run` access from dom0 back to Dev-General (for the file transfer).

## 3. Build

```bash
scripts/build.sh
# → dist/kittyglow.so  (sha recorded in logs/CHANGELOG.md)
# → dist/kittyglow.json
```

## 4. Deploy into dom0

```bash
scripts/deploy.sh
```
This copies the `.so` to `/usr/lib64/qt5/plugins/kwin/effects/`, the metadata to
`/usr/share/kwin/effects/kittyglow/metadata.json`, `chmod 644` both, and enables
the plugin in `~/.config/kwinrc` (`[Plugins] kittyglowEnabled=true`).

## 5. Activate (requires explicit approval — restarts the compositor)

KWin only reads the plugin list at startup, so a restart is required:

```bash
# in dom0, reuse KWin's own X env so the replacement connects to :0
eval $(cat /proc/$(pgrep -x kwin_x11)/environ | tr '\0' '\n' \
       | grep -E 'DISPLAY|XAUTHORITY|PATH|XDG' | sed 's/^/export /')
setsid nohup kwin_x11 --replace >/tmp/kwin-restart.log 2>&1 &
```

## 6. Verify

- KWin alive: `pgrep -a kwin_x11` in dom0.
- No load error: `journalctl _COMM=kwin_x11 -n 50 | grep -i kittyglow`.
- Visual: open a kitty window → yellow halo; fullscreen kitty → no halo.

## 7. Tune (src/kittyglow.cpp)

| Knob | Default | Meaning |
|------|---------|---------|
| `margin` | 22 | halo thickness (px) |
| `layers` | 8 | stacked rects (higher = softer) |
| `QColor(255,221,0,alpha)` | yellow | halo color |
| `alpha = 70 * t` | 0..70 | peak opacity |

Edit, then `scripts/build.sh && scripts/deploy.sh`, and restart KWin.

## 8. Disable / Uninstall

```bash
dom0 "kwriteconfig5 --file kwinrc --group Plugins --key kittyglowEnabled false"
# restart KWin, then optionally remove the two installed files.
```

## 9. Limitations

- Effect is always-on for kitty windows (no per-window toggle yet — see ROADMAP).
- Coordinate space assumes KWin's standard screen-space GL projection; if the halo
  renders offset, the projection handling in `paintQuad` needs adjustment.
- The live build container currently mounts the legacy `/home/user/kitty-glow`;
  first `build.sh` run re-points it to this project's `src/` automatically.
