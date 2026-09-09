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
| `src/kittyglow.cpp` | `KWin::Effect` subclass; quad + shortcut + repaint hooks | 169 |
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
- **Build #6 (v3.3)** built zero-warning, deployed + live-verified. Artifact:
  `dist/kittyglow.so`, sha256 prefix `097b3e24bffe` (metadata.json
  `3a3f66bc…`). v3 SDF renderer + animation-transform tracking + opacity
  fade + occlusion-clipped halo + seamless minimize (no isMinimized paint
  guard, isKittyWindow filter retained) + autorepeat-gated Meta+Shift+B.
  Build #5 (d445c704…) was defective — it accidentally dropped the
  isKittyWindow filter (halo on every window); superseded within the round.
- Command: `scripts/build.sh` (cmake + make inside `dom0-replica-fed37`).
- Deployed to dom0: `/usr/lib64/qt5/plugins/kwin/effects/plugins/kittyglow.so`
  and `/usr/share/kwin/effects/kittyglow/metadata.json` (both `644`).
- Enabled in `kwinrc` `[Plugins] kittyglowEnabled=true`; loaded at runtime via
  `org.kde.kwin.Effects.loadEffect` → `true`, survived full `reconfigure`.
- Live kwin PID 211257 (`--replace`, clean — no `--crashes`); deployed dom0
  sha256 matches `dist/` byte-for-byte.
- Shortcut config repaired: `kglobalshortcutsrc` `Window No Border` active
  field restored (`Meta+Shift+T,Meta+Shift+T,`), `plasma-kglobalaccel`
  restarted (single daemon, PID 210789), `kwinrulesrc` noborder parity
  restored to `true`.

## 6. Active Features
- Automatic kitty-window detection (`windowClass()` contains "kitty").
- 8-layer translucent yellow halo (RGB 255,221,0, margin 22 px, alpha 0→70).
- Fullscreen suppression; OpenGL-compositing guard.
- Live activation over DBus (`/Effects` loadEffect) — no compositor restart.
- kitty borderless window rule in `kwinrulesrc` (`[kitty-borderless]`,
  noborder Force=2, wmclassmatch RegExp=3, listed under `[General] rules=`).
- **Meta+Shift+B** → effect-registered `Toggle Kitty Borderless`: flips the
  persistent `[kitty-borderless]` rule and loopback-reconfigures KWin.
- **Meta+Shift+T** → native `Window No Border` (per-focused-window toggle);
  rebound from Meta+Shift+B, dead `kitty-toggle-border` entry deleted.

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
2026-09-08T00:02:28+05:30 — v3.3 completion (build #6, 097b3e24…):
occlusion-clipped halo, seamless minimize, autorepeat-gated B, T
reactivation, noborder parity restore, single-daemon 210789, kwin --replace
PID 211257. Build #5 regression (missing isKittyWindow filter → halo on all
windows) user-reported and fixed same round; lesson LL-010 recorded.
