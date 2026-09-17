<!-- HTML sibling: regenerate ONLY via bash ~/Projects/scripts/generate-docs-html.sh QubesOS/UI-Enhancements/Kwin/qubes-glow/SPECIFICATION.md — Rule 18c left-aligned CSS. Never hand-roll pandoc. -->
# Qubes Glow — Specification

> **Note:** All documentation and code in this project are purely AI-generated.

## 1. Problem Definition & Root Cause

Qubes Glow provides VM-label-aware halos around eligible application windows
and focused/global border controls on the Qubes OS dom0 X11 desktop.
The original kitty-only scope is historical; menus and notification surfaces
must not receive the application's glow. See LL-026/032/033 for constraints.

Root cause of the gap: KWin has no built-in "window-type glow" effect; the only
way to add one is a custom `KWin::Effect` plugin compiled against the exact
KWin ABI (5.27.8) and deployed into dom0 (where the compositor runs).

## 2. Architecture Decisions

### 2.1 Decision Matrix

| Decision | Chosen | Rejected | Rationale |
|----------|--------|----------|-----------|
| Glow technique | GLSL SDF single quad (v3) | 8 stacked rects (v1/v2, historical); single rect / blur shader | Per-side soft falloff in one hardware-blended draw (LL-020) |
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
qubes-glow/
├── src/        effect source (canonical, single-responsibility files)
├── container/   build-env definition (Fedora 37 / KWin 5.27.8)
├── scripts/    build.sh, deploy.sh
├── dist/       build output (gitignored)
├── docs/       research notes
└── logs/       documentary ledgers (committed) + runtime subdirs (gitignored)
```
Git boundary: the whole `qubes-glow/` directory. No cross-project dependencies.
Naming: display name **Qubes Glow**, repository/directory `qubes-glow`.
Keep runtime IDs (`kittyglow`, `kglowsync`), config keys, DBus names, library
basenames and shortcut IDs stable until an explicitly approved migration.
`kwin-effect-qubes-glow` is a proposed future package name, not a shipped package.
Research and rejected naming alternatives: logs/researched-ideas.md (Naming).
Current KF6 filename-derived ID guidance is not a KF5 metadata migration rule.

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
- **LL-016** — KWin property precedence: a loaded window *rule* overrides
  KWin *scripting* property writes. A `noborderrule` Force value in
  kwinrulesrc makes every script `noBorder` write revert at next state
  evaluation (live-proven 2026-09-09: rule removed → script wins).
  Consequence: the kitty borderless state is persisted in `kittyglowrc` and
  applied by script; kwinrulesrc must never again carry `noborderrule` for
  kitty.
- **LL-017** — Front-window occlusion geometry depends on the architecture
  era. Original (full-alpha leak era): clip at `expandedGeometry()`
  (frame + shadow) because the halo painted over translucent shadow
  gradients (penetration artifact, 2026-09-09). **SUPERSEDED 2026-09-10**
  under the LL-020 occluder-clip architecture, user-accepted ("Perfect,
  seamless"): non-dock occluders clip at `frameGeometry()` ONLY — the halo
  paints BENEATH them, so each window's own shadow gradient dims it
  progressively right up to the border (seamless pass-behind); the
  expandedGeometry hard cut left a wallpaper gap between halo-end and the
  occluding border. Docks/panels keep expandedGeometry (LL-018). Recorded
  as `ll017seamless-*` assertions in scripts/regression-checks.sh.
- **LL-018** — Docks/panels are screen chrome and ALWAYS clip the halo, even
  when translucent (adaptive plasma panel; probe: 1,450 gold px on panel vs
  48 px baseline). Genuinely translucent non-dock windows keep the
  bloom-through behavior by design.
- **LL-019** — Occlusion state must never be cached across frames, and every
  stacking change must repaint kitty halos: occluders are rebuilt from
  `stackingOrder()` on EVERY halo paint (a 120 ms snapshot lagged raise/drag
  transitions — one unclipped frame), anchored to the PAINTED kitty window
  (correct occluder set with 2+ kitty windows), and
  `stackingOrderChanged`/`windowActivated` trigger full-ring repaints,
  because an unfocused kitty otherwise never repaints and one bad frame
  persists indefinitely (user repro 2026-09-09: any konsole/firefox window
  placed in front of kitty penetrated, position-independent).
- **LL-020** — The halo ring must NEVER be clipped against the scene paint
  `region` given to `paintWindow`, and the 3-arg hw-clipping `render()` is
  banned. Three-part lesson (full arc 2026-09-10, E2E-verified on build
  #13): (a) `GLVertexBuffer::render(region, mode, hwClipping=true)` sets
  per-rect scissor boxes but the CALLER must enable `GL_SCISSOR_TEST` —
  with the test off (usual pipeline state) the full quad painted everywhere
  (the original glow-penetration leak); (b) forcing the test on made it
  WORSE: KWin 5.27.8's boxes proved degenerate in our paint context
  (Y-flip via GLFramebuffer::currentFramebuffer() height) and clipped the
  halo to NOTHING — a silent total-render failure that mimicked "leak
  fixed" (positive control: 74 px noise floor with kitty fully unoccluded);
  (c) KWin 5.27.8 passes only the window's own frame area as the paint
  region — the ring widening in `prePaintWindow` does NOT propagate into
  it (gate-logging proof: region∩halo == frame exactly). Final mechanism:
  clip = `haloRect − occludedAboveKitty(...)` only (scene region advisory),
  then CPU-subdivide the halo into ONE QUAD PER CLIP RECT drawn unclipped
  via the 1-arg `render(GL_TRIANGLES)`; the SDF is fragment-position-based
  so sub-rects are pixel-identical to the full quad. Also: desktop windows
  (`isDesktop()`) are excluded from occluders — KWin can leave plasma's
  fullscreen desktop window high in `stackingOrder()` after restacks,
  which would empty the clip and silence the halo entirely.
- **LL-021** — `unloadEffect` + `loadEffect` over DBus does NOT pick up a
  rebuilt `.so`: KWin keeps the library mapped, so a live reload
  re-instantiates the OLD code while the new file sits unnoticed on disk
  (2026-09-10: instrumented build reported loaded=true yet produced zero
  journal lines). Every new build requires a full `kwin_x11 --replace`;
  verify BEHAVIOR, never the DBus boolean alone.
- **LL-022** — `scripts/deploy.sh` transfers via three separate dom0 bridge
  calls, each gated by the dom0 password dialog; a cancelled/failed dialog
  is swallowed and the script still exits 0 with the PREVIOUS build left
  on dom0 (2026-09-10: #12b "deployed" while dom0 actually held the older
  #12-debug — cost a full restart cycle on a phantom build). ALWAYS
  sha-verify the deployed artifact (`sha256sum` dom0 vs `dist/`) after
  every deploy, before any restart or verification.
- **LL-024** — `/usr/share/kwin/effects/kittyglow/` holds a metadata-only
  KPackage (no `contents/`). KWin probes EVERY `KWin/Effect` package with
  BOTH loaders: the scripted loader logs `Could not initialize scripted
  effect: "kittyglow"` (no script to run), then the plugin loader loads
  the C++ `.so` fine. The failure line is BENIGN and has fired on every
  start/reconfigure since 2026-09-07 — it is NOT evidence that kglowsync
  or the effect broke. Verify by behavior (journal toggle lines,
  `getCurrentState` via DBus, painted pixels), never by that line.
- **LL-025** — `kwin_x11 --replace` overlaps instances: the OLD kwin holds
  the `org.kde.kittyglow` DBus name until it exits, so the new effect's
  service may not answer while kglowsync boots; a `callDBus` throw at
  script-evaluation time kills the script (`Could not initialize`). Fix:
  kglowsync's `slog` swallows callDBus failures and the bootstrap retries
  every 500 ms (≤60×) until the service answers (assertion
  `ll025-kglowsync-resilient-bootstrap`). C++-side retry was rejected —
  script packages reload on reconfigure, so the JS-side fix needs no
  kwin restart to revive.
- **LL-026** — the Qubes GUI proxy does NOT replicate `_NET_WM_WINDOW_TYPE`
  to dom0: every VM-proxied window (konsole, kitty, firefox, `Qui-*` tray
  widgets — verified 2026-09-11) reaches KWin type-less, and KWin classifies
  type-less windows as normal. Type-based EffectWindow predicates
  (`isDialog/isPopupWindow/…`) therefore NEVER match VM windows — they only
  ever worked for dom0-native apps (which is why the dom0 `kdialog` test
  passed while tray icons and the plasma start menu glowed). WM_CLASS is the
  one property that survives the proxy: chrome is excluded by class
  (`plasmashell`, `Qui-*`, `xembedsniproxy`, `krunner`) in both
  glowtargets.h and the kglowsync script (mirrored predicate). The
  "ghost square" (2026-09-11) was the 22 px halo of four stale 16×16
  `Qui-*` windows pinned at (0,0) — full evidence in
  docs/research/2026-09-11-ghost-square-qubes-tray-ghosts.md. Same
  directive's shortcut split: Meta+Shift+B = class-wide borderless
  (toggleNoBorder + requestApply), Meta+Shift+G = glow master switch
  (Key_G = 0x47; Meta+Shift+G = 0x12000047). Residual: VM-internal dialogs
  still glow — their type hints die in the proxy (WM_TRANSIENT_FOR
  replication unverified).
  **Supplement (build #17, 2026-09-11):** the corner "ghost square" itself
  survived the class exclusion — the Qui-* tray-source windows are
  override-redirect (KWin UNMANAGED: absent from clientList, no WM_CLASS
  reachable), so class checks cannot see them. Fixed with a minimum
  frame-size guard (sub-48 px = icon, never haloed) mirrored in kglowsync.
  Corner gold 456 → 0 verified live. Same build: the kglowsync bootstrap
  re-arms every 5 s until the first service reply (a reply can be silently
  LOST when the request lands on the dying kwin during --replace overlap —
  no throw, no callback, script wedged with desired=null; seen live on
  build #16's first restart).
- **LL-027** — per-window toggles + global masters (build #18, user
  directive 2026-09-11: "toggling glow and titlebar and border should be
  for the focused window not globally"). Design invariants:
  (1) Launch defaults stay global and persisted — glow ON, borderless
  everywhere (bootstrap sweep); per-window overrides are RUNTIME-ONLY and
  reset at every kwin restart, by design.
  (2) The script remains the SINGLE noBorder writer: focused-window border
  flips travel the kittytoggle `nextWindowOp` DBus channel; the script
  resolves `workspace.activeClient` itself (no windowId crosses the bus)
  and shields the flipped window from the 400 ms safety-net sweep via a
  windowId-keyed overrides map — otherwise the sweep clobbers the override
  within 400 ms (the map is the fix, verified live 2026-09-11).
  (3) Effect-side glow overrides are a pointer set — MUST be pruned on
  `windowDeleted` (glowfocus.h) or the paint path dereferences a dangling
  pointer.
  (4) Effect shortcut actions register under the kglobalaccel component
  **kwin** (`/component/kwin`), NOT `/component/kittyglow` — query there
  when auditing live bindings.
  (5) **Synthetic-key trap:** `xdotool key super+shift+b` (XTEST) failed to
  trigger a live, correctly-registered binding (file + live daemon both
  correct) after a kwin --replace, while physical keys fired instantly —
  verify shortcuts PHYSICALLY before debugging code; the journal names
  every toggle's scope (`focused`/`global`) so the outcome is provable.
- Robust ownership query on
  kglobalaccel is the NO-ARG `allShortcutInfos` on `/component/kwin`;
  keyed queries (`getGlobalShortcutsByKey`, `action()`) require exactly
  encoded key ints (LL-012).
- **LL-028** — ONE scale boundary, at the vertex upload (re-audit 2
  supersedes audit M2, 2026-09-11): in KWin 5.27.8 every effect-facing
  region — damage in prePaintWindow, addRepaint, occluders, the halo rect,
  and PaintData translation — is LOGICAL px (Scene::addRepaint intersects
  the logical viewport unscaled, scene.cpp:92; the GL scissor converts via
  mapToRenderTarget internally, itemrenderer_opengl.cpp:331;
  PaintData::toMatrix applies * deviceScale internally,
  kwineffects.cpp:208). Only the projection/vertex layer is device px
  (ortho box = rect * scale, itemrenderer.cpp:45). Audit M2's "widen damage
  in device px" was wrong-space — it over-widened (benign) at s > 1, while
  occluders built ×s genuinely double-scaled (over-clip). Rule: never
  multiply effect-facing geometry by renderTargetScale; scale exactly
  once, where vertices are built.
- **LL-030** — rollback completeness (re-audit 2 M5, 2026-09-11): an escape
  hatch must strip every component the deploy installs, not only the ones
  from the era it was written in — kittyglow-rollback.sh removed the
  retired kitty-toggle-border but not kglowsync, which then kept enforcing
  the last commanded borderless state on newly spawned windows after a
  "clean" rollback. Rule: new deployed component ⇒ update the rollback
  script in the same change.
- **LL-031** — audit claims are hypotheses until adjudicated against
  pinned source (re-audit 2, 2026-09-11): the deep audit's M2 coordinate-
  space claim was plausible, codified as a lesson, AND enshrined as a
  regression assertion — and was wrong; only re-derivation from the pinned
  KWin source caught it. Rule: every "the framework does X" claim gets a
  file:line citation from the pinned source before it becomes code, lesson,
  or assertion.
- **LL-032** — per-VM Qubes label hue (build #21, 2026-09-11): qubes-guid
  sets _QUBES_LABEL_COLOR (CARDINAL, plain 0x00RRGGBB) on every VM-proxied
  window — xprop evidence on Dev-General: 15586304 = 0x00EDD400 = its
  yellow qvm-ls label. The effect reads it via
  EffectsHandler::xcbConnection() (kwineffects.h:1280) +
  EffectWindow::windowId() (kwineffects.h:2684), ONE synchronous read per
  window LIFETIME, cached (misses cached too — dom0-native windows never
  re-read), pruned on windowDeleted. Active/inactive stays an OPACITY
  distinction (60%/30%) applied over the label hue; a relabeled VM keeps
  its cached hue until its windows reopen. LabelColor=false (kwinrc
  [Effect-kittyglow]) restores the configured gold. Dom0-native windows
  always use the configured colors — they carry no such atom.
- **LL-033** — override-redirect chrome escapes deny-list eligibility
  (build #22, 2026-09-15): Qt QMenu drop-downs, combo popups and tooltips are
  override-redirect surfaces. KWin wraps them as unmanaged EffectWindows, they
  set NO _NET_WM_WINDOW_TYPE (so isPopupWindow/isMenu/isComboBox never match),
  and they inherit the parent's WM_CLASS (so class denial misses them). The
  only reliable gate is `EffectWindow::isManaged()` (kwineffects.h:2588,
  "whether it's managed or override-redirect"; managed = window->isClient(),
  effects.cpp:2003, captured at construction so popups stay classified after
  Deleted-reparent). EffectWindow has NO isUnmanaged(). Without that gate every
  open menu draws its own halo (user report: drop-down glow + flickering
  bottom-edge outline where the list overruns the window border). The 48 px
  size guard was doing this structural work by accident for icon surfaces;
  isManaged() is the principled first-order denial.
- **LL-034** — build container package drift (2026-09-17, build #23): the
  pre-rename image `dom0-replica-fed37-img` carried a MANUALLY installed
  `kf5-kglobalaccel-devel`; recreating the container from the stale image
  silently dropped it and cmake failed ("Could NOT find KF5 (missing:
  GlobalAccel)"). Rule: every package the build needs MUST be in
  `container/setup-build-container.sh` dnf list (now includes it), and any
  manual in-container install must be followed by `podman commit CTR IMG`
  (done: image 657fec9e). Also fixed: setup script exec bit (build.sh
  invokes it directly; 644 → 755, exit 126).
- **LL-029** — per-action autorepeat gates (audit M3, 2026-09-11): a
  single shared `QElapsedTimer` gate across all toggle actions silently
  dropped CROSS-toggle presses (B then G within 220 ms lost G — a
  heuristic error a user feels but cannot name). Every action owns its
  gate. Corollary: any "debounce" state shared across semantically
  independent actions is a latent action-eater.

## 9. Official References

- KWin effect development: https://invent.kde.org/plasma/kwin/-/wikis/Development
- `KPluginMetaData` service-type parsing: KF5 `kpluginmetadata.h` (Fedora 37, 5.108)
- KWin 5.27.8 effect plugin API (`kwineffects.h`, `kwinglutils.h`) — pinned via the
  Fedora-37 replica container so the ABI never drifts from dom0.

## 10. Historical Scope

The MVP targeted kitty with a layered-alpha halo. Current application-window
eligibility and rendering are described in ARCHITECTURE.md; usage is in
HANDBOOK.md. Historical identifiers remain for compatibility, not scope.
