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

- C++20; one effect per file; borderless-state persistence extracted to
  `kittyglowstate.cpp` (single responsibility: kittyglowrc state store).
  `kittyborderrule.cpp` (kwinrulesrc rule toggle) is RETIRED — see LL-016:
- `kittyglow.cpp` is 274 lines — over the 150 budget. The overage is the
  cohesive GL render path (LL-recorded accepted tech debt, 2026-09-09);
  do NOT add logic to it without extracting first.
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
- **LL-006** — Effect plugin metadata MUST carry `"KPackageStructure": "KWin/Effect"`
  plus `X-KDE-Library`/`X-KDE-PluginKeyword`; without them `loadEffect` returns
  false even though the `.so` dlopens fine. Gate every deploy on the
  `src/test_create.cpp` harness (instance + factory cast + isSupported +
  createEffect all non-null) inside the build container.
- **LL-007** — Window rules live in `kwinrulesrc` (NOT `kwinrc`); a rule group is
  a no-op unless its name is listed under `[General] rules=`. Enum semantics
  (rules.h): rule type 0=Unused 1=DontAffect 2=Force 3=Apply 4=Remember
  5=ApplyNow 6=ForceTemporarily; match 0=Unimportant 1=Exact 2=Substring
  3=RegExp. Hot-reload via `org.kde.KWin.reconfigure` → `RuleBook::load()`.
- **LL-008** — Under dom0 qrexec, `sudo bash -s` shares stdin with inner
  `qvm-run` calls: the first `qvm-run` consumes the rest of the piped script.
  Decode scripts to a file and run the file instead
  (`base64 -d > /tmp/x.sh && sudo bash /tmp/x.sh`).
- **LL-009** — KWin 5.27 DBus: main interface is `org.kde.KWin` at `/KWin`
  (`reconfigure`, `supportInformation`); effects at `/Effects` with interface
  `org.kde.kwin.Effects` (`loadEffect`, `isEffectLoaded`, `unloadEffect` —
  there is NO `loadedEffects` method). `org.kde.kwin.KWin` does not exist.
  custom effects are separate `.so` files in `kwin/effects/`.
- **LL-010** — Compound guard lines: when an edit replaces a guard line with
  a comment, the surviving predicate MUST be restated — dropping
  `isKittyWindow()` from `paintWindow()` (build #5) drew the halo around
  every window incl. the panel. After any guard edit: re-read the function
  and `grep -n "isKittyWindow\|isMinimized"` before building. Never replace
  a compound guard line with prose-only text.
- **LL-011** — Shortcut registration is ORDER-dependent: kwin_x11 starting
  BEFORE `plasma-kglobalaccel` loses all its global shortcuts for the whole
  session (Sep 08 boot). Restarting kglobalaccel alone does NOT heal (kwin
  never re-registers); restarting kwin AFTER the daemon does. Re-validated
  by a real crash: kwin auto-restart with the daemon already up re-registered
  every shortcut.
- **LL-012** — kglobalaccel key ints use Qt modifier bits: SHIFT=0x02000000,
  CTRL=0x04000000, ALT=0x08000000, **META=0x10000000**. Meta+Shift+B =
  0x12000042 = 301989954; Meta+Shift+T = 0x12000054 = 301989972. Probes with
  wrong modifier bits silently address the wrong keys (Ctrl+Shift instead of
  Meta+Shift).
- **LL-013** — xdotool: `meta` modifier = Alt (Mod1); the Super/Win key is
  `super`. E2E shortcut tests must use `xdotool key super+shift+b`.
- **LL-014** — `KSharedConfig::openConfig()` caches the parsed file
  PROCESS-wide. Effects toggling files that KWin's RuleBook rewrites must
  call `reparseConfiguration()` per toggle or they act on a stale snapshot;
  `sync()` from a stale snapshot can resurrect groups KWin renamed/removed
  (and once crashed kwin during testing).
- **LL-015** — Rule groups in `kwinrulesrc` may carry UUID or numeric names
  (KWin re-save). Identify rules by CONTENT (`Description`/`wmclass`), never
  by group name (once relevant to `kittyborderrule.cpp`, now retired).
  **LL-016** — KWin property precedence: a loaded window *rule* overrides
  KWin *scripting* property writes. A `noborderrule` Force value in
  kwinrulesrc makes every script `noBorder` write revert at next state
  evaluation (live-proven 2026-09-09: rule removed → script wins).
  Consequence: the kitty borderless state is persisted in `kittyglowrc` and
  applied by script; kwinrulesrc must never again carry `noborderrule` for
  kitty. Robust ownership query on
  kglobalaccel is the NO-ARG `allShortcutInfos` on `/component/kwin`;
  keyed queries (`getGlobalShortcutsByKey`, `action()`) require exactly
  encoded key ints (LL-012).

## 9. Official References

- KWin effect development: https://invent.kde.org/plasma/kwin/-/wikis/Development
- `KPluginMetaData` service-type parsing: KF5 `kpluginmetadata.h` (Fedora 37, 5.108)
- KWin 5.27.8 effect plugin API (`kwineffects.h`, `kwinglutils.h`) — pinned via the
  Fedora-37 replica container so the ABI never drifts from dom0.

## 10. Appendix

MVP scope: layered-alpha halo only. Window detection is by `windowClass()`
containing "kitty" (case-insensitive); fullscreen windows are skipped. See
ARCHITECTURE.md for the GL draw path and HANDBOOK.md for tuning.
