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
OpenGL-compositing only, and four toggle shortcuts: **Meta+Shift+G** =
glow toggle for the FOCUSED window, **Meta+Shift+B** = border/titlebar
toggle for the FOCUSED window (both runtime-only, build #18), and the
global masters **Meta+Shift+Alt+G** / **Meta+Shift+Alt+B** (persisted,
sweep all eligible windows via the kglowsync script).

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

### 6b. Toggle shortcuts (v3.10, build #18 — per-window + global masters)

Four toggles exist. Launch default: glow ON, borders OFF (borderless) for
every eligible app window — per-window overrides always reset to this at
kwin restart (runtime-only, by design).

- **Meta+Shift+B — Toggle Window Borders (FOCUSED window).** Flips the
  focused window's border/titlebar only. The effect stages a window-op on
  the `nextWindowOp` DBus channel; the kglowsync script resolves its own
  `workspace.activeClient` (no window id crosses the bus), flips that
  window's `noBorder`, and records it in an overrides map (windowId →
  bool) so the 400 ms safety-net sweep PROTECTS the choice instead of
  clobbering it (LL-027). Runtime-only: dies with the window or kwin
  restart. E2E-verified live 2026-09-11 (journal: `focused-op: win=…
  (dev-general:kitty) noBorder true -> false`, sweep-safe soak).
- **Meta+Shift+G — Toggle Glow (FOCUSED window).** Flips the focused
  window's halo only, via a runtime override set in the effect
  (`glowfocus.h`, pruned on `windowDeleted`); the persisted global default
  is untouched. Journal names the scope: `toggle: glow (focused …) ->`.
  Note: with the focused window's halo occluded by maximized neighbours
  the visual change can be invisible even though the toggle fired —
  check the journal line.
- **Meta+Shift+Alt+B — Toggle Borders All Windows (GLOBAL master).** The
  build #16/#17 class-wide sweep: flips the persisted
  `~/.config/kittyglowrc` `[General] noBorder` and stages it via
  `KittyToggle::requestApply()`; the script sweeps **every eligible app
  window** live (60 ms poll + 400 ms safety net; 5 s bootstrap watchdog
  re-applies the default on every kwin start). Re-imposes the global
  default on all windows, resetting any per-window choices (documented
  reset semantics).
- **Meta+Shift+Alt+G — Toggle Glow All Windows (GLOBAL master).** Flips
  the persisted `[General] glowEnabled` and repaints fully — live, no
  restart, no flash — and clears every per-window glow override (master =
  everything back to the new default).
- **Meta+Shift+T — Window No Border (native KWin).** Per-focused-window
  titlebar/frame toggle, rebound from Meta+Shift+B when the effect needed B
  (kglobalshortcutsrc, applied via `plasma-kglobalaccel` unit restart — the
  unit name is `plasma-kglobalaccel.service`, not `kglobalaccel5`).

Common notes: eligibility mirrors the glow predicate (LL-026): scripting
type flags where present + chrome-class exclusion (plasma surfaces,
`Qui-*` ghosts, `xembedsniproxy`, `krunner`); size guard sub-48 px
(LL-026 supplement). Since the 2026-09-09 write-revert diagnosis the
persisted state lives in `kittyglowstate.cpp` — NOT in a kwinrulesrc
forcing rule (scripting < rules; the rule file must never carry
`noborderrule` again). Effect shortcuts register under the kglobalaccel
component **kwin** (`/component/kwin`), not their own component.
**Synthetic-key trap (LL-027):** after a kwin `--replace`, `xdotool key`
(XTEST) can fail to trigger a live, correctly-registered binding while
physical keys fire instantly — verify shortcuts on the real keyboard.
**Verification caveat:** `xdotool getwindowgeometry` reports the CLIENT
X window, and KWin X11 decorations wrap the client without resizing it —
frame-size probes can never see a titlebar appear/disappear. Verify B by
the kglowsync journal lines or visually.

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
LabelColor=true     # v3.11: per-VM Qubes label hue (false = configured gold)
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
It strips BOTH components the deploy installs — the kittyglow effect AND the
kglowsync live-sync script (unloaded, kwinrc flag deleted, script dir
removed); without the kglowsync strip new windows would keep spawning
borderless after a "clean" rollback (re-audit 2 M5).

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
| `GlowColor` / `GlowColorInactive` | 255,215,0 (gold) | halo RGB (fallback when `LabelColor=false`) |
| `GlowOpacity` / `GlowOpacityInactive` | 60 / 30 | peak opacity % (applied over the label hue too) |
| `LabelColor` (v3.11) | true | per-VM Qubes label hue (`_QUBES_LABEL_COLOR`); false = configured gold |
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
  physical press. Each toggle owns its gate — a B press followed by a G
  press inside 220 ms now toggles BOTH (audit M3 fix; the old shared gate
  silently dropped the second press).
- Toggle timing edges (documented, accepted): the focused-window border
  flip is resolved by the script ~60 ms after the keypress
  (`workspace.activeClient` at poll time) — an alt-tab inside that window
  flips the NEW focused window; and if the 60 ms poll ever stalled past
  two gated presses, the single command slot is last-wins (one flip
  instead of two). Both are improbable on the local bus and recover with
  one more press. A held key cannot churn either state (v3.9).
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
