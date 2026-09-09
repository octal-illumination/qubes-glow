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
- Automatic kitty-window detection (`windowClass()` contains "kitty").
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
- v3.4 (LL-019 fix) user acceptance test after Build #10 deploy + kwin
  restart: place konsole/firefox window over unfocused kitty → NO glow
  penetration, including right after raise; halo recolors on focus change;
  minimize/restore tracking + B/T single-fire unaffected; panel clipping
  (LL-018) still clean.

## 8. Known Issues
- Halo penetrates any window placed in front of kitty (LL-019): the 120 ms
  occluder snapshot lagged restacks and an unfocused kitty never repainted
  the one unclipped frame away (repro: konsole AND firefox, position-
  independent; back windows irrelevant). **Fix implemented (v3.4): occluders
  rebuilt on every halo paint, anchored to painted kitty, plus
  stackingOrderChanged/windowActivated full-ring repaint hooks. Build #10
  compile + deploy + kwin restart pending explicit build consent.**
- Halo paints over translucent plasma panel (LL-018): panel (opacity < 0.99)
  was never an occluder, so halo repaints left gold on panel chrome (probe:
  1,450 px vs 48 px baseline). **Fix shipped in Build #9, live in kwin 32660;
  v3.4 keeps the docks-always-clip rule in the per-paint occluder walk.
  Visual panel acceptance test still pending.**
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
