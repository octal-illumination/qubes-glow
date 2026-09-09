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
