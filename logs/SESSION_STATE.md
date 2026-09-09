# SESSION_STATE.md

> **Note:** All documentation and code in this project are purely AI-generated.

**Timestamp:** 2026-09-09T~04:15Z (09:45 local +05:30)
**Agent/session:** pi (new session), continuing from dead session 01a077e8 (2M tokens).

## Current Objective
Shortcut recovery is DONE (dom0 level). Next: **Step C** — harden `toggleKittyBorderless()` against KWin's group-renaming regression, rebuild, deploy, docs.

## Discovered Facts (all verified via dom0 probes this session)
- **ROOT CAUSE (final):** boot race — kwin_x11 started 08:58:11 BEFORE kglobalaccel 08:58:13 → kwin's global-shortcut registration silently lost → Meta+Shift+B/T dead since Sep 08 reboot. Restarting kglobalaccel alone does NOT heal (kwin never re-registers). **kwin restart AFTER the daemon heals it.**
- **FIX APPLIED & E2E VERIFIED:** kwin_x11 --replace (PID 14836, HOME=/home/chenpan, kittyglow + org.kde.kwin.Effects DBus healthy). kwin registry now shows both shortcuts ACTIVE: "Toggle Kitty Borderless" key 301989954 (=0x12000042=Meta+Shift+B), "Window No Border" key 301989972 (=0x12000054=Meta+Shift+T). xdotool super+shift+b flipped noborder true→false→true — full daemon→grab→dispatch→effect chain works.
- **kwinrulesrc normalized** to exactly one active group `[kitty-borderless]` (noborder=true, rules=kitty-borderless) — the KWin numeric-rename regression ([1] active vs [kitty-borderless] inert) deduplicated manually. Stable across reconfigures so far.
- **Key-encoding facts (hard-won):** Qt::SHIFT=0x02000000, Qt::CTRL=0x04000000, Qt::ALT=0x08000000, **Qt::META=0x10000000**. Meta+Shift+B=0x12000042=301989954; Meta+Shift+T=0x12000054=301989972. My earlier probes used 0x060000xx (=Ctrl+Shift) — wrong by modifier bit.
- **xdotool gotchas:** `meta` modifier = Alt (Mod1); Super/Win = `super`. E2E test must use `xdotool key super+shift+b`.
- **KF5 fact:** `setForeignShortcut` silently no-ops for a component with no live registration... (actually: registers against key only — my "empty" reads were wrong-key artifacts; ownership checks must use correct ints or no-arg `allShortcutInfos`).
- Accidental foreign Ctrl+Shift+B/T registrations (created by wrong-key setForeignShortcut) were **cleared** (setForeignShortcut with empty int array) — net zero.
- dom0 helper (`/home/user/.local/bin/dom0`) runs qrexec to dom0 as user chenpan (uid 1000); needs hardcoded `XDG_RUNTIME_DIR=/run/user/1000 DBUS_SESSION_BUS_ADDRESS=unix:path=/run/user/1000/bus DISPLAY=:0` exports; one password dialog per invocation.
- Current toggle code (src/kittyglow.cpp:249): hardcodes KConfig group "kitty-borderless"; `if (!g.exists()) return;` → dead toggle whenever KWin renames the group (recurring LL-007 family). 220 ms repeat-gate already present.
- **Repo state:** kittyglow.cpp = 273 lines (src/kittyglow.h does NOT exist — class declared in glowshader.h? no: class is in kittyglow.cpp? verify: only kittyglow.cpp + glowshader.h/.cpp in src/; the effect class header is NOT separate — kittyglow.cpp contains its own class declaration). Git HEAD bc9ecc8, tree clean.

## File Changes
- No repo source changes this phase. dom0 config changes: kwinrulesrc normalized; kglobalshortcutsrc daemon-rewritten (net zero); kwin replaced.
- Ledger commits this session: d697c0f (v2→v3.3 arc), 0d9844b/716b711 (earlier logs), de8afbe/704c63b/e4abea9 (diagnostics), bc9ecc8 (recovery success logs).

## Decisions & Rationale
- Kept canonical group name `kitty-borderless` during normalization (matches installed binary's hardcoded group) so the OLD binary works immediately.
- Correct key ints established empirically from kwin's own registry ("Window to Desktop 9" = 0x14000079 = Meta|Ctrl|9 proved Qt::META=0x10000000).
- Step C direction: find-by-content (Description=="kitty borderless" OR wmclass==kitty), not find-by-name; toggle whatever ACTIVE group matches; fallback activates an inert kitty group by rewriting [General] rules=. Kills the regression class instead of patching instances.
- Rule 13 flag: kittyglow.cpp 273 lines > 200 → propose extracting rule-toggle into `kittyborderrule.cpp/.h` (single responsibility: kwinrulesrc rule management) + CMakeLists update.

## Active Blockers
- None.

## Pending Work (ordered)
1. **Step C** (needs user "implement changes"): extract + harden toggle logic into src/kittyborderrule.cpp/.h; update CMakeLists.txt; build (needs "build" consent); deploy to dom0 + restart kwin; E2E re-verify.
2. Docs: HANDBOOK.md (behavior unchanged, note recovery), PROJECT_CONTEXT.md (build state), SPECIFICATION.md (LL-011: Qt::META encoding; LL-012: kwin-before-kglobalaccel boot race; LL-013: xdotool meta≠super; LL-014: setForeignShortcut semantics), ROADMAP.md phase status; HTML regen via generate-docs-html.sh (Rule 18).
3. Git commit docs; final verification per Rule 16/19.

## Full Context Dump
- kwin PID 14836 (restarted ~09:14 local Sep 09). kglobalaccel PID 14616 (restarted 09:07:34). Both healthy.
- kwinrulesrc final: `[General] count=1 rules=kitty-borderless`; `[kitty-borderless] Description="kitty borderless" wmclass=kitty wmclassmatch=3 noborder=true`.
- kwin registry entries (allShortcutInfos): Toggle Kitty Borderless {active:[301989954], default:[301989954]}; Window No Border {active:[301989972], default:[0]}.
- Parity at rest: rules=kitty-borderless, noborder=true (kitty borderless).
- T-key E2E not simulated (native kwin shortcut, same healed chain; user should press physically).

## Next Agent Handoff Message
Read PROJECT_CONTEXT.md + this file. Working dir ~/Projects/QubesOS/UI-Enhancements/Kwin/kitty-glow. Shortcuts are FIXED in dom0 (verified). Next action: present Step C proposal (extract harden toggle into kittyborderrule.cpp/.h per SESSION_STATE "Pending Work" #1) and wait for explicit "implement changes". For any dom0 probe use the `dom0` helper with the env exports listed above; E2E keypress = `xdotool key super+shift+b`.
