<!-- HTML sibling: regenerate ONLY via bash ~/Projects/scripts/generate-docs-html.sh <this-file> — Rule 18c left-aligned CSS. Never hand-roll pandoc. -->
# ROADMAP — Qubes Glow

> **Note:** All documentation and code in this project are purely AI-generated.

Phase tracker. Design rules live in SPECIFICATION.md (referenced by LL-ID);
state snapshot lives in PROJECT_CONTEXT.md. Refreshed 2026-09-11 (was stale
at build #7 — audit finding M4).

## Completed phases

- [x] Phase 1 — kitty halo PoC → v3: SDF shader, occlusion clipping
      (LL-017/018/019/020), seamless minimize tracking, shortcut rework
      (builds #1–#14).
- [x] Phase 2 — activation pipeline: build.sh (container), deploy.sh
      (sha-gated, LL-022), kglobalaccel registration healing (LL-011).
- [x] Phase 3 — persistence without rules: kittyglowrc state store,
      kwinrulesrc retired (write-revert fight, LL-016), kglowsync script
      owns noBorder writes (builds #5–#7).
- [x] Phase 4 — all-windows glow: chrome exclusion by class (LL-026 —
      qubes-gui strips _NET_WM_WINDOW_TYPE), ghost-square size guard,
      kglowsync bootstrap watchdog (builds #15–#17).
- [x] Phase 5 — per-window toggles + global masters (build #18, v3.10):
      Meta+Shift+B/G = focused window (runtime-only overrides, sweep-safe
      via the script's overrides map); Meta+Shift+Alt+B/G = persisted
      global masters (LL-027). User-accepted live.
- [x] Phase 6 — deep audit (2026-09-11): 0 critical/high; M1–M4 + L1–L11
      fixed same day (this refresh is M4; see SPECIFICATION LL-028/029).

- [x] Build #22 popup exclusion — user accepted menus and notifications.
- [x] Branding — Qubes Glow; project directory `qubes-glow`; runtime IDs retained.
- [x] Build #23 — Qubes Glow display metadata shipped + activated (KWin 102464);
      container recreated on qubes-glow mount (LL-034: kf5-kglobalaccel-devel
      added to setup list, image re-committed 657fec9e…); exec-bit fix on
      setup script. Display-name change is DONE — no further build needed.
- [ ] Recreate the old-path container mount with that build (HANDBOOK Section 2).

## Current phase

- [ ] Phase 7 — hardening backlog:
      - [ ] Rebase the build container off EOL Fedora 37 (audit L11;
            needs container setup rework, no runtime exposure).
      - [ ] Optional: merge the two 60 ms DBus polls into one composite
            call (efficiency note; only if ever profiled hot).
      - [ ] Optional: persist per-window overrides across kwin restarts
            if the user ever asks (currently runtime-only by design).

## Completion criteria (standing)

- Zero-warning builds; every fix lands with one regression assertion
  (Rule 20a); docs regenerated in the same step as code (Rule 14);
  physical-key verification of shortcuts after any kwin restart (LL-027).
