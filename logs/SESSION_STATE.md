# SESSION_STATE — kitty-glow (updated 2026-09-11T12:57:16+05:30)

## Current Objective
Re-audit 2 fix batch implemented (build #20) — awaiting build consent.

## Discovered Facts
- Pinned-source adjudication (kwin 5.27.8 in dom0-replica-fed37,
  /tmp/kwinsrc): effect-facing regions are LOGICAL px; ONE scale boundary
  at the vertex upload (ortho box = rect*scale, itemrenderer.cpp:45;
  Scene::addRepaint unscaled scene.cpp:92; mapToRenderTarget
  itemrenderer_opengl.cpp:331; toMatrix kwineffects.cpp:208). Previous
  audit's M2/LL-028 was wrong; corrected.
- qRound(int) ambiguous in Qt5 — maxExtent() int feeds direct assignment.
- Re-audit 2 report: docs/research/2026-09-11-reaudit-2.md (commit 4c97409).

## File Changes (this batch, commit 3f3283b)
- src/kittyglow.cpp: F1/F2/F3 space fixes, occludedAbove() signature
  (scale param dropped), header → build #20, qRound fix.
- src/glowconfig.h: maxExtent ceil + <cmath>.
- src/kittyglowstate.cpp: dead constants removed.
- src/kwin-script/kglowsync/contents/code/main.js: bootWatch stop,
  clientAdded heartbeat nuance doc.
- scripts/kittyglow-rollback.sh: kglowsync strip (M5).
- scripts/regression-checks.sh: ll028 flipped, +m5 assertion (25 total).
- src/kittyglow.json + kglowsync metadata.json: version 3.10.1.
- SPECIFICATION.md: LL-028 rewrite, +LL-030/031. HANDBOOK §7, PROJECT_CONTEXT #20.

## Pending Work
- Build #20: user build consent → compile → deploy (password) → restart
  consent → kwin restart (s=1: behavior identical; correctness-only).
- ROADMAP Phase 7 backlog (L11 container rebase etc.).

## Next Agent Handoff
If user says "build the app": bash scripts/build.sh (0-warning gate via
container /tmp/b_make.log), then scripts/deploy.sh, then ask restart
consent. Verify assertions + deployed sha. Nothing else in flight.
