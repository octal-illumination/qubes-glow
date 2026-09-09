# CHANGELOG

## 2026-09-06T23:50:19Z — Re-pointed build container to project src
- Ran `container/setup-build-container.sh`: committed live container to image
  `dom0-replica-fed37-img` (preserves the KWin 5.27.8 toolchain), then recreated
  `dom0-replica-fed37` mounting `src/` at `/src`.
- Project now fully self-consistent: `scripts/build.sh` compiles the canonical
  source directly. Legacy `/home/user/kitty-glow` no longer used by the build.

## 2026-09-06T23:40:28Z — Project created; effect imported & documented
- Created `~/Projects/QubesOS/UI-Enhancements/Kwin/kitty-glow/` with full
  AGENTS.md-compliant structure (`src/`, `container/`, `scripts/`, `docs/`, `logs/`).
- Imported canonical source: `src/kittyglow.cpp`, `src/CMakeLists.txt`,
  `src/kittyglow.json` (checksums match the prior working copy).
- Added `container/setup-build-container.sh` (Fedora-37 / KWin 5.27.8) and
  `scripts/build.sh` + `scripts/deploy.sh`.
- Authored `SPECIFICATION.md`, `ARCHITECTURE.md`, `HANDBOOK.md`,
  `PROJECT_CONTEXT.md`, `ROADMAP.md` (each with Rule 18 HTML pointer) + `README.md`.
- Carried forward from prior session: `kittyglow.so` (sha `89e8513b…`, 47 600 B)
  already deployed to dom0 plugin paths and enabled in kwinrc; KWin activation
  (restart) deferred per user.
- See SPECIFICATION.md §8 (Lessons Learned) and HANDBOOK.md for build/deploy/activate.

## 2026-09-06T23:52:44+00:00 — Removed legacy working directory
- Deleted `/home/user/kitty-glow/` (redundant pre-project working copy). Project
  `src/` is now the sole canonical source. `kitty-glow-out/` (build artifacts) kept.
- Container mount and all project files unaffected.

## 2026-09-06T23:53:14+00:00 — Removed redundant build-artifact dir
- Deleted `/home/user/kitty-glow-out/` (copy of compiled `kittyglow.so` + `kittyglow.json`).
  Canonical artifacts live in `dist/`; build container keeps its own `/src/build`.
- No project file references the removed dir.
- 2026-09-07T04:45:58Z Added scripts/kittyglow-rollback.sh (user-requested one-command rollback; hard copy to be installed at /usr/local/bin/kittyglow-rollback in dom0)
- 2026-09-07T04:59:47Z Rollback widened: covers kitty-toggle-border KWin script (root cause of global decoration loss) in addition to kittyglow effect
- 2026-09-07T05:15:53Z Rollback deployed to dom0 /usr/local/bin/kittyglow-rollback and executed: both kitty artifacts removed, KWin 197241 running clean (plasma-session env)

## 2026-09-07T06:30:03Z
- **v2 rollout fully implemented and live-verified** (user: "Let's do v2, implement").
  - Rebuilt `kittyglow.so` (46 728 B, sha e2e9cef0b0423bf5…) with corrected
    metadata (`KPackageStructure: KWin/Effect`, LL-006); deployed to the real
    KWin 5.27 scan dir `/usr/lib64/qt5/plugins/kwin/effects/plugins/`.
  - Enabled in kwinrc; loaded live over DBus (`org.kde.kwin.Effects.loadEffect`
    → true), verified `isEffectLoaded=true` and survival across full reconfigure.
  - kitty borderless rule registered in `kwinrulesrc` (noborder Force, wmclass
    RegExp, listed under `[General] rules=`) and hot-reloaded via reconfigure.
  - `scripts/deploy.sh`: fixed stale SO_DIR (now `effects/plugins`), replaced
    restart-based activation with verified DBus procedure.
  - Docs updated: HANDBOOK §4–§7 rewritten + renumbered, PROJECT_CONTEXT §5/§6/§7/§10
    refreshed, SPECIFICATION LL-006…LL-009 added. HTML siblings regenerated.

## 2026-09-07T07:08:41Z — Recovery round (user-approved "implement")
- dom0 snapshots: kglobalshortcutsrc / kwinrulesrc / kwinrc → /tmp/*.bak-20260907-123452.
- Meta+Shift+B bound to native KWin "Window No Border" (Toggle Window Titlebar
  and Frame); plasma-kglobalaccel unit restarted (no reread API in 5.27);
  kglobalaccel rewrote the entry itself (= live acceptance) at line 138.
- kwinrulesrc deduped: removed duplicate group [1]; [General] rules=kitty-borderless
  now points at the named group (content identical, verified pre-merge).
- Disabled v1 script residue ~/.local/share/kwin/scripts/kwin_script/ removed;
  stale kwin_scriptEnabled=false flag deleted from kwinrc.
- KWin rehomed out of the user fish shell: new PID 203470 via setsid+sudo
  wrapper (PPID chain: sudo 203468 → PID 1); single instance; kittyglow
  auto-loaded (isEffectLoaded=true); windows re-managed without errors.
- plasmashell restarted (PID 203516) — desktop shell (panel) restored.
- Findings recorded: chromium UUID rule groups unlisted in rules= (pre-existing
  inert); orphan kglobalaccel5 167889 no longer bus-relevant; kitty binary not
  present in dom0 (AppVM windows appear in dom0 with wmclass=kitty).

## 2026-09-07T22:02:39+05:30 — v3 deploy + shortcut rebind round (approved: "implement changes")
- Build: zero-warning; kittyglow.so sha256 b48e521c5a32eda4b32a5fb37ae3cf902b8deaf9dbe578123505df8602a02acb
- kglobalshortcutsrc: Window No Border -> Meta+Shift+T; dead kitty-toggle-border deleted; backup /tmp/kglobalshortcutsrc.v3.bak-20260907-215900
- Deployed v3 to dom0 (/usr/lib64/qt5/plugins/kwin/effects/plugins/kittyglow.so + /usr/share/kwin/effects/kittyglow/metadata.json), hash match
- v2 unloaded; v3 loaded (isEffectLoaded=true). KWin crashed 1x during live-swap (--crashes 1), auto-restarted, fresh instance loads v3 cleanly
- Open: orphan kglobalaccel5 PID 167889 (boot-time, not unit-owned, holds old shortcut state in memory) — kill pending user consent (Rule 1c)
- 2026-09-07T22:54:03+05:30 — src/kittyglow.cpp: paintWindow() now maps the halo quad through the scene's animation transform (data.xScale/yScale/xTranslation/yTranslation, scale about frame top-left — same affine math as stock BlurEffect for transformed windows) so the glow tracks minimize/restore mid-flight instead of snapping to destination geometry; halo alpha multiplied by data.opacity() (fade with the window; skipped when opacity <= 0.01).

## $T — v3.1 animation-tracking + orphan cleanup
- src/kittyglow.cpp: paintWindow() maps halo quad through the scene animation transform and multiplies alpha by data.opacity() (build #4, 8fca358a…).
- Process: orphan kglobalaccel5 167889 killed (consented); plasma-kglobalaccel restarted; single daemon verified.
- Deployed + clean kwin_x11 --replace; PROJECT_CONTEXT.md, HANDBOOK.md updated.

## 2026-09-08T00:02:28+05:30 — v3.3 completion (build #6, 097b3e24…)
- src/kittyglow.cpp: occlusion-clipped halo — updateOccluders() (windows above
  kitty in logical stackingOrder(), opacity >= 0.99, same desktop/activity, not
  minimized/deleted) + occludedAboveKitty() device-px rects; paintWindow()
  renders via GLVertexBuffer::render(clip, GL_TRIANGLES, true) scissor path
  (region-granular).
- Seamless minimize: isMinimized() paint guard removed (flag flips BEFORE the
  shrink animation, so the guard skipped exactly the anim frames; fully-
  minimized windows never enter paintWindow() — no steady-state leak).
- Meta+Shift+B autorepeat gate: 220 ms QElapsedTimer in toggleKittyBorderless()
  (kglobalaccel re-emits triggered() per autorepeat → rule churn + flicker).
- REGRESSION (build #5, d445c704…): the guard-removal edit also deleted
  isKittyWindow() from paintWindow() → halo on every window (taskbar artifact,
  distorted rings). User-reported; fixed in build #6 (filter restored,
  grep-verified, zero-warning build). Lesson LL-010 in SPECIFICATION.md.
- Config repair: kglobalshortcutsrc 'Window No Border' active field restored
  (Meta+Shift+T,Meta+Shift+T,); kwinrulesrc [kitty-borderless] noborder=true
  restored; plasma-kglobalaccel restarted (single daemon PID 210789).
- Verified: dom0 sha256 097b3e24… == dist/; kwin_x11 --replace PID 211257
  (0 errors); isEffectLoaded=true; B+T active in kglobalshortcutsrc.

## 2026-09-09T08:30:11+05:30 — Git commit d697c0f: entire v2→v3.3 arc committed
- New session recovered context of dead session 01a077e8 (kitty theming → borderless → kitty-glow v1→v3.3, build #6 deployed/live).
- Committed: v3 SDF sources (glowshader.cpp/.h, glowconfig.h), kittyglow.cpp v3.3, kittyglow.json metadata fix, v2-rollout-round.sh, kittyglow-rollback.sh, test_create.cpp probe, docs (LL-006..LL-010) + HTML siblings, all ledger updates. 24 files, +1533/−197.
- .gitignore: added `src/test_create` (compiled ELF probe, kept source tracked).
- Git state before this round: last commit 2110315 (2026-09-06T23:53:14Z, scaffold era); everything after was uncommitted.
- Next (pending user go): interrupted diagnostic — Meta+Shift+B / Meta+Shift+T reported "don't work / behave the same" (dom0 read-only checks first).

## 2026-09-09T03:27:54Z — Session state snapshot written (logs/SESSION_STATE.md)
- Post-reboot diagnosis milestone: B-key root cause identified (kwinrulesrc active group renamed [kitty-borderless]→[1] by KWin re-save; effect toggles inert group). T-key live state pending one dom0 probe (cancelled at password dialog).

## 2026-09-09T03:39:35Z — kglobalaccel restart done; re-registration absent; kwin restart required
- Live keys B/T still unowned after daemon restart. Next: kwin_x11 --replace (pending consent) + one-time kwinrulesrc normalize.

## 2026-09-09T09:27:58+05:30 — **RESOLVED in dom0: Meta+Shift+B and Meta+Shift+T work again (E2E verified)**
- Root-cause chain confirmed: boot race (kwin 08:58:11 started BEFORE kglobalaccel 08:58:13) → kwin's shortcut registration lost → keys dead since Sep 08. Restarting kglobalaccel alone does NOT heal (kwin never re-registers); **kwin restart AFTER the daemon does**.
- dom0 changes: kwin_x11 replaced (PID 14836, correct HOME, kittyglow + Effects DBus healthy); kwinrulesrc normalized to a single active group [kitty-borderless] (noborder=true, rules=kitty-borderless) — KWin's numeric-rename regression ([1]) deduplicated; accidental foreign Ctrl+Shift+B/T registrations (created during wrong-key probing) cleared.
- Verification: kwin registry shows both shortcuts ACTIVE with correct key codes (Meta+Shift+B=301989954, Meta+Shift+T=301989972); simulated keypress (xdotool super+shift+b) toggled noborder true→false→true; final parity borderless (noborder=true).
- Lessons pending promotion to SPECIFICATION.md (with Step C): Qt::META=0x10000000 (key-int encoding); xdotool 'meta'='Alt', 'super'=Mod4; KF5 setForeignShortcut silently no-ops for unregistered components.

## 2026-09-09T09:37:28+05:30 — Step C implemented: content-based kitty rule toggle (regression-immune)
- NEW src/kittyborderrule.h (27 ln) + src/kittyborderrule.cpp (70 ln): `KittyBorderRule::toggleKittyNoBorder()` — finds the ACTIVE kitty rule in kwinrulesrc by CONTENT (Description=="kitty borderless" || wmclass=="kitty"), toggles noborder; self-heals inert rules by appending the group to [General] rules=; returns nullopt when no kitty rule exists (caller no-ops). Kills the LL-007 group-rename regression class.
- src/kittyglow.cpp: toggleKittyBorderless() body now calls the module (keeps 220 ms repeat-gate + loopback reconfigure; reconfigure fires for every real toggle incl. flips to false). File at 274 ln — over-200 residual accepted as tech debt (cohesive GL render path); to be noted in SPECIFICATION.md during docs phase.
- src/CMakeLists.txt: kittyborderrule.cpp added to the kittyglow MODULE sources.
- Verification (Rule 16): re-read of modified function — gate intact, optional-bool semantics correct (nullopt → no reconfigure; false → reconfigure), QDBus call untouched. Compile validation happens in dom0-replica-fed37 container at build time (headers not present on this VM).
- Pending: user build consent → container build → deploy → E2E verify → docs (HANDBOOK/PROJECT_CONTEXT/SPECIFICATION LL-011..014/ROADMAP) + HTML regen.

## 2026-09-09T10:02:46+05:30 — Build #7 deployed & verified: group-rename immunity proven E2E
- dist/kittyglow.so badf8df7 (Step C + KSharedConfig::reparseConfiguration hardening) live in dom0; effect loaded; Meta+Shift+B registration active.
- E2E: xdotool super+shift+b toggles parity; canonical rules= preserved through toggles (no stale-name resurrection — the LL-007 failure vector is closed).
- Immunity test: active group renamed [kitty-borderless]→[3] + reconfigure → presses still toggle the right group (content-based lookup + reparse working). Restored canonical name after.
- Incident logged: kwin crash during stale-binary test presses (pre-reparse build was live); auto-restart healed registration (boot-order theory re-confirmed by real crash). No user-visible damage; parity restored to noborder=true.
- Pending: docs phase (HANDBOOK/PROJECT_CONTEXT/SPECIFICATION LL-011..014/ROADMAP + HTML regen) then final commit.

## 2026-09-09T10:09:50+05:30 — Docs phase complete (build #7 / Step C round closed)
- SPECIFICATION.md: LL-011 (kwin-before-kglobalaccel boot race; heal = kwin restart AFTER daemon; crash-recovery re-validation), LL-012 (Qt modifier bit encoding; Meta=0x10000000), LL-013 (xdotool meta=Alt, super=Mod4), LL-014 (KSharedConfig process-wide cache → reparseConfiguration per toggle), LL-015 (content-based rule identity for UUID/numeric group names; no-arg allShortcutInfos as robust ownership query). Code standards: kittyglow.cpp 274-ln overage recorded as accepted tech debt; kittyborderrule module noted.
- HANDBOOK.md §6b: B toggle now documented as content-matched/self-healing/reparse-per-toggle (build #7).
- PROJECT_CONTEXT.md: build #7 state (badf8df7…), shortcut-recovery summary, file map +274/70 ln, live PID 15974, parity at rest.
- ROADMAP.md: Phases 2–3 marked DONE (long complete); Phase 6 (Shortcut Reliability v3.4) added, all items checked.
- HTML siblings regenerated via sanctioned generator; 18e doc-zone equality diff clean.

## 2026-09-09T06:45:59Z — Seamless B implementation (LL-016/LL-017 fix, source round)
- **src/kittytoggle.h/.cpp (NEW, 24+104 lines):** DBus pull-service org.kde.kittyglow /sync nextSource() (staged toggle value, consumed by 60 ms script poll) + kglowsync package presence check + kwinrc enable. PoC-7-validated architecture (script package auto-run; minimal-metadata failure root-caused to missing X-Plasma-API/MainScript).
- **src/kwin-script/kglowsync/ (NEW):** KWin script package (metadata.json with verbatim-minimizeall schema keys; main.js poller: 60 ms nextSource poll + 400 ms watchdog re-assert with 2 s service heartbeat; applies Client.noBorder live to kitty windows).
- **kittyglow.cpp:** toggle drops org.kde.KWin.reconfigure() (the LL-016 full-screen white flash) → KittyToggle::requestApply(); ctor calls KittyToggle::init(); occludedAboveKitty() now clips by expandedGeometry() (LL-017 shadow-penetration artifact); removed unused QDBus includes; header comment updated. Net 274→283 lines (pre-existing >200; paint core intentionally untouched this round).
- **src/CMakeLists.txt:** added kittytoggle.cpp to module.
- **scripts/deploy.sh:** installs kglowsync package to chenpan's KPackage path via dom0 bridge, enables kglowsyncEnabled, kbuildsycoca5 rebuild. Note: qvm-run|base64 pipe failure inside dom0 shell not pipefail-protected (same as existing .so pattern).
- **Verification (Rule 16):** bash -n deploy.sh OK; metadata.json json.tool OK; node --check main.js OK; QStringLiteral-on-identifier bug caught and fixed pre-compile; C++ compile pending build consent.

## 2026-09-09T07:16:01Z — Build round: 4 Qt API fixes + green build
- **src/kittytoggle.cpp compile fixes (first build failed exit 2, silent by design of build.sh):** `public slots:`→`public Q_SLOTS:` (QT_NO_KEYWORDS); QStandardPaths::LocalDataLocation→GenericDataLocation (Qt5 has no LocalDataLocation; GenericDataLocation = ~/.local/share, matches deploy path); `auto *bus = QDBusConnection::sessionBus()`→value type (returns by value, not pointer); ExportSlots→ExportScriptableSlots|ExportNonScriptableSlots (ExportSlots is Qt 6.5+).
- **Build:** exit 0, 0 warnings (Rule 5), dist/kittyglow.so sha256 a8f0cbfe…, 87240 bytes.
- **Note:** build.sh is silent on failure (all container output pre-redirected to /tmp/b_*.log) — candidate UX fix, deferred.

## 2026-09-09T07:57:34Z — build #8 (JS timer fix + toggle trace)
- main.js: QTimer built parentless via fallback cascade (fixes journal-confirmed "Could not convert argument 0" at :40/:52); defensive Number(src) coercion; print() proof-of-life lines.
- kittyglow.cpp: qDebug trace on toggle success/no-rule paths.
- Rebuilt (sha256 21fce193…), zero warnings. Not yet deployed.

## 2026-09-09T08:21:05Z — E2E verdict + user-reported desktop disruption
- PROVEN: full toggle chain through JS reply callback. BROKEN: Client.noBorder write instantly reverts (suspect: in-memory forcing rule loaded at kwin start).
- USER IMPACT: 13:28 pkill left desktop unmanaged ~3 min (respawn failed); 2 further restarts; kglowsync loop currently churns noBorder every 400 ms. All live dom0 experimentation halted pending user decision.

## 2026-09-09T08:36:59Z — Step C rebuild: state-store persistence (kittyglowrc) replaces kwinrulesrc rule toggle
- Root cause of live write-revert fight: a loaded forcing rule in kwinrulesrc overrides KWin scripting `noBorder` writes (scripting < rules). Any in-rule `noborderrule` value must be unset (rule still lists window properties only) for script writes to stick.
- Created `src/kittyglowstate.{h,cpp}` (`~/.config/kittyglowrc` [General] noBorder; load/save/toggle; rule file untouched).
- Removed `src/kittyborderrule.{h,cpp}` (git rm); rewired `kittyglow.cpp` toggle body + `kittytoggle.cpp` (`nextSource` + new `getCurrentState` DBus slot + bootstrap contract doc); CMake sources updated.
- Rewrote `src/kwin-script/kglowsync/contents/code/main.js`: getCurrentState bootstrap, 60 ms nextSource poll, 400 ms sweep (2 s heartbeat pause), clientAdded coverage; scriptLog diagnostics via effect.
- Fixed `QStringLiteral(constexpr)` compile error in kittyglowstate.cpp.
- Container build green (zero warnings); dist sha256 kittyglow.so 25e019db….
- Docs synchronized: HANDBOOK.md, PROJECT_CONTEXT.md, ROADMAP.md, SPECIFICATION.md now reference kittyglowstate/kittyglowrc (kittyborderrule retired).

## 2026-09-09T08:39:56Z — SESSION_STATE.md refreshed (Rule 17)
- Brain-dump: Step C rebuild committed (bb79411), artifacts hashed, deploy/E2E pending explicit user go.
