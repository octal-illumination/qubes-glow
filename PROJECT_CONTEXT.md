<!-- HTML sibling: regenerate ONLY via bash ~/Projects/scripts/generate-docs-html.sh QubesOS/UI-Enhancements/Kwin/qubes-glow/PROJECT_CONTEXT.md — Rule 18c left-aligned CSS. Never hand-roll pandoc. -->
# Qubes Glow — Project Context

> **Note:** All documentation and code in this project are purely AI-generated.

## 1. Project Overview
**Qubes Glow**: KWin 5.27.8 / Qt5 / C++20 effect for Qubes OS dom0 X11.
VM-label-aware halos on eligible application windows, with focused/global
border controls. Repository directory: `qubes-glow`; runtime IDs unchanged.

## 2. Key File Map
| Path | Responsibility | ~Lines |
|------|----------------|--------|
| `src/kittyglow.cpp` | `KWin::Effect` subclass; quad + shortcut + repaint hooks | 288 |
| `src/kittyglowstate.cpp/.h` | Persistent borderless state in `kittyglowrc` (kwinrulesrc retired) | 76 |
| `src/glowshader.cpp/.h` | GLSL SDF glow shader (per-side soft falloff) | 130 |
| `src/glowconfig.h` | kwinrc `[Effect-kittyglow]` config reader (live-reload) | 80 |
| `src/CMakeLists.txt` | Plugin build (C++20, KWin links) | 40 |
| `src/kittyglow.json` | Effect metadata (Id, ServiceTypes) — embedded in `.so` | 15 |
| `container/setup-build-container.sh` | Fedora-37 / KWin 5.27.8 build env | 45 |
| `scripts/build.sh` | Compile in container → `dist/` | 30 |
| `scripts/deploy.sh` | Copy into dom0 + enable in kwinrc | 30 |

## 3. Architecture Summary
`KittyGlowEffect` tracks eligible managed application windows and draws ONE
hardware-blended triangle-fan quad around the frame rect; a GLSL SDF shader
(`glowshader.cpp`) computes per-side soft falloff (v3 — replaced the v2
8-stacked-rect approach). Thickness/radius/active-inactive colors come from
kwinrc `[Effect-kittyglow]`, live-reloadable via `reconfigureEffect`. Damage
is widened in `prePaintWindow` + repaint hooks on geometry/minimize so the
halo never smears. Full design: ARCHITECTURE.md.

## 4. Database / Data Layer
No database. `kittyglowrc` stores global glow/border state; kwinrc stores
appearance and enable settings. Per-window overrides and label cache are runtime-only.

## 5. Build State
- **Build #24 (v3.12.0, default GlowRadius=16) — SHIPPED;
  activation verified after authorized restart.**
  SHA256: `84f2a03161aa7a6d4fcdf2fb092b1793e80b99f666bc60dfac98e9d312a4b0ac`.
  Container configure/build: zero warnings/errors; 26/26 regression checks.
  Evidence: logs/build/build24*.log; logs/output/build24-verification.log.
  All four installed hashes match; modes 644; GlowRadius=16; both enable
  flags true. Authorized restart: PID 103269 -> 103625; effect loaded=true,
  new plugin mapped, GlowRadius=16. Evidence: logs/output/build24-activation.log.
  Earlier unloaded-effect state resolved; its cause was not established.
- Settled live appearance: `GlowRadius=16` in dom0 kwinrc
  `[Effect-kittyglow]`; prior read-back confirmed 16. Code default
  changed to 16 (glowconfig.h defaults + kwinrc fallback); build #24
  compiled and deployed — dom0 configuration already specifies 16
  explicitly. Tuning reference: HANDBOOK.md Section 9.
- **Build #23 (v3.12.0, Qubes Glow branding) — SHIPPED 2026-09-17,
  sha d899970d…, deployed + activation-verified (KWin PID 102464).**
  Display metadata now "Qubes Glow" (verified on dom0); runtime IDs
  (kittyglow.so/kglowsync/kittyglowrc/DBus/shortcut IDs) unchanged by design.
  Deploy sha-gated (all four artifacts OK); restart 102178 → 102464;
  plugin mapped, EFFECT_LOADED=true. Container recreated on the qubes-glow
  mount with kf5-kglobalaccel-devel installed (was missing from the stale
  image — LL-034, setup script fixed + image re-committed 657fec9e…).
  Zero-warning compile; 26/26 assertions.
- **Build #22 (v3.12.0, unmanaged-popup exclusion) — BUILT 2026-09-15,
  sha 25169edd…, deployed and activation-verified (KWin PID 101201).**
  All four installed artifact hashes and both enable flags verified;
  restart 100860 → 101201, effect loaded and plugin mapped. User accepted
  all menu glow, flicker and notification fixes: "done. everything works." Evidence: logs/output/build22-activation.log.
  New isManaged() gate in
  glowtargets.h rejects override-redirect chrome (Qt QMenu drop-downs,
  combo popups, tooltips) that previously drew their own halo (LL-033 —
  user report: menu glow + flickering bottom-edge outline). EffectWindow
  has no isUnmanaged(); isManaged() = window->isClient() (effects.cpp:2003).
  Zero-warning compile in container; 26/26 assertions (new ll033; earlier
  "27" notes were a miscount — HEAD had 25, +ll033 = 26).
- **Build #21 (v3.11.0, per-VM label hue) — SHIPPED 2026-09-13,
  sha 2b43c24f, kwin pid 81141.** Label hues resolving live on first
  paint (dev-general:* => #edd400 each, once per window then cached);
  dom0-native windows confirmed atom-less (white fallback). GlowColor/
  Inactive=255,255,255 (white) persisted for dom0. Cross-VM hue proof
  pending a red/blue VM window. Batch: Halo color now follows each VM's Qubes label
  (_QUBES_LABEL_COLOR, xprop-verified 0x00EDD400 on Dev-General) —
  new src/glowlabel.{h,cpp}, cached one read per window lifetime, pruned
  on windowDeleted, active/inactive = opacity over the hue, LabelColor
  config key (default true; false = configured gold), dom0-native windows
  fall back to gold. 26 assertions; C++ syntax-verified in container.
- **Build #20 (v3.10.1, re-audit-2 fix batch) — SHIPPED 2026-09-11,
  sha f9389aec, kwin pid 65324.** Functional probe identical to #19
  (G+Alt+G 80 ms both fired; master restored on) — behavior-neutral at
  s=1 as predicted; space correctness restored for HiDPI. Batch: F1/F2/F3 coordinate-space corrections (LL-028 rewritten:
  effect-facing regions are LOGICAL; one scale boundary at the vertex
  upload — corrected audit M2's wrong rule), M5 rollback completeness
  (kglowsync stripped), qRound ambiguity fix, maxExtent ceil, dead
  constants removed, bootWatch stop, versions unified to 3.10.1.
  25 assertions green; C++ syntax-verified in container (moc +
  -fsyntax-only). Report: docs/research/2026-09-11-reaudit-2.md.
- **Build #19 (audit-fix batch) — SHIPPED 2026-09-11,
  sha 07537ab5, kwin pid 64176.** M3 gate fix behaviorally verified live
  (G + Alt+G inside 80 ms → both fired; shared gate would drop 2nd).
  Glow master restored on. Audit-fix batch: per-action autorepeat gates
  (LL-029), M2 "device-px" widening — WRONG SPACE, corrected by re-audit 2
  (LL-028), scriptLog newline sanitize, glowconfig clamps,
  orphaned comment removed, v2-rollout-round.sh purged, metadata de-drift,
  ROADMAP refreshed. Deep audit: docs/research/2026-09-11-deep-audit.md.
- **Build #18 (v3.10, per-window toggles + global masters, 2026-09-11)**
  — sha `7df23937…`, deployed + sha-verified, live in kwin 62946. B/G now
  act on the FOCUSED window (runtime-only overrides; script-side overrides
  map shields them from the 400 ms sweep — LL-027); global masters moved to
  Meta+Shift+Alt+B/G (persisted, sweep + reset semantics). VERIFIED LIVE:
  focused B flips only the focused window (journal `focused-op` lines,
  konsole AND kitty, sweep-safe), focused G fires scope-named, Alt-masters
  sweep/flip with 4734 px visual swing, user physical-key acceptance
  PASSED. New files: src/glowfocus.{h,cpp}.
- **Build #17 (v3.9.1, ghost-square fix + kglowsync watchdog, 2026-09-11)**
  — sha `d66b9009…`. Size guard (sub-48 px windows never haloed) kills the
  unmanaged Qui-* tray source ghosts (corner gold 456 → 0); bootstrap
  watchdog re-arms on lost replies.
- **Build #16 (v3.9, shortcut split + chrome-class exclusion, 2026-09-11)**
  — superseded by #17 within the hour (its kglowsync wedged on a lost
  bootstrap reply; size guard added for the ghost square).
- **Build #15 (v3.8, all-windows glow, 2026-09-11)** — sha `6d8886da…`,
  deployed + sha-verified, live in kwin PID 56055. Eligibility is
  window-type-based (normal app windows glow; dialogs/notifications/OSD/
  menus/tooltips/splash/utility excluded — LL-023). Verified: kitty ring
  1,287 px on clean layout; dialog band 112 (noise-level, PASS);
  notification dock-strip band 0. kglowsync hardened against the
  `--replace` bus-name race (LL-025, resilient bootstrap + retry).
- **Build #14 (v3.7, seamless pass-behind, 2026-09-10)** — sha `2ab0c4df…`,
  deployed + gate-verified, live in kwin PID 51644. Non-dock occluders clip
  at `frameGeometry()` only (halo passes behind windows, dimmed by their
  shadow gradient; no wallpaper gap). **USER ACCEPTED: "Perfect, everything
  works as it should and seamless."** Positive control bit-identical
  (5,214/25,896). Also: `scripts/deploy.sh` now hard-gates on sha mismatch
  (LL-022); `scripts/regression-checks.sh` added — 8 assertions, all green.
- **Build #13 (v3.6, LL-020 final, 2026-09-10)** built zero-warning.
  Artifact: `dist/kittyglow.so`, sha256 `cb63bd4b…`, deployed + sha-verified
  on dom0, live in kwin PID 51070. Mechanism: halo clip = `haloRect −
  occluders` (scene region advisory — LL-020c), CPU-subdivided into one quad
  per clip rect drawn unclipped via 1-arg `render(GL_TRIANGLES)`;
  `isDesktop()` excluded from occluders. E2E-verified: positive control
  5,214 strict / 25,896 loose ring px with kitty unoccluded; occlusion 0 px
  under konsoles; 24-cycle leak burst flat at 320/394 text-noise (no
  accumulation).
- **Build #8 (v4, Step C rebuild, 2026-09-09)** built zero-warning. Artifact:
  `dist/kittyglow.so`, sha256 `25e019db…`. Replaces the kwinrulesrc rule
  toggle with `kittyglowstate.cpp` (`~/.config/kittyglowrc` `[General]
  noBorder`) — root cause: a loaded forcing rule overrides scripting
  `noBorder` writes (write-revert fight, LL-016 family). Adds
  `getCurrentState()` DBus slot consumed by the kglowsync script bootstrap
  (kwin/script restart coverage). `kittyborderrule.cpp` retired.
  **Deployed + E2E-verified on dom0 2026-09-09:** effect auto-loaded on
  reconfigure, script bootstrap live (journal), kitty forcing rule DELETEd
  from kwinrulesrc (count=0), 4-press kglobalaccel E2E — rc flips in sync,
  single stuck sweep write per toggle, zero reverts, steady-state silence.
  → toggles still flip parity, `rules=` preserved, canonical name restored
  after.
- **Shortcut recovery (dom0, this session):** Sep 08 boot race (kwin 08:58:11
  before kglobalaccel 08:58:13) killed B/T registration. kwin restarted
  AFTER daemon → both shortcuts live (B=301989954, T=301989972); kwinrulesrc
  normalized to single active `[kitty-borderless]`. Lesson LL-011.
  A real kwin crash during testing auto-recovered and re-registered both
  shortcuts (daemon already up) — boot-order theory re-validated.
- Command: `scripts/build.sh` (cmake + make inside `dom0-replica-fed37`).
- Deployed to dom0: `/usr/lib64/qt5/plugins/kwin/effects/plugins/kittyglow.so`
  and `/usr/share/kwin/effects/kittyglow/metadata.json` (both `644`);
  dom0 sha256 matches `dist/` byte-for-byte.
- Enabled in `kwinrc` `[Plugins] kittyglowEnabled=true`; live via
  `org.kde.kwin.Effects.loadEffect` → `true`. Historical reference: kwin PID
  15974 (2026-09-07 era, `--crashes 1` recovered); current live reference:
  PID 101201 (build #22 activation, 2026-09-15).
- Parity at rest: `rules=kitty-borderless`, `noborder=true` (borderless).

## 6. Active Features
- **All-windows glow (build #15)**: every normal application window gets
  the halo; unmanaged override-redirect chrome (LL-033), dialogs,
  notifications, OSD, popup menus, tooltips, splash and utility windows
  are excluded (window-type + isManaged predicates, LL-023/LL-033).
- SDF soft-falloff halo (v3 shader; per-side thickness/radius/colors from
  kwinrc; label hue when LabelColor=true — LL-032). The 8-layer v2 halo is
  historical.
- Fullscreen suppression; OpenGL-compositing guard.
- Live activation over DBus (`/Effects` loadEffect) — no compositor restart.
- Historical: the kitty forcing rule in `kwinrulesrc` (`[kitty-borderless]`,
  noborder Force=2, RegExpMatch) was retired 2026-09-09 (LL-016); state lives
  in `kittyglowrc` + kglowsync. kwinrulesrc must carry no `noborderrule`.
- **Meta+Shift+B** → effect-registered `Toggle Kitty Borderless` (registration
  ID kept for kglobalaccel; semantics are build #18): stages a FOCUSED-window
  border flip via `KittyToggle::requestWindowOp()`; kglowsync applies
  `noBorder` live (overrides map shields it from the sweep — LL-027).
  The persisted class-wide/global master is Meta+Shift+Alt+B.
- **Meta+Shift+G** → effect-registered `Toggle Glow` (build #16): flips
  `kittyglowrc` `glowEnabled` + full repaint; the glow switch moved off B
  by user directive 2026-09-11.
- **Meta+Shift+T** → native `Window No Border` (per-focused-window toggle);
  rebound from Meta+Shift+B, dead `kitty-toggle-border` entry deleted.
- Shortcut registration self-heals whenever kwin starts after kglobalaccel
  (incl. crash auto-restarts); manual healing = restart kwin_x11 last (LL-011).

## 7. Pending / In-Progress
- Branding source metadata updated to Qubes Glow; not built or deployed.
- Next build needs old-path container mount recreated (HANDBOOK Section 2).
- Repository renamed to `qubes-glow`; installed runtime names remain unchanged.
- Superseded 2026-09-15: all-windows-glow aesthetics acceptance resolved —
  user accepted build #22 end-to-end ("done. everything works.").

## 8. Known Issues
- Halo penetrates any window placed in front of kitty (LL-019): the 120 ms
  occluder snapshot lagged restacks and an unfocused kitty never repainted
  the one unclipped frame away (repro: konsole AND firefox, position-
  independent; back windows irrelevant). **Fix implemented (v3.4): occluders
  rebuilt on every halo paint, anchored to painted kitty, plus
  stackingOrderChanged/windowActivated full-ring repaint hooks. Build #10
  (sha 31254894) deployed + live in kwin 36048, B/T re-registered after
  LL-011 heal-restart. ~~USER ACCEPTANCE TEST PENDING~~ **RESOLVED +
  E2E-verified on build #13 (2026-09-10): zero penetration in 24-cycle
  burst; occlusion 0 px under konsoles.**
- Halo paints over translucent plasma panel (LL-018): panel (opacity < 0.99)
  was never an occluder, so halo repaints left gold on panel chrome (probe:
  1,450 px vs 48 px baseline). **Fix shipped in Build #9, live in kwin 32660;
  v3.4 keeps the docks-always-clip rule in the per-paint occluder walk.
  Visual panel acceptance test still pending.**
- ~~LL-020 halo-invisible regression (scissor-enable build #10)~~
  **RESOLVED 2026-09-10 on build #13**: scene paint region is frame-only
  (prePaint widening does not propagate) and KWin's hw-clipping scissor
  boxes are degenerate in our context — final mechanism clips against
  occluders only and CPU-subdivides the ring into unclipped sub-quads
  (SPECIFICATION.md LL-020; positive control 5,214/25,896 px with kitty
  unoccluded, leak burst flat at text-noise).
- ~~Build container (`dom0-replica-fed37`) still mounts legacy `/home/user/kitty-glow`~~
  **RESOLVED 2026-09-06T23:50:19Z:** container re-created mounting this project's
  `src/` at `/src` (committed image `dom0-replica-fed37-img` preserves toolchain).
- Historical: per-window toggles shipped in build #18 (ROADMAP Phase 5).
- Pre-existing: three chromium UUID rule groups in `kwinrulesrc` are not listed
  under `[General] rules=` → silently inert (user data, left untouched).
- ~~KWin crashed once during the v3 live-swap~~ **RESOLVED
  2026-09-07T23:00Z:** hot-swap crash root-caused to the live-reload path;
  `kwin_x11 --replace` restart path is clean (PID 209874, zero errors).
- ~~Orphan kglobalaccel5 PID 167889~~ **RESOLVED 2026-09-07T23:00Z:** killed on
  user consent; unit restarted; single daemon (209480) verified on bus with
  both shortcuts registered. Root cause of T being dead: the orphan's stale
  boot-time key grabs shadowed the fresh daemon's registry.

## 9. Connected Targets
- **dom0** (X11 `:0`, KWin 5.27.8) — deployment + activation target.
- **Dev-General** AppVM — hosts source, build container, `dom0` helper.
- **dom0-replica-fed37** Podman container — Fedora 37 build env (KWin 5.27.8 devel).

## 10. Last Updated
2026-09-17 — Union Alpha: build #24 restart authorized and completed;
PID 103625, effect loaded, plugin mapping and GlowRadius=16 verified.
Exact activation timestamp is recorded in logs/CHANGELOG.md.
2026-09-17T07:25:00Z — Union Alpha: code default GlowRadius changed from 32
 to 16 (glowconfig.h fallback + struct init); documentation examples
 synced. Not yet built/deployed — dom0 already carries the explicit 16.
2026-09-17T06:35:59Z — Union Alpha: recorded the user-settled GlowRadius=16;
 documentation-only sync, no new dom0 command, build, deploy, or restart.
2026-09-15 — Renamed to Qubes Glow / `qubes-glow` (branding + docs; runtime
IDs unchanged). Container mount recreation + display-metadata build/deploy
pending. Audit corrections: regression count is 26 (not 27); build #15-era
entries below are historical records superseded by builds #17–#22.
2026-09-11T10:5x — Build #18 SHIPPED + user-accepted (sha 7df23937, kwin
62946): per-window B/G toggles + global Alt-masters; LL-027 recorded
(single-writer overrides map, pointer-set pruning, /component/kwin,
synthetic-key trap). 20 regression assertions green.
2026-09-11T09:2x — Build #17 SHIPPED + verified live (sha d66b9009, kwin
60603): ghost square eliminated (size guard; unmanaged Qui-* sources),
kglowsync watchdog (lost-reply re-arm), B/G toggles verified end-to-end
(14-window sweep both directions), 17 regression assertions green.
2026-09-11T08:4x — Build #16 implemented (shortcut split B/G + LL-026
chrome-class exclusion + app-wide kglowsync); ghost-square evidence
documented (docs/research/2026-09-11-ghost-square-qubes-tray-ghosts.md);
GLOBAL-TODO Step 12 registered. Build pending consent.
2026-09-11T08:0x — All-windows glow shipped (#15, sha 6d8886da, kwin
56055): type-based eligibility (LL-023), kglowsync resilient bootstrap
(LL-025), benign loader-noise documented (LL-024). Probes verify2/3/4 in
logs/probe/. Stale v3.4 acceptance item retired (was user-accepted #10).
2026-09-10T21:4xZ — Seamless pass-behind shipped (#14) + user-accepted;
deploy.sh sha-gate added (LL-022); in-repo regression-checks.sh (8 green);
Build State section restored (lost to an atomic edit rollback).
2026-09-10T20:4xZ — LL-020 saga closed: builds #11–#13. Final mechanism
(clip = halo − occluders, CPU subdivision, unclipped 1-arg render,
isDesktop() occluder skip) E2E-verified: positive control 5,214/25,896 px,
occlusion 0 px, 24-cycle burst flat. Docs refreshed (SPECIFICATION
LL-020/021/022; ARCHITECTURE §2.3/§3/§4; HANDBOOK §5 LL-021/022 caveats).
2026-09-10T00:4xZ — Build #10 executed on explicit "build the app": zero-
warning compile (sha 31254894), deployed to dom0 (sha verified), kwin
restarted (activation + LL-011 heal; old kwin 34892→35345→36048 — note kwin
had restarted/rebooted to 34892 between sessions, prior ref 32660 stale),
isEffectLoaded=true, B/T shortcuts re-registered. Awaiting user acceptance
test per §7.
2026-09-09T20:xxZ — LL-019 fix implemented (v3.4) in src/kittyglow.cpp:
occluders rebuilt on every halo paint (120 ms cache deleted), anchored to
the painted kitty window, stackingOrderChanged + windowActivated full-ring
repaint hooks added. SPECIFICATION LL-017/018 registry gap repaired, LL-019
added; HANDBOOK occlusion section synced. Build #10 pending explicit build
consent.
2026-09-09T19:xxZ — Build #9 (LL-018 fix) VERIFIED LIVE: .so mapped in kwin
32660 (5 mappings), B+T shortcuts re-registered, restart log 0 errors,
metadata dir perms fixed to 755/644. Remaining: visual panel test + re-run
panel probe (expect gold ≡ 48 px baseline).
2026-09-09T18:05Z — LL-018 fix coded in `updateOccluders()` (docks/panels
always clip the halo, even when translucent; probe evidence: 1,450 gold px
on panel vs 48 px baseline, static clipping re-verified clean). HANDBOOK
occlusion section synced. Build #9 + KWin restart pending user consent.

### Branding update — 2026-09-17T10:42:04.760106+05:30
Qubes Glow source branding implemented; directory qubes-glow. Runtime IDs and
build #22 deployment unchanged; user accepted menu/notification fixes. Existing
container still references old source path; recreation deferred to next build.
