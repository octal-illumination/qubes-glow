# SESSION_STATE — 2026-09-10T21:00Z (kitty-glow LL-020 closure, agent: build)

## Current Objective (1-line)
DONE — LL-020 fixed (build #13, E2E-verified), docs/ledgers/commit complete. Remaining: two proposals for the user (deploy.sh hardening; regression-assertion location).

## Discovered Facts (session-final)
- Root cause LL-020 (three-part, see SPECIFICATION.md): (a) 3-arg render(region,mode,true) = caller-must-enable GL_SCISSOR_TEST (off → full-quad leak); (b) boxes degenerate in our context → test-on renders NOTHING; (c) paintWindow's region = window frame only in KWin 5.27.8 (prePaint widening does NOT propagate) and the rasterizer never enforces region.
- Final mechanism (src/kittyglow.cpp v3.6): clip = haloRect − occludedAboveKitty(); one sub-quad per clip rect via 1-arg render(GL_TRIANGLES); isDesktop() excluded from occluders.
- Verification (build #13 cb63bd4b, kwin PID 51070): positive control 5,214 strict/25,896 loose ring px (kitty unoccluded, konsoles minimized); occlusion 0 px under konsoles; 24-cycle+5-idle burst flat 320/394 (text noise). Wallpapers ≈18k gold px (explains run1 "C-states"; never halo).
- LL-021: live unload/load does NOT swap .so code (QPluginLoader keeps library mapped) — kwin_x11 --replace required per build; verify behavior not the DBus boolean.
- LL-022: deploy.sh = three dialog-gated dom0 calls; swallowed failure exits 0 with stale artifact (cost one restart cycle); ALWAYS sha-verify dom0 vs dist after deploy.
- Probe quirks: xdotool slow with --sync (24-cycle burst needs timeout≥300); windowraise ineffective on managed windows; activating an already-active window → no repaint → no effect logs; desktop wallpaper has ~85-960 drifting gold-px noise.

## File Changes (all committed 8b5f656 + follow-up)
- src/kittyglow.cpp: v3.6 subdivision mechanism + isDesktop skip; instrumentation stripped.
- SPECIFICATION.md LL-020/021/022; ARCHITECTURE.md §2.3/§3/§4; HANDBOOK.md §5 LL-021/022 caveats; PROJECT_CONTEXT.md §5/§8/§10; HTML siblings regenerated.
- logs/probe/: ll020_final.py, ll020_v35_burst.py, ll020_positive.py (+crops).
- Ledgers: CHANGELOG, Audit-CHANGELOG, Action-History, commands-log updated per action.

## Decisions & Rationale
- Rejected: scissor debugging rebuild cycles (uncertain, heavy dom0 disruption) → CPU subdivision (deterministic, SDF position-based = pixel-identical sub-rects).
- Rejected: keeping region∩halo as a clip source (empirically frame-only → halo invisible).
- Open proposals (NOT implemented): (1) deploy.sh sha-gate exit-nonzero on mismatch; (2) kitty-glow regression assertion location — open-dom0/regression-checks.sh is cross-project (Rule 21); suggest in-repo logs/../checks or explicit user approval for open-dom0.

## Active Blockers
- None. Build #13 live and verified.

## Pending Work (for next session)
1. User decision on deploy.sh hardening + regression assertion location; implement per answer.
2. Visual acceptance pass by user (halo appearance on-screen; panel LL-018 visual test still open from #9).
3. Optional: raise default alpha/color config into kwinrc [Effect-kittyglow] (currently defaults: #99ffd700 active, radius 32; section absent from kwinrc).

## Next Agent Handoff Message
Read PROJECT_CONTEXT.md + SPECIFICATION.md LL-020/021/022 first. Tree is clean at 8b5f656(+). Kitty test window: xdotool search --onlyvisible --class kitty (id changes per launch). Any new build: build.sh → deploy.sh → SHA-VERIFY dom0 vs dist → kwin_x11 --replace (consent) → positive control via logs/probe/ll020_positive.py. Never trust loadEffect booleans or deploy exit codes alone.
