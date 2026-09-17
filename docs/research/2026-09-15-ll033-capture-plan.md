<!-- HTML sibling: regenerate ONLY via bash ~/Projects/scripts/generate-docs-html.sh <this-file> — Rule 18c left-aligned CSS. Never hand-roll pandoc. -->
# LL-033 capture plan — human-in-the-loop (2026-09-15)

> **Note:** All documentation and code in this project are purely AI-generated.

> **Archived (2026-09-15, post-rename):** written while the project directory
> was still `kitty-glow`. The `isManaged()` fix this capture informed shipped
> in build #22 and was user-accepted. If ever re-run, substitute
> `Kwin/qubes-glow` for `Kwin/kitty-glow` in the commands below.

Purpose: identify the class, geometry and managed-state of each surface
behind the three reported artifacts, so the `glowtargets.h` fix can be
written from evidence instead of hypothesis. Remote automation was tried
and rejected (probe v7): the target Konsole lives on a non-current desktop
and every synthetic drive switches desktops instead of opening the menu.

## Precondition (one-time, from the desktop)

Bring the **dom0-native Konsole** to the *current* desktop and keep it
visible, so the popup opens where the capture can see it:

```bash
# in dom0, as chenpan
wmctrl -l                 # list windows + their desktops
wmctrl -s 9               # or whichever desktop the target konsole is on
```

## Command (from Dev-General, project root)

```bash
cd ~/Projects/QubesOS/UI-Enhancements/Kwin/kitty-glow

dom0 "bash -lc 'python3 /tmp/ll033cap.py'"
```

with `/tmp/ll033cap.py` pushed first (same base64 bridge as the probes):

```bash
dom0 "bash -lc 'qvm-run -p Dev-General base64 -w0 \
  /home/user/Projects/QubesOS/UI-Enhancements/Kwin/kitty-glow/logs/probe/ll033_capture.py \
  | base64 -d > /tmp/ll033cap.py'"
```

## What the operator does

1. When it prints `>>> 1. OPEN the Konsole menu-bar drop-down`, click the
   **File** menu and leave it open.
2. Watch the countdown; it re-censuses every 300 ms for 12 s and prints the
   moment a new window or client-list entry appears, plus
   `Override Redirect State` and managed status.
3. It then auto-prints an ASCII downsampling of the popup region so the
   outline shape is readable in text (no image viewing required).
4. Repeat for phase 2 when prompted, triggering the notification that
   shows the glow (for example a VM notification or `notify-send`).

Escape closes the menu; the probe never writes config, never kills a
process, and never touches the compositor.

## Decision table (what the capture decides)

| Capture result | Artifact | Fix |
| --- | --- | --- |
| New surface is **unmanaged** (absent from `_NET_CLIENT_LIST`) | 1, 2 | `if (!w->isManaged()) return false;` in `glowtargets.h` |
| New surface is **managed**, class = parent app (`konsole`) | 1, 2 | second predicate needed — report class + geometry; likely Qubes-prop or transient-based |
| Border-only bright ring around the popup rect | 1, 2 | confirms the popup itself is haloed (not the parent's ring clipped) |
| Notification is **unmanaged** | 3 | same `isManaged()` fix |
| Notification is **managed** with a normal app class and `_QUBES_VMWINID` | 3 | add a Qubes-proxied-notification predicate |
| Notification is **managed**, `opacity() < 0.99` | 3 | occluder rule: treat notification-typed surfaces as occluders regardless of translucency |

## After the capture

1. Record findings in
   `docs/research/2026-09-15-ll033-surface-eligibility.md` (Evidence gaps
   section), replacing the unproven entries.
2. Propose the `glowtargets.h` edit (Rule 1a) with the capture output
   attached as justification.
3. On approval: edit, add the regression assertion, rebuild, deploy,
   restart with consent, then re-verify the three artifacts on screen.