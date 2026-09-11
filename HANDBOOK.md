<!-- HTML sibling: regenerate ONLY via bash ~/Projects/scripts/generate-docs-html.sh QubesOS/UI-Enhancements/Kwin/kitty-glow/HANDBOOK.md — Rule 18c left-aligned CSS. Never hand-roll pandoc. -->
# kitty-glow — Handbook

> **Note:** All documentation and code in this project are purely AI-generated.

## 1. Purpose & Features

A KWin effect that draws a soft yellow halo around every eligible window —
all normal application windows (build #15); dialogs, notifications, OSD,
popup menus, tooltips, splash and utility windows are excluded by
window-type predicates, and plasma surfaces, Qubes tray-widget ghosts,
xembedsniproxy and krunner are excluded by window class (LL-026 — qubes-gui
strips `_NET_WM_WINDOW_TYPE`). Features: automatic window tracking, 8-layer
alpha falloff, seamless pass-behind occlusion, fullscreen suppression,
OpenGL-compositing only, and two persisted toggles: **Meta+Shift+G** = glow
master switch, **Meta+Shift+B** = class-wide border (frame+titlebar) toggle
applied live by the kglowsync script.

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
This copies the `.so` to `/usr/lib64/qt5/plugins/kwin/effects/plugins/` (the
directory KWin 5.27 actually scans), the metadata to
`/usr/share/kwin/effects/kittyglow/metadata.json`, `chmod 644` both, and enables
the plugin in `~/.config/kwinrc` (`[Plugins] kittyglowEnabled=true`).

## 5. Activate (live, no compositor restart)

KWin 5.27 can load effect plugins at runtime over DBus — no restart needed
**for a first activation or for reconfigure tuning**:

```bash
# in dom0, as the desktop user
sudo -u chenpan env DBUS_SESSION_BUS_ADDRESS=unix:path=/run/user/1000/bus \
  XDG_RUNTIME_DIR=/run/user/1000 \
  dbus-send --session --print-reply --dest=org.kde.KWin /Effects \
  org.kde.kwin.Effects.loadEffect string:kittyglow   # expect: boolean true
```

> **LL-021 (2026-09-10):** a live unload/load does NOT pick up a REBUILT
> `.so` — KWin keeps the library mapped and re-instantiates the old code
> while the new file sits on disk. After every `build.sh` + `deploy.sh`, a
> full `kwin_x11 --replace` is required. Verify behavior (journal/screen),
> never the DBus boolean alone.
>
> **LL-022 (2026-09-10):** `deploy.sh` runs three dom0 bridge calls, each
> gated by the password dialog; a cancelled dialog is swallowed and the
> script still exits 0 with the PREVIOUS build left on dom0. Always
> sha-verify after deploy:
>
> ```bash
> sha256sum dist/kittyglow.so | cut -c1-16
> dom0 'sha256sum /usr/lib64/qt5/plugins/kwin/effects/plugins/kittyglow.so | cut -c1-16'
> ```

Verify with `isEffectLoaded string:kittyglow` on the same path. A full
`org.kde.KWin.reconfigure` (object `/KWin`) also picks the plugin up from
`kwinrc` — and additionally hot-reloads window rules from `kwinrulesrc`
(`Workspace::slotReconfigure` → `RuleBook::load()`).

## 6. kitty borderless rule

Registered in dom0 `~/.config/kwinrulesrc`:

```ini
[kitty-borderless]
Description=kitty borderless
noborder=true
noborderrule=2          # 2 = Force (rules.h SetRule enum)
wmclass=kitty
wmclassmatch=3          # 3 = RegExpMatch
```

plus `rules=kitty-borderless` under `[General]` (a rule group is ignored unless
listed there). Applied to kitty windows at (re)mapping after a reconfigure.

Note: no kitty binary exists in dom0 — the rule matches kitty windows launched
in AppVMs, which appear in dom0 (Qubes window integration) with wmclass
`kitty`.

### 6b. Borderless shortcuts (v3)

Two independent toggles exist:

- **Meta+Shift+B — Toggle Window Borders (effect-registered, class-wide).**
  Registered by the effect itself via `KGlobalAccel::setShortcut` (objectName
  kept as the historical `Toggle Kitty Borderless` id for kglobalaccel
  stability). Flips the persistent borderless state in
  `~/.config/kittyglowrc` (`[General] noBorder`) and stages the new value via
  `KittyToggle::requestApply()`; the kglowsync KWin script consumes it on its
  60 ms poll and applies `noBorder` live to **every eligible app window**
  (build #16: generalized from kitty-only by user directive 2026-09-11) —
  no reconfigure, no restart. Eligibility mirrors the glow predicate
  (LL-026): scripting type flags where present + chrome-class exclusion
  (plasma surfaces, `Qui-*` ghosts, `xembedsniproxy`, `krunner`). State
  survives reboots via the script bootstrap. Since the 2026-09-09
  write-revert diagnosis the state lives in `kittyglowstate.cpp` — NOT in a
  kwinrulesrc forcing rule: a loaded rule overrides KWin scripting
  `noBorder` writes (scripting < rules), so the rule file must never carry
  `noborderrule` for kitty again. The old content-based rule toggle
  (`kittyborderrule.cpp`) was retired. E2E-verified 2026-09-11 (build #17):
  sweep wrote `noBorder` to all 14 eligible app windows in BOTH directions
  via `requestApply` → `nextSource consumed`; state bootstrap re-applies it
  on every kwin start (5 s watchdog re-arm, LL-026 supplement).
  **Verification caveat:** `xdotool getwindowgeometry` reports the CLIENT
  X window, and KWin X11 decorations wrap the client without resizing it —
  frame-size probes can never see a titlebar appear/disappear. Verify B by
  the kglowsync journal sweep lines or visually.
- **Meta+Shift+G — Toggle Glow (effect-registered, build #16).** Glow master
  switch: flips `kittyglowrc` `[General] glowEnabled` and repaints fully —
  live, no restart, no flash. Moved off Meta+Shift+B by user directive
  2026-09-11 (B carried the glow switch only in build #15).
- **Meta+Shift+T — Window No Border (native KWin).** Per-focused-window
  titlebar/frame toggle, rebound from Meta+Shift+B when the effect needed B
  (kglobalshortcutsrc, applied via `plasma-kglobalaccel` unit restart — the
  unit name is `plasma-kglobalaccel.service`, not `kglobalaccel5`).

### 6c. Live-reloadable glow config (v3)

All visual parameters live in dom0 `~/.config/kwinrc` under
`[Effect-kittyglow]` and reload on `reconfigureEffect string:kittyglow` (or a
KWin reconfigure) — no rebuild needed:

```ini
[Effect-kittyglow]
Enabled=true
GlowRadius=32        # default thickness for all sides (px)
GlowLeft=32          # per-side overrides
GlowTop=32
GlowRight=32
GlowBottom=32
GlowCornerRadius=8   # 0..64
GlowColor=255,215,0     # active window halo color (RGB)
GlowColorInactive=255,215,0
GlowOpacity=60       # 0..100, active
GlowOpacityInactive=30
```

```bash
# apply after editing (no rebuild, no restart):
dom0 "sudo -u chenpan env DBUS_SESSION_BUS_ADDRESS=unix:path=/run/user/1000/bus \
  XDG_RUNTIME_DIR=/run/user/1000 dbus-send --session --print-reply \
  --dest=org.kde.KWin /Effects org.kde.kwin.Effects.reconfigureEffect \
  string:kittyglow"
```

## 7. Rollback

`kittyglow-rollback` is installed in dom0 at `/usr/local/bin` (hard copy per
workspace Rule 21). Modes: default (strip artifacts, restore clean KWin),
`xfwm4` (switch to xfwm4 instead), `only` (strip artifacts, leave WM alone).

## 8. Verify

- KWin alive: `pgrep -a kwin_x11` in dom0.
- No load error: `journalctl _COMM=kwin_x11 -n 50 | grep -i kittyglow`.
- Visual: open a kitty window → yellow halo; fullscreen kitty → no halo.

## 9. Tune (no rebuild — kwinrc `[Effect-kittyglow]`, see §6c)

| Knob | Default | Meaning |
|------|---------|---------|
| `GlowRadius` | 32 | halo thickness on all sides (px) |
| `GlowLeft/Top/Right/Bottom` | = GlowRadius | per-side thickness overrides |
| `GlowCornerRadius` | 8 | corner rounding (0–64 px) |
| `GlowColor` / `GlowColorInactive` | 255,215,0 (gold) | halo RGB |
| `GlowOpacity` / `GlowOpacityInactive` | 60 / 30 | peak opacity % |
| `Enabled` | true | master switch |

Edit kwinrc in dom0, then apply live with the `reconfigureEffect` command in
§6c — the SDF shader rebuilds from config, no recompile.

## 10. Disable / Uninstall

```bash
dom0 "kwriteconfig5 --file kwinrc --group Plugins --key kittyglowEnabled false"
# restart KWin, then optionally remove the two installed files.
```

## 11. Limitations

- Journal noise: `Could not initialize scripted effect: "kittyglow"` on
  every kwin start/reconfigure is BENIGN (LL-024) — the scripted loader
  probes the metadata-only effect package before the plugin loader loads
  the C++ .so. Never a sign of breakage; verify by behavior instead.
- The Plasma notification toast cannot be probed directly (unmanaged
  window): exclusion is verified by construction (type predicate, LL-023)
  plus observation that the toast's dock-type strip gets no halo (band 0).
- Effect is always-on for app windows (the Meta+Shift+G toggle flips the
  glow class-wide, not per-window — see ROADMAP).
- Minimum frame-size guard (build #17): windows smaller than 48 px in
  either dimension are never haloed and never borderless-toggled — this is
  the only reliable exclusion for UNMANAGED windows (Qui-* tray sources,
  override-redirect, no WM_CLASS: class checks cannot see them; the
  "ghost square" at the screen corner was a 16×16 Qui source). Mirrored
  in kglowsync's `isBorderlessTarget` so the script and effect agree.
- Glow visibility depends on stacking: with app windows maximized or
  stacked over each other, halos are offscreen or occluded — Meta+Shift+G
  toggling changes almost nothing VISIBLE while the desktop is fully
  stacked. Minimize/resize windows to see rings (occlusion rules, LL-019).
- v3.4 renderer: one SDF quad per kitty window, mapped through the scene's
  animation transform (scale about frame top-left + translation, the same
  affine model stock BlurEffect uses), so the glow tracks minimize/restore
  mid-flight and fades with `data.opacity()`; non-uniform animation scale
  slightly distorts corner radius for the animation's duration (<300 ms).
- Occlusion clipping (rebuilt every halo paint since v3.4): windows logically
  stacked above the PAINTED kitty (per `stackingOrder()`, same
  desktop/activity, not minimized) are subtracted from the halo region —
  docks/panels at `expandedGeometry()` (frame + shadow, LL-018), normal
  windows at `frameGeometry()` ONLY (build #14): the halo paints BENEATH
  them, so each window's own shadow gradient dims it progressively right up
  to the border — a seamless pass-behind with no wallpaper gap (user-accepted
  2026-09-10). Fully transparent terminal bodies show the ring through by
  design (only chrome/text occlude). The occluder set is recomputed on every
  halo paint (no cache), and stacking/activation changes
  (`stackingOrderChanged`, `windowActivated`) force a full halo repaint — a
  window raised above an unfocused kitty clips correctly on the very next
  frame (LL-019). Opaque windows and docks/panels (even translucent ones,
  e.g. an adaptive plasma panel) always clip; other genuinely translucent
  windows are skipped, letting the halo show through them by design (LL-018).
- Meta+Shift+B and G are autorepeat-gated (220 ms): one state flip per
  physical key press; a held key cannot churn either state (v3.9).
- Meta+Shift+T and B are also affected by kglobalaccel state: if the daemon
  ever deactivates a shortcut it persists an empty active field in
  `kglobalshortcutsrc` (T's line reads `Meta+Shift+T,,…`). Symptom: shortcut
  registered but dead. Remedy: restore the second field to
  `Meta+Shift+T,Meta+Shift+T,` and restart `plasma-kglobalaccel.service`,
  then `kwin_x11 --replace`.
- Live kwin hot-swaps (`loadEffect` while running) have crashed once during
  rollout — prefer enabling via kwinrc + reconfigure or `kwin_x11 --replace`
  rather than hot-loading repeatedly (verified clean restart path).
- KWin 5.27 has no border-only vs titlebar-only state: `noborder` is
  frame+titlebar unified, so B and T both toggle both together by design.
- The build container (`dom0-replica-fed37`) mounts this project's `src/` at
  `/src` (re-pointed 2026-09-06T23:50:19Z); `build.sh` builds straight from it.
