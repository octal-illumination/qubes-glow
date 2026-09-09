<!-- HTML sibling: regenerate ONLY via bash ~/Projects/scripts/generate-docs-html.sh QubesOS/UI-Enhancements/Kwin/kitty-glow/logs/SESSION_STATE.md — Rule 18c left-aligned CSS. Never hand-roll pandoc. -->
# SESSION_STATE.md — kitty-glow pre-compaction brain dump

> **Note:** All documentation and code in this project are purely AI-generated.

## 1. Timestamp
2026-09-09T20:05Z (this session: continuation after prior 2M-token session 01a077e8 died)

## 2. Current Objective
LL-019: fix the kittyglow halo-occlusion artifact — the halo penetrates ANY
unmaximised/unminimised window placed in front of kitty (any app: konsole,
firefox; any position; back windows irrelevant). Revised fix bundle PROPOSED,
awaiting "implement changes".

## 3. Discovered Facts
- Project: `~/Projects/QubesOS/UI-Enhancements/Kwin/kitty-glow`, git clean at HEAD
  `b7ddcf1` (Build #9 live on dom0, kwin PID 32660; B+T shortcuts registered).
- KWin **5.27.8** on dom0, X11. Effect v3.3: occlusion-clipped SDF halo
  (`src/kittyglow.cpp`), shader `src/glowshader.cpp`, config `src/glowconfig.h`.
- Upstream `kwineffects.h` v5.27.8 fetched to `/tmp/kwineffects-5.27.8.h` (public
  API; `src/effects.h` is the internal impl header — do not use). Verified:
  - `void stackingOrderChanged()` line 1820 — EffectsHandler signal since 4.10,
    "emitted when ever the stacking order is change, ie. a window is risen or
    lowered" → the missing deterministic healing hook.
  - `void windowActivated(KWin::EffectWindow *w)` line 1550 — correct focus
    signal; the earlier-planned name `activeWindowChanged` does NOT exist in
    5.27.8 (would have failed to compile).
  - NO `EffectsHandler::movingWindow()` in 5.27 (m_movingWindowsSet is private
    to WindowMotionManager, lines 3744-3764); `EffectWindow::isUserMove()`
    line 2456 exists for drag hardening.
- Prior-session upstream verification stands: `GLVertexBuffer::draw` in
  5.27.8 (`libkwineffects/kwinglutils.cpp`) is a plain per-rect scissor loop —
  no rect-count limit, no bounding-rect fallback, y-flip side-agnostic → the
  GL clip path itself is symmetric and sound.
- Root-cause model (user's simplified repro kills the left/right theory):
  during window placement (raise/drag transition) at least one halo frame
  draws unclipped, because occluders are (a) cached 120 ms and (b) rebuilt
  only from inside paintWindow while kitty is still topmost in the OLD
  stacking, and (c) never rebuilt on stacking change. Because an UNFOCUSED
  kitty then never repaints, that single bad frame persists indefinitely.
- Prior defect registry (from SESSION_STATE of 18:20Z): A wrong anchor
  (topmost kitty in stackingOrder, not painted w); B stale-cache race (rebuild
  while kitty topmost stores EMPTY occluder list → unclipped frame); C drag
  lag (scissor up to 120 ms behind); D stale color (no focus repaint hook).
  User's repro is consistent with B/C primarily; A needs 2+ kitty windows.
- paintWindow() clip path: `clip = region ∩ halo; if(!cacheFresh)
  updateOccluders(); if(!m_occluders.isEmpty()) clip -= occludedAboveKitty(...)`.
  updateOccluders(): kittyIdx = LAST kitty in stackingOrder(); occluders =
  windows after kittyIdx, skipping deleted/minimized/translucent(<0.99
  non-dock)/off-desktop. occludedAboveKitty(): expandedGeometry() rects ×
  render scale, +1 px fatten, ∩ halo.

## 4. File Changes (this session)
- logs/output-history.md — LL-019 addendum 3 (simplified repro, API verification)
- logs/Action-History.md — user-repro entry + response
- logs/commands-log.md — upstream header fetch + grep entries
- logs/CHANGELOG.md — session-recovery commit entry
- logs/SESSION_STATE.md — this file (refresh)
- All committed in the session-recovery commit; NO source changes
  (Rule 1a: awaiting explicit "implement changes").

## 5. Decisions & Rationale
- No source edits before user consent (Rule 1a) — fix bundle PROPOSED, not applied:
  1. Rebuild occluders on EVERY halo paint; delete the 120 ms cache
     (m_occluders/m_stackingStamp) — kills B + C.
  2. Anchor occluders to the PAINTED kitty window w, not topmost kitty — kills A.
  3. Connect `stackingOrderChanged()` → repaint full halo ring of every kitty
     window — heals placement penetration in one frame.
  4. Connect `windowActivated()` → repaintHalo of the previously active kitty —
     fixes stale ACTIVE-gold color (D).
  5. Optional drag hardening: while `w->isUserMove()/isUserResize()`, skip the
     clip cache entirely (rebuild in-loop).
- Fix is expected small (~30-40 changed lines, all in src/kittyglow.cpp).

## 6. Active Blockers
- None diagnostic. All dom0 actions (deploy/restart/probe) need password-dialog
  consent rounds; source edits need "implement changes"; build/deploy needs
  "build the app" (Rule 1).

## 7. Pending Work
1. On "implement changes": apply the 5-item bundle in src/kittyglow.cpp,
   Rule 16 verification (build + regression-checks.sh), docs sync
   (HANDBOOK/PROJECT_CONTEXT/SPECIFICATION LL-019 entry, HTML regen), commit.
2. On "build the app": scripts/build.sh → deploy → kwin restart → user
   acceptance test (place konsole/firefox window over kitty → no penetration,
   focus switch recolors halo, panel regression LL-018 still clean).

## 8. Full Context Dump
- Prior session recovered via logs/Action-History.md + output-history.md tails
  (LL-016/017/018 chain, Build #9 deployed+verified, commits d697c0f→b7ddcf1).
- Verbatim user repro (this session): "the glow penetrating artifact happens to
  any unmaximised unminimised window that is placed in front of it. This
  happens to anywaind i guess, beause i tested with konsole terminal and
  firefox, both exhibitted the same behaviour." Plus earlier: front window
  right/left of kitty made no difference; a maximised window behind kitty is
  irrelevant.
- Effect paint order: paintWindow draws kitty normally FIRST, then the halo
  quad with GL_BLEND, GLVertexBuffer::render(clip, GL_TRIANGLES, true) — the
  clip region is the only thing keeping the halo off front windows.

## 9. Next Agent Handoff Message
"Read logs/SESSION_STATE.md (this file) + LL-019 addendum 3 in
logs/output-history.md. The fix bundle is fully specified in section 5 — on the
user's explicit 'implement changes', apply items 1-4 (5 optional) in
src/kittyglow.cpp, run Rule 16 verification, sync docs + HTML siblings,
commit. Build/deploy only on explicit 'build the app'."
