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
| `src/kittyglow.cpp` | `KWin::Effect` subclass; GL halo drawing | 95 |
| `src/CMakeLists.txt` | Plugin build (C++20, KWin links) | 40 |
| `src/kittyglow.json` | Effect metadata (Id, ServiceTypes) — embedded in `.so` | 15 |
| `container/setup-build-container.sh` | Fedora-37 / KWin 5.27.8 build env | 45 |
| `scripts/build.sh` | Compile in container → `dist/` | 30 |
| `scripts/deploy.sh` | Copy into dom0 + enable in kwinrc | 30 |

## 3. Architecture Summary
`KittyGlowEffect` tracks kitty windows via `windowAdded`/`windowDeleted` into a
`QSet`. Each frame, `paintWindow()` draws 8 alpha-ramped yellow rects (behind the
window) using `ShaderManager::UniformColor` + `GLVertexBuffer`, then calls the
real `effects->paintWindow()`. Full design: ARCHITECTURE.md.

## 4. Database / Data Layer
None. Stateless effect; no persistence beyond kwinrc enable flag.

## 5. Build State
- Build #1 complete. Artifact: `kittyglow.so` (47 600 B), sha `89e8513b…`.
- Command: `scripts/build.sh` (cmake + make inside `dom0-replica-fed37`).
- Deployed to dom0: `/usr/lib64/qt5/plugins/kwin/effects/kittyglow.so`
  and `/usr/share/kwin/effects/kittyglow/metadata.json` (both `644`).

## 6. Active Features
- Automatic kitty-window detection (`windowClass()` contains "kitty").
- 8-layer translucent yellow halo (RGB 255,221,0, margin 22 px, alpha 0→70).
- Fullscreen suppression; OpenGL-compositing guard.

## 7. Pending / In-Progress
- **Activate:** KWin not yet restarted (user deferred) → effect not yet visible.
- Visual verification after activation.
- Tuning of halo parameters.

## 8. Known Issues
- ~~Build container (`dom0-replica-fed37`) still mounts legacy `/home/user/kitty-glow`~~
  **RESOLVED 2026-09-06T23:50:19Z:** container re-created mounting this project's
  `src/` at `/src` (committed image `dom0-replica-fed37-img` preserves toolchain).
- No per-window toggle yet (always-on for kitty). See ROADMAP.

## 9. Connected Targets
- **dom0** (X11 `:0`, KWin 5.27.8) — deployment + activation target.
- **Dev-General** AppVM — hosts source, build container, `dom0` helper.
- **dom0-replica-fed37** Podman container — Fedora 37 build env (KWin 5.27.8 devel).

## 10. Last Updated
2026-09-06T23:50:19Z — build container re-pointed to project `src/` (item #2).
Effect built, installed in dom0, enabled in kwinrc; activation deferred pending
user approval.
