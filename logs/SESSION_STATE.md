# SESSION_STATE.md

> **Note:** All documentation and code in this project are purely AI-generated.

**Timestamp:** 2026-09-09T~03:5xZ (converted session; local +05:30 morning Sep 09)
**Agent/session:** pi (this new session), continuing from dead session 01a077e8 (2M tokens) — reconstruction via SESSION_STATE.md + Action-History.md + session JSONL.

## Current Objective
Recover and complete kitty-glow v3.3+ work: diagnose why **Meta+Shift+B** (Toggle Kitty Borderless) and **Meta+Shift+T** (Window No Border) don't work after the user's Sep 08 reboot, then fix.

## Discovered Facts (all verified via dom0 probes this session)
- Machine rebooted **Sep 08 08:58 local**; kwin_x11 PID 5679, kglobalaccel restarted clean (no journal errors).
- Today is Sep 09; kwin has been up since Sep 08 boot. kittyglow **loads successfully**: journal "Successfully loaded plugin effect: kittyglow" at boot + reloads 09:23:39/44/47 (Sep 08), 08:31:33/35 (Sep 09).
- On-disk kglobalshortcutsrc is CORRECT: `[kwin]` group has `Toggle Kitty Borderless=Meta+Shift+B,Meta+Shift+B,...` and `Window No Border=Meta+Shift+T,,...` (single lines, deduped last session).
- **ROOT CAUSE (B key): kwinrulesrc regression.** Current file:
  - `[1]` — Description="kitty borderless", **noborder=true**, wmclass=kitty, wmclassmatch=3 — **ACTIVE** (`[General] count=1, rules=1`)
  - `[kitty-borderless]` — same content but **noborder=false** — INERT (not in rules=)
  - The effect's `toggleKittyBorderless()` (src/kittyglow.cpp) hardcodes KConfig group `"kitty-borderless"` → toggles the INERT group. B fires (proven: inert group flipped true→false since last session's repair) but nothing visible changes → "B doesn't work".
  - Mechanism: KWin re-saves the rulebook with numeric group names (`[1]`) whenever it rewrites kwinrulesrc — recurring pattern (LL-007 family; dedup fix at 07:08 Sep 7 was undone by KWin's own re-save).
- **org.kde.kwin.Effects bus service anomaly**: batch2 method-call answered "Cannot find '.isEffectLoaded'" (service alive, method-name resolution quirk) but batch3 introspect/loadedEffects said "Service does not exist" — flaky or interface-name issue; unresolved, not believed to be the shortcut blocker (effect loads via kwinrc [Plugins] regardless). Earlier session used full-name `org.kde.kwin.Effects.isEffectLoaded` successfully.
- kglobalaccel DBus (org.kde.KGlobalAccel at /kglobalaccel) is ALIVE; `allComponents` includes /component/kwin but NOT a separate kittyglow component (effect registers under kwin component, as designed).
- **T key live-state UNKNOWN** — the busctl getGlobalShortcutsByKey/shortcut queries (B=100663362=0x06000042, T=100663380=0x06000054) were CANCELLED at the dom0 password dialog.
- Post-reboot kwinrulesrc noborder state = kitty currently HAS its border (active group [1] noborder=true → borderless ON... wait: noborder=true means NO border → kitty IS borderless now).
- Phase 1 of this session (approved: "implement changes" via git plan) is DONE: commit d697c0f ("v2→v3.3" 24 files +1533/−197), logging commits 0d9844b, 716b711. Working tree was clean before diagnostic probes.

## Decisions & Rationale
- Reconstructed dead session instead of re-deriving: read its SESSION_STATE.md, Action-History.md, session JSONL tail.
- Diagnosis-first: no edits until root cause confirmed; all dom0 probes read-only.
- Planned fix direction for B: make `toggleKittyBorderless()` robust — read `[General] rules=`, find the ACTIVE group by Description/wmclass match, toggle THAT group (handle arbitrary group names incl. `[1]`), normalize/sync inert duplicates. Optionally also create-if-missing instead of early-return. NOT yet implemented — needs user approval per Rule 1.
- T key likely needs only live-state confirmation; if live daemon has T bound and native toggle still dead, next suspect is X11/grab level.

## Active Blockers
- Each dom0 probe requires user password dialog; last batch cancelled (~09:47 local).

## Pending Work
1. Re-run the 2 cancelled busctl queries (one dom0 password prompt): live owner of Meta+Shift+B/T keys + action→key maps. Confirm T registration and B's live registration under /component/kwin.
2. Propose + implement fix in src/kittyglow.cpp (robust kwinrulesrc group lookup) after user says "implement changes".
3. One-time data normalization of dom0 kwinrulesrc (merge [1]/[kitty-borderless], restore noborder=true) — needs approval.
4. Rebuild + reinstall KCM/effect via dom0 (build consent required: "build the app").
5. Update HANDBOOK/PROJECT_CONTEXT/SPECIFICATION (LL entry for KWin-numeric-group-rename), regenerate HTML siblings, commit, regression-checks.sh.

## Next Agent Handoff Message
Read this file + PROJECT_CONTEXT.md. Ask user to run (approve) the pending dom0 busctl probe to confirm T's live binding. Then propose the toggleKittyBorderless robust-group fix (never recalculating: effect must toggle the group listed in [General] rules=, whatever its name). Do not build without "build the app".
