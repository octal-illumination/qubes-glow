# Session State — kitty-glow (2026-09-09, deployed + E2E verified)

**Objective:** COMPLETE — write-revert root-cause fix shipped, deployed, and E2E-verified on dom0.

## Discovered Facts
- Root cause (live-proven): kwinrulesrc forcing rule > KWin scripting `noBorder` writes. Rule deleted → writes stick, zero reverts.
- kglowsync new code running on dom0 (PID 28473 kwin_x11): "bootstrap: persisted state=true -> desired=true" at 14:20:31.
- Script auto-runs after unloadScript when kglowsyncEnabled=true + reconfigure/start path (observed empirically: unload at 14:20:30 → alive+bootstrap at 14:20:31).
- A dormant loadScript object (id 1, never started) may sit on /Scripting — harmless (no timers).
- busctl on dom0 defaults to SYSTEM bus — must pass --user for session-bus calls (dbus-send defaults to session). This caused a false "kglobalaccel dead" scare.
- kglobalaccel healthy (PID 14616), "Toggle Kitty Borderless" registered, restarted once as no-op.
- E2E 2026-09-09 14:31: 4 presses → kittyglowrc false/true/false/true, ONE sweep write per press, every write stuck (now= matches), 3 s post-final silence. Final state: borderless.
- kittyglowrc created on first toggle: [General] noBorder=true at rest.

## File Changes (this work cycle)
- src/kittyglowstate.{h,cpp} NEW; kittyborderrule.{h,cpp} REMOVED; kittyglow/kittytoggle rewired; main.js rewritten; CMake updated.
- dom0: kwinrulesrc group [1] deleted (owner/mode preserved); kittyglow.so+metadata+kglowsync pkg installed; kwinrc Plugins enabled.
- Committed bb79411 (rebuild) + e6d4d7c (session state) + final commit this turn (docs/logs).
- logs/build.log stale Rule-2 monitoring symlink REMOVED.

## Decisions & Rationale
- kittyglowrc over kwinrulesrc: LL-016 (rules override scripting).
- Effect loaded BEFORE script reload: bootstrap needs effect's getCurrentState service.

## Active Blockers
- None.

## Pending Work (optional)
- Hardening idea (proposed, not approved): script should retry getCurrentState when the effect's heartbeat appears after a failed bootstrap (covers the script-loads-before-effect startup race).
- Rule 19/20 regression checks are open-dom0 project assets — do NOT run on this project (Rule 21 isolation).

## Next Agent Handoff Message
Fresh agent: read logs/SESSION_STATE.md + PROJECT_CONTEXT.md here. The toggle
feature is deployed and verified; no dom0 action pending. If the user reports
an issue, first grep journal: `journalctl -b | grep "kglowsync(js):"`.
