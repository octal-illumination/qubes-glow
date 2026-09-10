# SESSION_STATE — 2026-09-10T22:00Z (kitty-glow: LL-020 closed + seamless shipped, agent: build)

## Current Objective (1-line)
DONE — all work user-accepted and committed through 62db578. No active task; next session starts from the pending-items list below.

## Discovered Facts (session-final)
- Build #14 (v3.7, sha 2ab0c4df) live in kwin PID 51644, USER ACCEPTED ("Perfect, everything works as it should and seamless").
- Occlusion geometry final: docks expandedGeometry (LL-018), normal windows frameGeometry ONLY — halo passes beneath, dimmed by the occluder's own shadow gradient; wallpaper gap eliminated (LL-017 superseded).
- deploy.sh now sha-hard-gates (LL-022); regression registry scripts/regression-checks.sh: 8 assertions green (ll020-* x5, ll017seamless-* x2, ll016-* x1); check_not helper because bash `!` through "$@" executes command "!" (not negation).
- Probe learnings: user konsoles have FULLY TRANSPARENT bodies (useless as occluder probes; ring shows through by design); plasma panel clips the ring's top band at y≈161-166 (by design, LL-018); kitty text passes goldness masks (text contamination); MONTEREY WALLPAPER carries drifting gold-ish px; kitty window id changes per launch — ALWAYS search --class kitty (the hardcoded 0x4c001a7 is the pi-session KONSOLE, NOT kitty — bit us twice).
- LL-021 (live reload can't swap .so) and LL-022 (deploy dialog-failure exits 0) both proven again this session; mitigations in place.

## File Changes (committed 62db578)
- src/kittyglow.cpp: occludedAboveKitty two-class geometry (isDock ternary).
- scripts/deploy.sh: sha gate after transfer; scripts/regression-checks.sh: NEW.
- SPECIFICATION.md (LL-017 supersession note), ARCHITECTURE.md (§2.3), HANDBOOK.md (occlusion section), PROJECT_CONTEXT.md (§5 Build State restored — lost earlier to an atomic edit rollback — §10), HTML siblings regenerated (18e audit clean).
- logs/probe/: ll014_seam{,2,3,4}.py + crops (seam evidence).

## Decisions & Rationale
- Regression registry kept IN-REPO (user chose option (a) over cross-project open-dom0 entry — Rule 21 isolation).
- Screen-probe results were ambiguous for the seam (transparent occluder bodies) — final verification = user's eyes (accepted).

## Active Blockers
- None.

## Pending Work (next session)
1. ROADMAP/HANDBOOK: verify remaining open item = LL-018 visual panel acceptance (panel probe) — low priority, user never complained since #9.
2. Optional: kwinrc [Effect-kittyglow] config section (currently defaults: active #99ffd700 @100%, inactive gold @30%, radius 32).
3. If a kitty update changes KWin internals: re-run regression-checks.sh FIRST, then positive control (logs/probe/ll020_positive.py).

## Next Agent Handoff Message
Read PROJECT_CONTEXT.md + SPECIFICATION.md LL-017/020/021/022 first; tree clean at 62db578. Kitty: `xdotool search --onlyvisible --class kitty | head -1` (never hardcoded ids). New build loop: build.sh → deploy.sh (gate auto-verifies) → kwin_x11 --replace (consent) → ll020_positive.py → regression-checks.sh. Screen probes: beware wallpaper-gold noise, text contamination, panel top-band clip.
