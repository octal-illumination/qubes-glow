# SESSION_STATE — Qubes Glow (updated 2026-09-17T11:17:05)

## Current Objective
None in flight. Items 1+2 complete: build #23 (Qubes Glow branding) deployed
+ activated; ARCHITECTURE.md method names corrected.

## Discovered Facts
- Deployed display name on dom0: "Qubes Glow" (metadata verified on target).
- Live kwin PID 102464; plugin mapped; EFFECT_LOADED=true.
- Build #23 sha d899970d…; kglowsync metadata sha 0e5ea96e….
- LL-034: stale image lacked kf5-kglobalaccel-devel (manual install lost on
  recreate); setup script now includes it; image re-committed 657fec9e….
- setup-build-container.sh needed exec bit (644→755) — build.sh invokes directly.

## File Changes (this session)
- container/setup-build-container.sh: +kf5-kglobalaccel-devel, LL-034 note, chmod +x.
- ARCHITECTURE.md: focused/global toggle names, repaintAllGlowHalos, occludedAbove,
  glowtargets.h eligibility line, kitty-only flow wording.
- PROJECT_CONTEXT.md: build #23 entry, 26-assertion correction, stale-state fixes.
- ROADMAP.md: build #23 complete; Phase 7 backlog unchanged.
- SPECIFICATION.md: LL-034 recorded; decision matrix SDF correction.
- scripts/regression-checks.sh: header rebranded (Qubes Glow).
- README.md rewritten (Qubes Glow scope); HTML siblings regenerated.
- docs/research/2026-09-15-ll033-capture-plan.md: archived notice (old paths).

## Decisions & Rationale
- Runtime IDs (kittyglow.so, kglowsync, kittyglowrc, DBus, shortcut IDs) kept —
  settings/shortcuts survive; only display branding migrated.
- Container recreated (not symlinked) per plan; mount verified qubes-glow/src.

## Active Blockers
None.

## Pending Work
- Phase 7 backlog (ROADMAP): container Fedora rebase, optional DBus poll merge,
  optional override persistence — none scheduled.
- User-directed work only.

## Full Context Dump
- dom0 bridge: ~/.local/bin/dom0 (AuthExec, password dialog, stdin/argv).
- Deploy paths: /usr/lib64/qt5/plugins/kwin/effects/plugins/kittyglow.so,
  /usr/share/kwin/effects/kittyglow/metadata.json,
  /home/chenpan/.local/share/kwin/scripts/kglowsync/.
- Verify chain: sha-gate → kwin_x11 --replace → isEffectLoaded → /proc/PID/maps.
- Regression suite: 26 assertions, scripts/regression-checks.sh.

## Next Agent Handoff
Nothing in flight. If user reports a visual issue: reproduce, diagnose against
pinned source (container /usr/include/kwineffects.h, 5.27.8), propose fix,
await consent. For any container recreation: expect gglobalaccel-devel already
in setup list (LL-034); verify mount points at qubes-glow/src before cmake.
