# SESSION_STATE.md — kitty-glow pre-compaction brain dump

> **Note:** All documentation and code in this project are purely AI-generated.

## 1. Timestamp
2026-09-09T18:58:09Z — pi session rooted at ~/Projects/QubesOS/UI-Enhancements (Build agent).

## 2. Current Objective
LL-019 (halo penetrates front windows): approved v3.4 bundle IMPLEMENTED and
committed; Build #10 (compile+deploy+kwin restart) awaits explicit consent.

## 3. Discovered Facts
- Prior 2M-token session (01a077e8) fully recovered from ledgers; commits:
  b7ddcf1 (Build #9 live in kwin 32660) → 82e60e5 (repro+API verify) →
  e415dff (v3.4 fix) → 612442a (18e pointer strip). Git clean.
- LL-019 user repro: ANY konsole/firefox window over UNFOCUSED kitty lets the
  halo penetrate; position-independent; back windows irrelevant → systematic.
- Root cause: 120 ms occluder cache rebuilt only inside paintWindow (old
  stacking still valid), never on stacking change; unfocused kitty never
  repaints → one unclipped frame persists forever.
- Upstream kwineffects.h 5.27.8 verified: stackingOrderChanged() (no args) and
  windowActivated(EffectWindow*) exist; `activeWindowChanged` does NOT exist.
- Prior-session registry gap: LL-017/LL-018 existed only in HANDBOOK, never in
  SPECIFICATION.md — repaired this session.
- logs/SESSION_STATE.md carried a stray HTML-sibling pointer (the generator
  selection key) → kept regenerating orphan logs/SESSION_STATE.html; stripped
  per Rule 22f; scoped 18e equality diff now clean.
- Workspace-literal 18e diff command is structurally broken (recursive
  grep -rl vs non-recursive ls; hundreds of pre-existing pairs; exempt
  docs/mdns_visualizer.html) — reported to user; scoped variant used.

## 4. File Changes
- src/kittyglow.cpp — v3.4: occluders rebuilt on EVERY halo paint (cache
  symbols deleted); occludedAboveKitty(kitty, halo, scale) anchored to the
  PAINTED kitty; repaintAllKittyHalos() added; stackingOrderChanged +
  windowActivated connected in ctor; header v3.3→v3.4.
- ARCHITECTURE.md — full rewrite (was stale v1/v2: 8-layer drawGlow, m_windows).
- SPECIFICATION.md — LL-017/018 added, LL-019 added, LL-015/016 reformatted.
- HANDBOOK.md — renderer/occlusion bullets → v3.4. PROJECT_CONTEXT.md —
  Known Issues/Pending/Last Updated synced. 4 HTML siblings regenerated;
  orphan logs/SESSION_STATE.html deleted; this file rewritten.
- Ledgers appended: CHANGELOG, Audit-CHANGELOG, commands-log, Action-History.

## 5. Decisions & Rationale
- No cross-frame occlusion cache at all (vs widening 120 ms): halo paints at
  blink cadence; per-paint stackingOrder() walk is negligible; cache WAS the race.
- Repaint hooks over damage-healing: deterministic; windowActivated also fixes
  stale ACTIVE-gold halo after focus loss.
- Fail-open when painted kitty ∉ stackingOrder (window closing).
- Did not edit broken 18e rule text (Rule 11 sync gating); reported instead.

## 6. Active Blockers
- None technical. Build #10 gated on explicit "build the app" (Rule 1b).

## 7. Pending Work
1. On "build the app": HANDBOOK build steps → deploy → kwin restart → verify
   .so in /proc/$(pidof kwin_x11)/maps + B/T shortcuts via allShortcutInfos.
2. User acceptance (PROJECT_CONTEXT §7): front window over unfocused kitty →
   no penetration incl. right-after-raise; focus recolor; minimize/restore
   tracking; B/T single-fire; panel clipping (LL-018) still clean.
3. On pass: Known Issues LL-019 → resolved; commit; regenerate HTMLs.

## 8. Full Context Dump
- Occluder filters per paint: skip deleted/minimized; skip !isDock &&
  opacity<0.99; require isOnCurrentDesktop+isOnCurrentActivity; rect =
  expandedGeometry()*scale (+1 px fatten); intersect halo rect; GL flips
  scissor rects itself (top-left-origin device px).
- Compile runs in Dev-General container (F37, KWin 5.27 headers — dom0 has
  none); deploy via base64 pipe + sudo cp to dom0 plugin dir (exact path in
  HANDBOOK build section); kwin_x11 restart needed to load new .so.

## 9. Next Agent Handoff Message
Fresh agent: read PROJECT_CONTEXT.md §7/§8 and the HANDBOOK build section.
Ask the user: "Build the app?" If yes, run HANDBOOK build → deploy → restart
→ verify (.so maps + B/T shortcuts), then user acceptance per §7; on pass set
Known Issues LL-019 resolved and commit (HTMLs too). Do not edit the broken
workspace-level 18e rule text without Rule 11 sync approval.
