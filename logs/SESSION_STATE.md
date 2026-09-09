# Session State — kitty-glow (2026-09-09, Step C rebuild committed)

**Objective:** rebuild the kitty borderless state layer offline (kittyglowrc state store + kglowsync bootstrap rewrite); deploy + E2E still pending user go.

## Current Objective
Ship the write-revert root-cause fix: persistent state in `~/.config/kittyglowrc` (`[General] noBorder`), applied live by the kglowsync KWin script; kwinrulesrc must never carry `noborderrule` for kitty again.

## Discovered Facts
- Root cause (live-proven 2026-09-09): a loaded forcing rule in kwinrulesrc overrides KWin scripting `noBorder` writes (scripting < rules). Removing the rule → script writes stick (parity went 3→5).
- kglowsync script was verified LOADED in kwin (loadScript confirmed; heartbeat pauses after 2 s without effect replies).
- `print()` is dropped by journald on this kwin build; script diagnostics go through the effect's `scriptLog` DBus slot (one-way).
- Parentless `new QTimer()` works on this build; parented construction throws "Could not convert argument 0".
- `QStringLiteral` needs a raw string literal (token-pasting) — not `constexpr` names.
- Container dom0-replica-fed37 (Fedora 37 / KWin 5.27.8) mounts project src at /src; build logs land in container at /tmp/b_cmake.log, /tmp/b_make.log.
- Build artifacts: dist/kittyglow.so sha256 25e019db…, kittyglow.json 3a3f66bc….
- kglobalaccel restart did NOT help registrations (kglobalaccel stores ints, no string keys).
- dom0 password dialog flow for DBus works (kglobalaccel + org.kde.kwin.Effects unloadScript verified live).

## File Changes
- src/kittyglowstate.{h,cpp} — NEW state store (kittyglowrc [General] noBorder; load/save/toggle).
- src/kittyborderrule.{h,cpp} — DELETED (git rm; superseded).
- src/kittyglow.cpp — toggle body now persists via KittyGlowState, no rule edit, no reconfigure.
- src/kittytoggle.{h,cpp} — nextSource + NEW getCurrentState DBus slot reading kittyglowrc (kglowsync bootstrap consumer).
- src/kwin-script/kglowsync/contents/code/main.js — REWRITTEN: getCurrentState bootstrap, 60 ms nextSource poll, 400 ms sweep (pauses after 2 s without effect heartbeat), clientAdded coverage, scriptLog diagnostics.
- src/CMakeLists.txt — sources updated.
- HANDBOOK/PROJECT_CONTEXT/ROADMAP/SPECIFICATION + HTML siblings synced.
- Committed bb79411 (tree clean).

## Decisions & Rationale
- kittyglowrc over kwinrulesrc: rules override scripting — any rule-based state re-creates the write-revert fight (LL-016).
- getCurrentState bootstrap: kwin restarts reload the script; bootstrap restores persisted state and applies to pre-existing windows (covers kwin restarts too).
- 400 ms sweep pauses without effect heartbeat: plugin unloaded → script stops touching windows (no churn, kitty keeps last state).
- Rejected: rule-toggle with self-heal (previous Step C) — it WAS the root cause of the fight.

## Active Blockers
- None for offline work. Deploy + E2E require explicit user go (dom0 touch).

## Pending Work (ordered)
1. Deploy (needs "deploy"): dist/ → Dev-General:~/.local/share/kwin/effects/kittyglow + script to ~/.local/share/kwin/scripts/kglowsync.
2. Delete kitty forcing rule from dom0 kwinrulesrc (needs explicit go — desktop-affecting edit).
3. Restart kglowsync script + effect; verify bootstrap log line via scriptLog→journalctl.
4. E2E toggle test: Meta+Shift+B ×3, parity, journal scriptLog lines, kittyglowrc value flips.
5. Update PROJECT_CONTEXT build state (#8) + Rule 19 checks if shell scripts touched.

## Full Context Dump
- Toggle contract: C++ stages 1/2 via nextSource(); script consumes; desired=null until bootstrap replies.
- Sweep applies only when `desired !== null && now - lastReplyMs < 2000`.
- isKitty: class contains 'kitty' (case-insens), not deleted/desktopWindow/dock.
- kwinrulesrc kitty rule to delete: group with Description=kitty-borderless / wmclass kitty — exact group names seen: [3] (noborderrule Force=2, wmclassmatch RegExp=3, listed in [General] rules=).
- DBus: service org.kde.kittyglow, path /sync, iface org.kde.kittyglow, slots nextSource, getCurrentState, scriptLog(String).

## Next Agent Handoff Message
Fresh agent: read logs/SESSION_STATE.md then PROJECT_CONTEXT.md in
~/Projects/QubesOS/UI-Enhancements/Kwin/kitty-glow. Ask the user for
explicit "deploy" consent — do NOT touch dom0 without it. After deploy,
the kwinrulesrc kitty rule must be DELETED before the toggle can work;
propose the exact deletion command and wait for "run it".
