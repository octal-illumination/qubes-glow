# Ghost Square at Top-Left (0,0): Qubes Tray-Widget Source Windows Getting Halos

> **Note:** All documentation and code in this project are purely AI-generated.

**Date:** 2026-09-11 · **Session:** build #15 verification follow-up · **Status:** finding recorded; fix proposed, implementation pending user approval

## The Report

User (2026-09-11): "There is a small empty square with glow on the top right corner of the screen that doesn't seem to represent anything, why is it there? Figure out."

## Evidence (pixel-level, probe `logs/probe/ll026_ghost_hunt.py` / `ll026_ghost_hunt2.py`)

Screen-wide strict-gold mask found exactly 5 components; the relevant one:

| Gold component | Verdict |
|---|---|
| **452 px ring, bbox (0,0)–(40,44)** | **The ghost square.** A halo ring around nothing — it is the 22 px-margin halo of the four stale `Qui-updates`/`Qui-domains`/`Qui-clipboard`/`Qui-disk-space` **16×16 windows pinned at (0,0)**. They're the VM-side source windows of the Qubes tray widgets: qubes-gui keeps them mapped at (0,0) while their visible XEMBED copies live in the panel. They paint no content, have no `_NET_WM_WINDOW_TYPE` (proxy strips it) → pass `isGlowWindow()` → the effect draws halos for empty ghosts. Visible only in the exposed corner strip above the maximized konsoles. |

Other components (not the square): a 12×12 arc at (1036,6) = fragment of the
legitimate halo around the real `Qube Manager` app window (581,0, 453×767,
mostly occluded); 16×16 / 12×14 / 14×9 blobs = yellow content *inside* windows
(VM-state icons in Qube Manager, terminal text).

## Root Cause Chain

1. Qubes GUI proxy does **not replicate `_NET_WM_WINDOW_TYPE`** to dom0 for
   VM-proxied windows — verified: konsole, kitty, firefox, `Qui-*` all report
   "not found". Dom0-native apps (kdialog, plasmashell) DO set it.
2. KWin treats type-less windows as **normal** windows.
3. `KittyGlowTargets::isGlowWindow()` excludes by type predicates
   (`isDialog/isNotification/isPopupWindow/…`) → **they never match
   VM-proxied windows**. They only worked for dom0-native windows (which is
   why the dom0 `kdialog` exclusion test passed while real chrome doesn't).
4. The `Qui-*` ghosts (mapped, 16×16, contentless at (0,0)) and the panel
   tray icon `Qui-devices` (XEMBED at 1022,757) therefore get halos.

The only window property that survives the proxy reliably is **WM_CLASS**
(e.g. `Qui-updates`, `Dev-General:konsole`).

## Proposed Fix (approved plan pending "implement changes")

Class-based chrome exclusion in `src/glowtargets.h` (mirrored in
`src/kwin-script/kglowsync/contents/code/main.js`): exclude windows whose
class matches `plasmashell`, `Qui-*` (case-insensitive), `xembedsniproxy`,
`krunner` — alongside the existing type predicates (which keep working for
dom0-native windows). This removes the ghost ring, tray-icon halos, and
plasma start-menu/submenu halos in one mechanism.

## Open Questions / Follow-ups for the Later Investigation Task

- User reported "top **right**" corner; the ring measures at top-**left**
  (0,0). Confirm which square the user means; if a top-right one exists
  independently, hunt it with `ll026_ghost_hunt.py` while it is visible.
- Whether the `Qube Manager` halo should stay (real app window — currently
  stays) or be excluded.
- Whether VM-internal dialogs (type hints stripped in proxy) should be
  excluded by some non-type signal (e.g. WM_TRANSIENT_FOR replication
  behaviour of qubes-gui — unverified).
- xdotool `search --class .` misses classless windows; `xwininfo` is not
  installed on dom0. `qdbus org.kde.KWin /KWin supportInformation` returned
  empty output on this build (0 blocks) — find a working authoritative
  inventory method (e.g. `qdbus org.kde.KWin /KWin org.kde.KWin.supportInformation`
  explicit interface form, or KWin debug console export) for future hunts.

## Resolution (build #17, 2026-09-11)

- Root cause confirmed: the corner ghost is a 16x16 UNMANAGED Qui-*
  tray-source window (override-redirect, KWin `clientList` excludes it, no
  WM_CLASS readable) — class exclusion alone could never remove it.
- Fix shipped in build #17 (`d66b9009…`): minimum frame-size guard
  (sub-48 px = icon, never haloed) in `glowtargets.h` + mirrored
  `isBorderlessTarget` in kglowsync; plus a bootstrap watchdog (5 s re-arm
  until first service reply — build #16's script wedged when the request
  landed on the dying kwin during `--replace`, reply silently LOST).
- Verified live on dom0 (kwin 60603): corner gold 456 → 0 (ghost gone),
  tray icons clean, B toggle swept noBorder across all 14 eligible app
  windows in both directions, G toggle fires, bootstrap applies
  borderless default at startup.
- Testing lesson: xdotool client geometry does not reflect KWin X11
  titlebar changes (decorations wrap the client without resizing it);
  B is verified via the sweep journal lines / visual check instead.
