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
| `src/kittyglow.cpp` | `KWin::Effect` subclass; quad + shortcut + repaint hooks | 274 |
| `src/kittyborderrule.cpp/.h` | Content-based kitty rule toggle in kwinrulesrc (rename-immune) | 70 |
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
- **Build #7 (v3.4, Step C)** built zero-warning, deployed + E2E-verified.
  Artifact: `dist/kittyglow.so`, sha256 prefix `badf8df70b6a` (metadata.json
  `3a3f66bc…`). Adds `kittyborderrule.cpp` — content-based (Description /
  wmclass) lookup of the ACTIVE kitty rule, inert-rule self-heal via
  `[General] rules=`, and `KSharedConfig::reparseConfiguration()` per toggle
  (LL-014). Group-rename immunity PROVEN E2E: active group renamed to `[3]`
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
- **Meta+Shift+B** → effect-registered `Toggle Kitty Borderless`: flips the
  persistent kitty window rule (content-matched via `kittyborderrule.cpp`,
  survives KWin group renames) and loopback-reconfigures KWin.
- **Meta+Shift+T** → native `Window No Border` (per-focused-window toggle);
  rebound from Meta+Shift+B, dead `kitty-toggle-border` entry deleted.
- Shortcut registration self-heals whenever kwin starts after kglobalaccel
  (incl. crash auto-restarts); manual healing = restart kwin_x11 last (LL-011).

## 7. Pending / In-Progress
- Visual verification of v3.3 with a real kitty window: seamless minimize
  (halo tracks shrink), halo clipping under overlapping windows during drags,
  single-fire B per press, live T.

## 8. Known Issues
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
2026-09-09T~04:35Z — Step C complete (build #7, badf8df7…): rename-immune
content-based rule toggle, reparse hardening, E2E + rename-immunity verified;
Sep 08 shortcut-registration outage root-caused (LL-011 boot race) and healed
(kwin restart after daemon); kwinrulesrc normalized; LL-011..015 recorded.
