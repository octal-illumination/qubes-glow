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

## 2026-09-09T09:02:42Z — Steps 1–3 executed on dom0: deploy, rule deletion, E2E verification PASSED
- Deploy: kittyglow.so (25e019db…) + metadata + kglowsync package installed to dom0, kwinrc enabled; new effect auto-loaded on reconfigure ("Successfully loaded plugin effect: kittyglow").
- Script: stale old-code instance unloaded; new script auto-ran — journal "bootstrap: persisted state=true -> desired=true" (getCurrentState DBus path verified live).
- kwinrulesrc: kitty forcing rule group [1] DELETED (python in-place edit preserving owner/mode 600 chenpan), [General] count=0, rules= entry dropped; protonvpn/chromium UUID rules untouched; org.kde.KWin.reconfigure flushed.
- E2E (kglobalaccel invokeShortcut x4 via busctl --user): kittyglowrc flipped false/true/false/true; each toggle exactly ONE sweep write, every write stuck ("now=" matches); 3 s steady-state silence — write-revert fight ELIMINATED. Final state borderless (user normal).
- Note: first E2E attempt failed "name is not activatable" — agent error: busctl without --user targets system bus (kglobalaccel itself healthy, shortcut registered). Diagnosed and corrected in-session.

## 2026-09-09T09:09:13Z — Docs + session state refreshed (deployment recorded)
- PROJECT_CONTEXT.md §5/§10: build #8 deployed + E2E-verified on dom0 (14:31Z); busctl --user note.
- HANDBOOK.md: E2E verification note on the Meta+Shift+B toggle entry.
- SESSION_STATE.md: feature-complete snapshot; hardening idea (bootstrap retry) proposed, not approved.

## 2026-09-09T16:08:54Z — fix(kittyglow): translucent dock/panel halo penetration (LL-018)
- **src/kittyglow.cpp** \`updateOccluders()\`: occluder skip condition \`w->opacity() < 0.99\` now applies only to non-dock windows; docks/panels always clip (synthetic probe: 1,450 gold px on panel vs 48 px baseline). Pending: rebuild + redeploy + KWin restart (separate build consent).

## 2026-09-09T16:44:22Z — build+deploy: Build #9 (LL-018 fix) live on dom0
- Build zero-warning; artifact sha256 e7ff8627… verified on dom0 post-deploy.
- kwin_x11 restarted 32381→32660, restart log clean, kglowsync bootstrap expected at start.
- Post-restart verification (isLoaded, metadata 755/644 fix, shortcut registration) pending — dom0 password dialogs cancelled twice.

## 2026-09-09T16:55:43Z — verify: Build #9 confirmed live on dom0 (LL-018)
- kittyglow.so: 5 memory mappings in kwin PID 32660 (definitive load proof via /proc/maps; KWin 5.27.8 exposes no loadedEffects/isLoaded DBus methods).
- Shortcuts: Toggle Kitty Borderless (B) + Window No Border (T) both registered post-restart (LL-011 self-heal verified).
- dom0 metadata dir perms normalized 755/644 (was chenpan-unreadable; would have blocked effect metadata).
- Remaining: visual panel acceptance test / probe re-run.

### 2026-09-09T18:30:25Z — LL-019 addendum 3: repro simplified (any front window), API verification, session recovery
- Recovered prior-session state after 2M-token session death; committed pending log updates (session-recovery commit).
- User re-test: artifact is systematic — ANY unmaximised/unminimised window placed in front of kitty penetrates (konsole, firefox; position-independent; back windows irrelevant).
- Upstream 5.27.8 API checks: stackingOrderChanged() EXISTS (line 1820, since 4.10); windowActivated() is the correct focus signal (activeWindowChanged does NOT exist); no EffectsHandler::movingWindow(); isUserMove() available.
- Revised 5-item fix bundle proposed in logs/SESSION_STATE.md §5; source untouched pending Rule 1a consent.

## 2026-09-09T18:55:33Z — LL-019 penetration fix implemented (v3.4, Build #10 pending consent)
- src/kittyglow.cpp: occluders rebuilt from stackingOrder() on EVERY halo
  paint — 120 ms cache deleted (m_occluders/m_stackingStamp/updateOccluders
  removed); occluder set anchored to the PAINTED kitty window (was topmost
  kitty — wrong set with 2+ kitty windows); added stackingOrderChanged +
  windowActivated connections repainting the full kitty halo ring via new
  KittyGlowEffect::repaintAllKittyHalos(); header v3.3→v3.4.
- SPECIFICATION.md: Lessons Learned registry repaired — LL-017/LL-018 were
  never entered (Build #9 fixes existed only in HANDBOOK); added LL-017
  (expandedGeometry shadows), LL-018 (docks always clip), LL-019 (no
  cross-frame occlusion cache; stacking/activation repaint hooks); fixed
  LL-015/LL-016 bullet formatting; orphaned kglobalaccel note kept as own
  bullet.
- HANDBOOK.md: renderer/occlusion bullets updated to v3.4 per-paint
  semantics (HANDBOOK.md, §Limitations).
- PROJECT_CONTEXT.md: Known Issues LL-019 entry (fix coded, Build #10
  pending explicit build consent), Pending = v3.4 acceptance test,
  Last Updated prepended.
- ARCHITECTURE.md: rewritten — still described retired v1/v2 design
  (8-layer drawGlow, m_windows QSet, windowAdded seeding); now documents
  the v3.4 SDF pipeline incl. per-paint occlusion and repaint hooks.
- Rule 18: 4 HTML siblings regenerated via sanctioned script; --check OK;
  scoped 18e equality diff clean after reverting orphan
  logs/SESSION_STATE.html (logs/ ledgers are not HTML-generated, Rule 22f).
- Rule 16: no stale symbols; stackingOrderChanged()/windowActivated()
  verified against upstream kwineffects.h 5.27.8; brace balance OK; changed
  regions re-read (paintWindow clip block, repaintAllKittyHalos,
  occludedAboveKitty, constructor wiring).
- Build #10 (compile + deploy + kwin restart) NOT run — Rule 1b: awaiting
  explicit "build the app".

## 2026-09-09T18:56:35Z
- logs/SESSION_STATE.md: stripped stray first-line HTML-sibling pointer comment (Rule 22f: logs/ ledgers are not HTML-generated; the pointer is the generator selection key and kept regenerating an orphan SESSION_STATE.html). Scoped 18e equality diff now clean.

## 2026-09-09T18:58:09Z
- logs/SESSION_STATE.md: rewritten per Rule 17 (v3.4 milestone, >15 tool calls) — recovery record, decisions, pending Build #10 + acceptance test, handoff message.

## 2026-09-09T19:10:26Z — Build #10 executed: v3.4 deployed live (awaiting user acceptance)
- scripts/build.sh in dom0-replica-fed37: zero warnings (Rule 5 log review),
  dist/kittyglow.so sha256 31254894d21b…, kittyglow.json 3a3f66bc…
- scripts/deploy.sh: dom0 .so + metadata hash-verified identical to dist;
  kwinrc kittyglowEnabled=true confirmed.
- kwin_x11 --replace (approved activation chain): old 34892 → 35345. Note:
  kwin PID had changed to 34892 between sessions (restart/reboot; prior
  ref 32660 stale). LL-011 registration loss observed (new kwin before
  kglobalaccel re-attach → zero kittyglow entries in allShortcutInfos);
  healed by documented remedy (kwin restart AFTER kglobalaccel active):
  35345 → 36048. Verified: isEffectLoaded=true, B/T shortcut structs present,
  .so mapped 5× in /proc/36048/maps.
- Verification-path notes: (a) scripts/build.sh lost its execute bit —
  invoked via Built:
31254894d21bed650e129410f9372c5fa891fc0b61a707d9b912755f04b5e335  /home/user/Projects/QubesOS/UI-Enhancements/Kwin/kitty-glow/dist/kittyglow.so
3a3f66bc269ab88315b5be07e9ab7c554cf7a26db2de5d0df6e08d2e40b6a06f  /home/user/Projects/QubesOS/UI-Enhancements/Kwin/kitty-glow/dist/kittyglow.json; (b) correct allShortcutInfos call is
  /component/kwin org.kde.kglobalaccel.Component.allShortcutInfos (NOT
  org.kde.kglobalaccel.KGlobalShortcutInfo) — introspected live and recorded
  to prevent repeat 4-call hunt; (c) DBus probes must run under
  sudo -u chenpan with session env (HANDBOOK §5).
- PROJECT_CONTEXT.md §8/§10 synced (build live, USER ACCEPTANCE PENDING);
  HTML sibling regenerated (Rule 18a).
- Build logs: logs/build/build-10-*.log, logs/build/deploy-10-*.log.
- Rule 2 cleanup: no monitoring symlinks created (foreground tee capture).
## 2026-09-10T01:50:00Z — LL-019 v3.4 live repro campaign (no code change)
- Empirical probe campaign on live dom0 (kwin 36048): PIL ImageGrab + numpy strict-gold forensics; static states A/B/C verified LL-017/019-correct; two transient unclipped frames (d02/d16) captured at interaction boundaries; progressive-accumulation hypothesis formed from user's "intensity/thickness increases" report. See logs/SESSION_STATE.md for full state and next step. No source modified.

## 2026-09-10T18:51:08+05:30 — LL-020 fix replaced: CPU quad subdivision (build #11 code)
- src/kittyglow.cpp: scissor-based hw clipping ABANDONED — KWin 5.27.8 per-rect scissor boxes proved degenerate in our paint context (positive control 2026-09-10: 74/91 px noise floor with kitty unoccluded; halo drew nothing since build #10 deploy). Replaced with one quad per clip rect drawn unclipped via 1-arg render(GL_TRIANGLES); SDF is fragment-position-based so sub-rects are pixel-identical to the full quad.
- occludedAboveKitty(): isDesktop() exclusion added — plasma desktop window must never occlude (stackingOrder() can hold it high after restacks; would empty the clip).
- Header bumped v3.5→v3.6. STATUS: code written, NOT yet built/deployed. Pending: build #11, deploy, kwin restart, positive control + occlusion + leak burst, then SPECIFICATION LL-020 lesson rewrite + HANDBOOK/PROJECT_CONTEXT/ARCHITECTURE refresh + regression assertion + commit.

## 2026-09-10T20:23:41+05:30 — LL-020 closed: builds #11–#13, subdivision mechanism verified
- src/kittyglow.cpp: LL-020 final mechanism (clip = haloRect − occluders; CPU sub-quad draws via 1-arg render; isDesktop() occluder skip; instrumentation stripped in #13 cb63bd4b). E2E: positive control 5,214/25,896 px, occlusion 0 px, 24-cycle burst flat 320/394.
- SPECIFICATION.md: LL-020/021/022 entries added (three-part render lesson; live-reload cannot swap .so; deploy sha-verify).
- ARCHITECTURE.md §2.3/§3/§4 + HANDBOOK.md §5 + PROJECT_CONTEXT.md §5/§8/§10 synced; HTML siblings regenerated via sanctioned script.

## 2026-09-10T23:11:18+05:30 — Seamless pass-behind shipped (#14) + user-accepted
- src/kittyglow.cpp: non-dock occluders clip at frameGeometry() (LL-017 superseded; docks keep expandedGeometry). USER ACCEPTED: "Perfect, everything works as it should and seamless."
- scripts/deploy.sh: LL-022 sha hard-gate (exit 1 on dom0/local mismatch); fired live on #14 deploy.
- scripts/regression-checks.sh: NEW in-repo registry (Rule 20, option (a) per user) — 8 assertions green; check_not helper (bash `!` through "$@" pitfall).
- SPECIFICATION.md LL-017 supersession + ARCHITECTURE.md §2.3 + HANDBOOK.md occlusion + PROJECT_CONTEXT.md §5 (Build State restored after atomic edit rollback) + §10; HTML siblings regenerated (18e audit clean).

## 2026-09-11T08:05+05:45 — Build #15 (all-windows glow) shipped
- src/kwin-script/kglowsync/contents/code/main.js: resilient bootstrap (slog try/catch + retry ≤60×500ms) — LL-025 bus-name race on kwin --replace.
- logs/probe/ll015_diagnose.py, ll015_allwin.py, ll015_verify2/3/4.py: build #15 probes (config audit, staged ring/exclusion checks, xprop notification type).
- scripts/regression-checks.sh: +ll025-kglowsync-resilient-bootstrap (11 assertions, all green).
- SPECIFICATION.md: LL-024 (benign two-loader metadata probe) + LL-025; HANDBOOK §1/§11 all-windows behavior + verification methods; PROJECT_CONTEXT §5/§6/§7/§10 build #15 state; SESSION_STATE.md rewritten.
- Deploy sha-verified 6d8886da… on kwin 56055; verification: kitty 1,287 px PASS, dialog 112 PASS, notification dock-strip 0 PASS, toggle + borderless persistence live.

## 2026-09-11T08:4x+05:45 — Build #16 implemented (pre-build)
- src/kittyglow.cpp: v3.9 — new toggleBorderless() (gate + toggleNoBorder + requestApply); B action re-pointed to it (display "Toggle Window Borders"); NEW G action "Toggle Glow" (Meta+Shift+G); header/class comments synced.
- src/glowtargets.h: LL-026 chrome-class exclusion (plasmashell, Qui-*, xembedsniproxy, krunner) — qubes-gui strips _NET_WM_WINDOW_TYPE.
- src/kwin-script/kglowsync/contents/code/main.js: kittyWindows()/isKitty() → appWindows()/isBorderlessTarget() (class + defensive type-flag mirror); all kitty-scope wording updated.
- scripts/regression-checks.sh: +4 LL-026 assertions (15 total, all green).
- SPECIFICATION.md LL-026; HANDBOOK §1/§6b/§11; PROJECT_CONTEXT §5/§6/§10; SESSION_STATE refreshed.
- STATUS: implemented, verified statically (node --check + assertions); build #16 NOT yet run — awaiting explicit build consent.

## 2026-09-11T09:22:36+05:30 — Build #17 shipped + verified live (d66b9009)
- src/glowtargets.h + kittyglow.cpp: minimum frame-size guard (sub-48 px =
  icon, never haloed) — removes unmanaged Qui-* tray-source ghosts
  (override-redirect, no WM_CLASS); corner gold 456 → 0 verified live.
- src/kwin-script/kglowsync/contents/code/main.js: bootstrap watchdog —
  re-arms every 5 s until the first service reply (lost-reply wedge seen on
  build #16 restart).
- scripts/regression-checks.sh: +2 assertions (17 total, all green).
- SPECIFICATION LL-026 supplemented; HANDBOOK 6b/11 updated; PROJECT_CONTEXT
  build state refreshed; research doc resolution appended.
- Verified live (kwin 60603): ghost gone, tray clean, B swept 14 windows
  both directions, G fires, borderless default at startup.

## 2026-09-11T10:47:10+05:30 — Build #18 shipped + user-accepted (7df23937)
- src/glowfocus.{h,cpp}: NEW — runtime-only per-window glow override set,
  pruned on windowDeleted (dangling-pointer guard).
- src/kittyglow.cpp: B/G rerouted to FOCUSED-window handlers; NEW global
  masters Meta+Shift+Alt+B/G (persisted sweep semantics); paint gates
  consult GlowFocus::glowAllowed; windowDeleted pruning.
- src/kittytoggle.{h,cpp}: nextWindowOp() DBus slot (window-op channel).
- main.js: overrides map (windowId->bool) shields per-window flips from
  the 400 ms sweep; focusedBorderFlip via workspace.activeClient.
- CMakeLists + kittyglow.json v0.3; +3 assertions (20 total, all green).
- Verified live (kwin 62946): focused-op flips sweep-safe (konsole+kitty),
  focused G scope-named, Alt-masters global with visual swing, user
  physical-key acceptance PASSED. LL-027 recorded; HANDBOOK 6b rewritten;
  PROJECT_CONTEXT updated.

## 2026-09-11T11:10:38+05:30 — Deep comprehensive audit (docs/research/2026-09-11-deep-audit.md)
- Full-source audit of build #18: structure, logic, ops, syntax, idiom,
  integration, process flow, lint (shellcheck/node), security, memory,
  races, math, AI-drift. NO code modified (audit only).
- Verdict: 0 critical/high; 4 medium (stale v2 deploy script w/ retired
  rule flow + old sha; latent HiDPI damage-widening bug; shared autorepeat
  gate dropping cross-toggle presses; stale ROADMAP) + 11 low. All fixes
  PROPOSED, awaiting consent.
- Verified clean: SDF math, RAII/memory, buffer safety, sha/byte-identity
  of deployed artifacts, 20/20 assertions, race contracts, doc claims
  traced to code.

## 2026-09-11T11:19:43+05:30 — Audit fixes implemented (M1–M4, L1–L10)
- M1: scripts/v2-rollout-round.sh PURGED (stale v2 flow, old sha, retired
  rule mechanism).
- M2: prePaintWindow/repaintHalo damage widening now device-px
  (maxExtent * renderTargetScale) — LL-028.
- M3: per-action autorepeat gates (4 timers + gated() helper) — LL-029;
  shared gate removed.
- M4: ROADMAP.md refreshed to build-#18 state (was stale at build #7).
- L1/L2: kittyglow.json Description de-drifted; kglowsync metadata 1.0→1.1.
- L3: kittyglowstate.h/.cpp comments updated to v3.10 semantics.
- L4/L5: glowconfig readColor clamped 0..255 + 4th-alpha dead path removed;
  glow widths qBound 0..200.
- L6/L7: HANDBOOK §11 documents focus-race + last-wins staging edges.
- L8: scriptLog flattens newlines (journal-injection hygiene).
- L9: orphaned duplicate comment removed (kittyglow.cpp).
- L10: test_create.cpp disclaimer added.
- +3 assertions (23 total): ll028 device-px widening, ll029 per-action
  gates, m1 v2-script purged. SC2164 fixed in regression-checks.sh.
- NOT applied: L11 container rebase (Phase 7 backlog).
- All static checks green; C++ rebuild REQUIRED before deploy (consent
  requested separately).

## 2026-09-11T12:29:38+05:30 — Build #19 shipped + M3 verified live
- Compiled (0 warnings, sha 07537ab5), deployed (sha-verified), kwin
  restarted (pid 64176), bootstrap restored borderless default.
- M3 live proof: G → Alt+G 80 ms apart → journal shows BOTH
  "toggle: glow (focused …)" and "toggle: glow (global)" — the shared
  gate would have dropped the second press. Glow master restored on.
- Full fix batch contents: see previous entry + commit 10db577.

## 2026-09-11T12:47:01+05:30 — Re-audit 2 complete (post build #19)
- Full-spectrum re-audit; report: docs/research/2026-09-11-reaudit-2.md.
- Verdict: no critical/high, no build-#19 regression. Pinned-source
  adjudication (kwin 5.27.8) proved the previous audit's M2 wrong:
  effect-facing regions are LOGICAL; one scale boundary at the vertex
  upload. F1 damage widening, F2 occluder space, F3 translation space —
  all latent (s==1 system). M5: rollback misses kglowsync. Six minor notes.
- Drift corrections proposed (SPEC LL-028, assertion ll028,
  PROJECT_CONTEXT, kittyglow.cpp comments) — awaiting consent.
