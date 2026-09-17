<!-- HTML sibling: regenerate ONLY via bash ~/Projects/scripts/generate-docs-html.sh <this-file> — Rule 18c left-aligned CSS. Never hand-roll pandoc. -->
# LL-033 — Surface eligibility gap: menus, popups, notifications (2026-09-15)

> **Note:** All documentation and code in this project are purely AI-generated.

Diagnosis for the three user-reported artifacts after build #21 (v3.11.0):
(1) glow around expanded menu-bar drop-downs, (2) a flickering glow outline
left where a drop-down extends past its window's bottom border, (3)
notifications that sometimes carry a halo.

## Verdict

**Root cause is a single eligibility gap, not three bugs.** The report is
satisfied by the unmanaged-popup half of the hypothesis; the notification
half is a separate gap. One structural defect was found in the current
predicate that also explains the historical ghost-square workaround.

## Confirmed facts (pinned KWin 5.27.8 source, `/tmp/kwinsrc`)

- **`EffectWindow` has no `isUnmanaged()`.** The API is
  `isManaged()` — installed header `/usr/include/kwineffects.h:2588`:
  *"whether it's managed or override-redirect"*. Any proposed
  `isUnmanaged()` exclusion would not have compiled.
- **`isManaged()` is the durable flag.** `effects.cpp:2003`
  (`EffectWindowImpl` ctor) sets `managed = window->isClient()`, with a
  comment explaining the flag is captured *precisely* so effects can still
  distinguish managed from unmanaged windows once a `Deleted` parent
  replaces the original ("e.g. combo box popups, popup menus, etc").
- **Unmanaged windows DO reach effects.** `effects.cpp` connects
  `Workspace::unmanagedAdded` → `setupUnmanagedConnections`, wiring
  closed/opacity/frame-geometry/damaged/visibleGeometry signals, and
  `slotUnmanagedShown` asserts `qobject_cast<Unmanaged *>`. So this
  project's predicate really is consulted for popups.
- **Type predicates cannot see Qt menus.** `Window::isPopupWindow()`
  (`window.h:2340`) matches only `NET::ComboBox / DropdownMenu / PopupMenu
  / Tooltip`. Qt's `QMenu` popups are override-redirect, set no type atom,
  and inherit the parent's `WM_CLASS` (`konsole`) — so every type- and
  class-based exclusion in `glowtargets.h` misses them.
- **`isManaged()` is live-verifiable.** `xprop -root _NET_CLIENT_LIST`
  membership is the CLI equivalent of `window->isClient()`.

## Structure gaps in the current predicate (`src/glowtargets.h`)

Order of checks today: deleted → desktop/dock → dialog/notification/OSD →
splash/tooltip/popupWindow/utility → class (`plasmashell`, `qui-*`,
`xembedsniproxy`, `krunner`) → **size < 48 px**. Missing:

1. **No `isManaged()` check** — override-redirect surfaces are eligible
   whenever they clear the type/class/size filters. This is the direct
   cause of the menu artifacts.
2. **No `isOnCurrentDesktop()` / `isOnCurrentActivity()` check** — robust
   even if KWin's window list is per-desktop as in 5.27 (not confirmed on
   this box; the check is cheap and closes an unknown).
3. **The 48 px size guard is doing structural work it should not.** It is
   the *only* thing excluding the unmanaged `Qui-*` tray-source windows
   (16x16 icons, class `Qui-devices` etc., **confirmed unmanaged** in the
   live census). Those are excluded by `qui-` class today, so the guard
   appears to be covering override-redirect surfaces with any other class,
   or class-less ones. `isManaged()` makes the guard redundant as
   popup-chrome protection.
4. **Occluder transparency rule interacts with notifications/photographic
   overlays.** `occludedAbove()` skips non-dock windows with
   `opacity() < 0.99`, so a translucent surface stacked above a glowing
   window lets the halo bloom through it. Live check: the Plasma
   notification (`plasmashell`, 332x74, `_NET_WM_WINDOW_OPACITY` **unset**
   → effect-facing `opacity() == 1.0`, type NOTIFICATION, `skipTaskbar`/
   `skipPager`/`SkipSwitcher`, **managed**) is therefore *already* a
   correct occluder and is already class-excluded from glowing — so this
   notification is not the source of artifact (3).

## Evidence gaps (honest status)

Per-artifact on-screen A/B reproduction was **not** achieved. Automated
menu opening failed on this box: the target Konsole sits on a non-current
desktop (the display is at desktop index 9 of 10, with the Visible
`plasmashell` surfaces spanning all desktops) and every drive attempt
produced a full-screen desktop switch rather than a popup, yielding **0
new root-tree windows and 0 new client-list ids** for click, `Alt+F` and
`F10`. Earlier pixel probes were invalidated by a masking error: build #21
set `GlowColor=255,255,255` on dom0, so a gold-colored halo mask was
measuring wallpaper (the recurring 165x17, fill-0.995 cluster in both
"menu" and "notif" frames).

Still unproven, therefore: **which** class/geometry each of the three
artifacts belongs to. A human-in-the-loop capture is the remaining step
(see the plan), not more remote automation.

## Fix (implemented build #22, v3.12.0, sha 25169edd — deployed pending consent)

- `glowtargets.h`: `if (!w->isManaged()) return false;` as the FIRST denial,
  before type/class/size — kills override-redirect chrome (Qt menus,
  tooltips, popup windows — artifact 1 and the menu half of artifact 2).
- Artifact 3: not independently sealed; plasmashell notifications were
  already excluded and the VM-notification capture produced no surface.
  The `isManaged()` gate closes the unmanaged-notification case; re-check
  on-screen after deploy.
- Artifact 2: re-test after deploy; if flicker persists the remaining path
  is the damage/repaint hooks issuing ring repaints for a surface that is
  not itself drawn.

## Method notes

Probes: `logs/probe/ll033_surface_diag{,2..7}.py` (v1–v6 superseded; v1's
managed/hex comparison was invalid — `xdotool` emits decimal ids, `xprop`
hex). Logs: `logs/run/ll033*-diag-*.log`. Crop images:
`logs/probe/ll033/`.