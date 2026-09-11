<!-- HTML sibling: regenerate ONLY via bash ~/Projects/scripts/generate-docs-html.sh QubesOS/UI-Enhancements/Kwin/kitty-glow/PROJECT_CONTEXT.md — Rule 18c left-aligned CSS. Never hand-roll pandoc. -->
# kitty-glow — Project Context

> **Note:** All documentation and code in this project are purely AI-generated.

## 1. Project Overview
Custom **KWin (KDE Plasma 5.27.8) compositor effect** that paints a soft yellow
halo around `kitty` terminal windows. Platform: Qubes OS dom0 X11 compositor.
Language: C++20 / Qt plugin. Purpose: make kitty windows visually distinct.

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
`KittyGlowEffect` tracks kitty windows via `windowClass()` and draws ONE
hardware-blended triangle-fan quad around the frame rect; a GLSL SDF shader
(`glowshader.cpp`) computes per-side soft falloff (v3 — replaced the v2
8-stacked-rect approach). Thickness/radius/active-inactive colors come from
kwinrc `[Effect-kittyglow]`, live-reloadable via `reconfigureEffect`. Damage
is widened in `prePaintWindow` + repaint hooks on geometry/minimize so the
halo never smears. Full design: ARCHITECTURE.md.

## 4. Database / Data Layer
None. Stateless effect; no persistence beyond kwinrc enable flag.

## 5. Build State
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
  `org.kde.kwin.Effects.loadEffect` → `true`. Live kwin PID 15974
  (`--crashes 1` — recovered from the test-time crash; stable since).
- Parity at rest: `rules=kitty-borderless`, `noborder=true` (borderless).

## 6. Active Features
- **All-windows glow (build #15)**: every normal application window gets
  the halo; dialogs, notifications, OSD, popup menus, tooltips, splash
  and utility windows are excluded (window-type predicates, LL-023).
- 8-layer translucent yellow halo (RGB 255,221,0, margin 22 px, alpha 0→70).
- Fullscreen suppression; OpenGL-compositing guard.
- Live activation over DBus (`/Effects` loadEffect) — no compositor restart.
- kitty borderless window rule in `kwinrulesrc` (`[kitty-borderless]`,
  noborder Force=2, wmclassmatch RegExp=3, listed under `[General] rules=`).
- **Meta+Shift+B** → effect-registered `Toggle Kitty Borderless`: persists
  the state to `kittyglowrc` via `kittyglowstate.cpp`; the kglowsync script
  (bootstrap + poll + sweep) applies `noBorder` live to all kitty windows.
- **Meta+Shift+T** → native `Window No Border` (per-focused-window toggle);
  rebound from Meta+Shift+B, dead `kitty-toggle-border` entry deleted.
- Shortcut registration self-heals whenever kwin starts after kglobalaccel
  (incl. crash auto-restarts); manual healing = restart kwin_x11 last (LL-011).

## 7. Pending / In-Progress
- User acceptance of the all-windows glow (build #15): subjective look of
  halos on non-kitty apps (konsole, firefox) + notification/menu exclusion
  aesthetics. Programmatic verification already passed (kitty 1,287 px;
  dialog 112 = noise; notification dock-strip 0).

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
- No per-window toggle yet (always-on for kitty). See ROADMAP.
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
