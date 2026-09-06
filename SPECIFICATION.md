<!-- HTML sibling: regenerate ONLY via bash ~/Projects/scripts/generate-docs-html.sh QubesOS/UI-Enhancements/Kwin/kitty-glow/SPECIFICATION.md — Rule 18c left-aligned CSS. Never hand-roll pandoc. -->
# kitty-glow — Specification

> **Note:** All documentation and code in this project are purely AI-generated.

## 1. Problem Definition & Root Cause

kitty is the primary terminal in this Qubes OS environment. On a shared X11
desktop, kitty windows are visually indistinguishable from other VM windows.
**Fix:** a KWin (KDE Plasma 5.27) compositor effect that paints a soft yellow
halo around every kitty window, making it instantly identifiable.

Root cause of the gap: KWin has no built-in "window-type glow" effect; the only
way to add one is a custom `KWin::Effect` plugin compiled against the exact
KWin ABI (5.27.8) and deployed into dom0 (where the compositor runs).

## 2. Architecture Decisions

### 2.1 Decision Matrix

| Decision | Chosen | Rejected | Rationale |
|----------|--------|----------|-----------|
| Glow technique | 8 stacked translucent GL rects | Single rect / blur shader | Cheap, no shader code, soft falloff via alpha ramp |
| Where it runs | dom0 KWin plugin | AppVM-side (kitty config) | The compositor owns window framing; kitty cannot draw outside its own content |
| Build host | Fedora-37 replica container | Build on dom0 directly | dom0 has no toolchain; container matches dom0's KWin 5.27.8 ABI exactly |
| Transfer to dom0 | qvm-run base64 pipe | qvm-copy file RPC | Filecopy RPC into dom0 was refused; qvm-run (dom0→VM exec) works |
| Activation | kwinrc enable + KWin restart | Live D-Bus loadEffect | D-Bus session bus unreachable from the harness shell |

### 2.2 Rejected Alternatives

- **picom rounded/glow:** picom was uninstalled; KWin owns compositing on this desktop.
- **kitty background tint:** only tints in-client content, not the frame/halo.
- **KWin script (JS):** cannot draw arbitrary GL geometry; C++ plugin required.

## 3. Upstream Conformance Rules (KWin 5.27 effect plugin)

- Plugin class subclasses `KWin::Effect`, overrides `paintWindow()`.
- Metadata (embedded JSON) must declare `KPlugin.Id` and
  `ServiceTypes: ["KWin/Effect"]` (read by `KPluginMetaData::serviceTypes()`).
- Exported via `KWIN_EFFECT_FACTORY(ClassName, "metadata.json")`.
- `kwineffects.h` uses `std::span` → **C++20 required**.
- Draw with `ShaderManager::pushShader(KWin::ShaderTrait::UniformColor)`,
  `GLVertexBuffer::streamingBuffer()`, `setData(4,2,verts,nullptr)`,
  `render(GL_TRIANGLE_STRIP)`.

## 4. Project Structure Charter

```
kitty-glow/
├── src/        effect source (canonical, single-responsibility files)
├── container/   build-env definition (Fedora 37 / KWin 5.27.8)
├── scripts/    build.sh, deploy.sh
├── dist/       build output (gitignored)
├── docs/       research notes
└── logs/       documentary ledgers (committed) + runtime subdirs (gitignored)
```
Git boundary: the whole `kitty-glow/` directory. No cross-project dependencies.

## 5. Build & Packaging Specification

- Toolchain: `cmake` + `gcc-c++` (C++20) inside `dom0-replica-fed37` (Fedora 37,
  `kwin-devel-5.27.8`, `kf5-*-devel`, `libepoxy-devel`, `qt5-qtbase-devel`).
- Build: `cmake -B build && cmake --build build` → `kittyglow.so` (47 600 B).
- Install (dom0): `.so` → `/usr/lib64/qt5/plugins/kwin/effects/`,
  `metadata.json` → `/usr/share/kwin/effects/kittyglow/`.
- Enable: `kwriteconfig5 --file kwinrc --group Plugins --key kittyglowEnabled true`.

## 6. Code Standards

- C++20; one effect per file; `kittyglow.cpp` ≤ 150 lines (currently ~85).
- Descriptive names; no hardcoded magic numbers without a named const.
- Markdown docs carry the Rule 18 pointer + AI disclaimer.

## 7. Quality Gates (every change must pass)

- [ ] `cmake` configure + build exit 0, **zero warnings** (Rule 5).
- [ ] `ldd kittyglow.so` resolves all libraries (no `not found`).
- [ ] `nm -D` shows `KittyGlowEffect::paintWindow` + factory symbols.
- [ ] Installed `.so`/`metadata.json` are mode `644` (readable by KWin user).
- [ ] Embedded metadata contains `ServiceTypes: ["KWin/Effect"]`.
- [ ] `sha256sum` of built `.so` recorded in CHANGELOG and matches deployment.

## 8. Lessons Learned Registry

- **LL-001** — KWin 5.27 `kwineffects.h` uses `std::span`; build **must** be C++20.
- **LL-002** — `GLVertexBuffer::setData` needs all 4 args (`texcoords=nullptr`);
  use `ShaderTrait::UniformColor` + `render(GL_TRIANGLE_STRIP)`.
- **LL-003** — `qvm-copy` *into* dom0 was refused; transfer via `qvm-run` base64 pipe.
- **LL-004** — Plugin files installed as `600` by `sudo` redirect silently break
  KWin load; must `chmod 644`.
- **LL-005** — Built-in KWin effects are statically linked into `libkwin.so`;
  custom effects are separate `.so` files in `kwin/effects/`.

## 9. Official References

- KWin effect development: https://invent.kde.org/plasma/kwin/-/wikis/Development
- `KPluginMetaData` service-type parsing: KF5 `kpluginmetadata.h` (Fedora 37, 5.108)
- KWin 5.27.8 effect plugin API (`kwineffects.h`, `kwinglutils.h`) — pinned via the
  Fedora-37 replica container so the ABI never drifts from dom0.

## 10. Appendix

MVP scope: layered-alpha halo only. Window detection is by `windowClass()`
containing "kitty" (case-insensitive); fullscreen windows are skipped. See
ARCHITECTURE.md for the GL draw path and HANDBOOK.md for tuning.
