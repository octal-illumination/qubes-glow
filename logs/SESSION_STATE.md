# Session State

**Timestamp:** 2026-09-09T17:55Z (build agent, pi session for kitty-glow Phase-2 verify)

## Current Objective
Verify Phase-2 criterion "glow does not penetrate other windows / surfaces" on dom0 with synthetic Kitty-class windows, and fix any confirmed artifact.

## Discovered Facts (all verified this session)
- dom0 X11, user chenpan (uid 1000), tools present: xdotool, wmctrl, kstart5, xterm, spectacle. No xterm/xdotool in Dev-General (qvm-run returned empty).
- kglowsync effect is loaded and live: `journalctl --user _COMM=kwin_x11` shows `kglowsync(js): clientAdded: applied desired=true` for a synthetic `-class Kitty` xterm (borderless applied, frame extents 4,4,23,6).
- KWin scripting `loadEffect` on this build returns false for newly installed script packages (kglowprobe unusable without restart) — probed via kglowsync's DBus scriptLog channel instead.
- xterm `-geometry` is in CHARACTER CELLS (900x650 => screen-filling window). Small window = `80x24+300+180`.
- xterm windows self-retitle (`user@dom0:/run/qubes`), breaking name-based window lookup; CLASS-based xdotool search + `windowactivate --sync` works.
- Halo IS rendered for Kitty-class xterm: pale-gold ring measured (17,807 px @ clip-g8, bbox x[289..794] y[169..506]).
- CRITICAL measurement lesson: halo color at 60% alpha over white desktop = (255,242,175)-ish; a `B<90` gold mask MISSES it. Use alpha-aware mask `(R>235)&(G>170)&(40<B<200)&(R>=G)&(G>B+15)`.
- Static clip under opaque front window: VERIFIED. Cover xterm over halo window => 0 halo px through cover (g9a-c, burst + blink repaints, simulated drag: no change).
- CONFIRMED ARTIFACT (user-reported class): halo renders ON TOP of plasma panel. Panel-zone gold = 1,450 px when ring intersects panel (diag-g3/g4) vs 48 px baseline. Saturated gold (159,137,16) over panel accent at y=745, tinted panel body (241,230,203).
- Root cause: kittyglow.cpp `updateOccluders()` line ~236 skips windows with `opacity() < 0.99` (intentional for translucent windows) => translucent plasma panel never clips the halo; panel does not repaint each frame so halo pixels persist over panel chrome.
- Prior session (01a077e8) history: git initialized, Phase-1 commit d697c0f on branch; kglowsync deployed via kpackagetool5; regression-checks.sh exists in open-dom0 (not this project).

## File Changes
- logs/probe/diag-g3.png, diag-g4.png — first giant-xterm batch (halo-over-panel evidence)
- logs/probe/burst-g6.png..g7d.png — failed-burst batch (desktop only; kept for baseline)
- logs/probe/clip-g8.png, clip-g9a.png..g9d.png — correct burst batch (static-clip verification)
- logs/commands-log.md — appended per command batch
- No source files modified this session yet.

## Decisions & Rationale
- Probe via DBus scriptLog + xterm class-override instead of kglowprobe package (loadEffect=false on this build; restarting kwin_x11 refused without explicit user consent per Rule 1c).
- Proposed fix (NOT yet implemented — awaiting "implement changes"): in `updateOccluders()` treat docks as occluders regardless of opacity:
  `if (!w->isDock() && w->opacity() < 0.99) continue;` (translucent normal windows keep intentional bloom-through; panels must clip).

## Active Blockers
- None technical. Awaiting user consent for the one-line fix, then rebuild+redeploy needs kwin effect rebuild (build consent separate per Rule 1b).

## Pending Work
1. User approval of dock-occluder fix.
2. Edit src/kittyglow.cpp updateOccluders() (docks always occlude).
3. Rebuild effect + redeploy to dom0 + restart kwin (needs explicit consent; restart required for effect reload per this build).
4. Re-run panel-intersection probe: expect panel-zone gold == 48 px baseline.
5. Update CHANGELOG/PROJECT_CONTEXT/HANDBOOK; add LL entry (panel translucency => docks always clip).

## Next Agent Handoff Message
Read this file, then src/kittyglow.cpp updateOccluders() (~line 224). The verified fix is: docks become occluders even when translucent. Ask the user for "implement changes" + "build" consent before editing/compiling. Probe frames + masks already in logs/probe/; reuse the alpha-aware halo mask (do NOT use B<90 mask — it misses pale halo on white).
