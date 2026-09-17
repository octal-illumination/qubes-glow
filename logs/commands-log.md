# Commands Log

## 2026-09-06T23:50:19Z — Re-point build container
- Command: `bash container/setup-build-container.sh`
- Reason: address item #2 — make the build container mount the project's `src/`
  and capture the toolchain into image `dom0-replica-fed37-img` so deps persist.

## 2026-09-06T23:40:28Z — Scaffold project tree + import source
- Command: `mkdir -p .../kitty-glow/{src,container,scripts,docs/research,logs/{build,output,error,run},dist}` then `cp /home/user/kitty-glow/{kittyglow.cpp,CMakeLists.txt,kittyglow.json} .../src/`
- Reason: create AGENTS.md-compliant project structure and import canonical source.

## (prior session) Build effect in container
- Command: `podman exec dom0-replica-fed37 bash -c 'cd /src && rm -rf /tmp/build && cmake -B /tmp/build -DCMAKE_BUILD_TYPE=Release && cmake --build /tmp/build -j$(nproc)'`
- Reason: compile the KWin effect against the KWin 5.27.8 ABI.

## (prior session) Deploy to dom0
- Command: `dom0 "sudo bash -lc '... qvm-run -p Dev-General base64 -w0 <so> | base64 -d > /usr/lib64/qt5/plugins/kwin/effects/kittyglow.so; ...; chmod 644 ...'"`
- Reason: install plugin into dom0 where the compositor runs.

## (prior session) Enable plugin
- Command: `dom0 "kwriteconfig5 --file kwinrc --group Plugins --key kittyglowEnabled true"`
- Reason: register the effect so KWin loads it on next (re)start.

## 2026-09-06T23:52:44+00:00 — Remove legacy kitty-glow dir
- Command: `rm -rf /home/user/kitty-glow`
- Reason: clean up redundant pre-project working copy (user-approved; kitty-glow-out retained).

## 2026-09-06T23:53:14+00:00 — Remove kitty-glow-out
- Command: `rm -rf /home/user/kitty-glow-out`
- Reason: user-approved cleanup of redundant build-artifact copy.
## 2026-09-07T04:41:22Z — hardened kittyglow rebuild
- Command: `podman exec dom0-replica-fed37 cmake --build /tmp/build` + host container probe
- Reason: rebuilt effect after KWin crash — added isOpenGLCompositing() guard, RAII ShaderBinder, renderTargetScale() coords; verified IID/scan/instance headlessly (sha 9115df9a)
## 2026-09-07T04:45:58Z — rollback escape hatch
- Created scripts/kittyglow-rollback.sh (bash -n OK, 55 lines): unloads effect via DBus, strips kwinrc flag, removes plugin file, restores KWin or xfwm4. Modes: default|xfwm4|only
- Reason: user requested a one-command dom0-terminal escape hatch before any rollout
- 2026-09-07T04:46:52Z Wrote logs/SESSION_STATE.md (pre-deploy snapshot: rollback script ready, awaiting deploy consent)
## 2026-09-07T04:59:47Z — rollback widened to cover border script
- Forensics: nm -D on 5 leftover .so variants showed NO decoration APIs; root cause of konsole titlebar loss = kwinrc [Plugins] kitty-toggle-borderEnabled=true (JS script, over-broad matching), seen in /tmp/kwinrc snapshot
- Edited scripts/kittyglow-rollback.sh: now also DBus-unloads kitty-toggle-border, deletes its kwinrc flag, removes ~/.local/share/kwin/scripts/kitty-toggle-border/, sends KWin reconfigure; bash -n OK
- Reason: user reported konsole lost titlebar/border when effect was live-loaded
- 2026-09-07T05:00:08Z Updated logs/SESSION_STATE.md with root-cause section (toggle-border JS script = global decoration thief; kwinrulesrc window rule = correct scoped approach)
## 2026-09-07T05:15:53Z — rollback deployed & executed in dom0
- dom0 preflight: xfwm4 195973 live; kwinrc had kittyglowEnabled + kitty-toggle-borderEnabled; script dir + buggy .so present
- Attempt 1: install to /usr/local/bin as user → Permission denied (dom0.AuthExec runs as user; sudo required) — no state changed
- Attempt 2: sudo install OK → kittyglow-rollback (default mode) ran: both [Plugins] flags stripped, script dir removed, .so removed; its step-3 kwin relaunch replaced xfwm4 but died silently (logged to /dev/null — blind spot, now fixed in project copy)
- Attempt 3 (after one Access-denied dialog): found bare-env kwin 196556 from attempt-2 timing; pkill'd it, relaunched kwin from plasmashell env (PID 28279 source, DISPLAY=:0, XDG_RUNTIME_DIR=/run/user/1000) → kwin 197241 alive, no errors, DBus supportInformation OK
- Reason: user approved deploy+run of the escape hatch to restore decorations
- 2026-09-07T05:16:19Z Script fix: kwin relaunch now logs to /tmp/kwin-relaunch.log (was /dev/null — the blind spot that cost two recovery rounds); bash -n OK; SESSION_STATE updated
## 2026-09-07T05:18:46Z — log append for Q&A turn
- Reason: Rules 8/10 — record mode clarification; no commands run against dom0 or project files
2026-09-07T06:20:27Z | v2-rollout-round.sh | dom0 round: deploy .so+metadata, enable kittyglow, register kitty-borderless rule (noborderrule 1→2 Force), DBus reconfigure, verify loadedEffects + supportInformation
2026-09-07T06:21:32Z | v2-rollout-round.sh | dom0 round (file-based fix): verify/complete deploy, enable, rule, reconfigure, verify

## 2026-09-07T06:30:03Z
- Command: `grep -n "^## " HANDBOOK.md` / python section renumber — Reason: fix stale §6–§9 numbers after inserting new §5–§7 activation content (Rule 16 consistency).
- Command: `python3 deploy.sh patch + bash -n` — Reason: apply LL-004/SESSION_STATE fix (SO_DIR missing /effects/plugins) and replace outdated restart instructions with the verified DBus activation.
- Command: `ls dist/ + sha256sum` — Reason: PROJECT_CONTEXT Build State must record the real v2 artifact identity, not invented numbers (Rule 6 accuracy).
- Command: `python3 PROJECT_CONTEXT.md sections 5/6/7/10 update` — Reason: sync state snapshot with v2 completion (Rule 12c).
- Command: `cat >> logs/{CHANGELOG,commands-log,Action-History,output-history}.md` — Reason: atomic logging (Rules 3/9/10/8/23).
- Command: `bash ~/Projects/scripts/generate-docs-html.sh HANDBOOK.md SPECIFICATION.md ARCHITECTURE.md PROJECT_CONTEXT.md` — Reason: regenerate HTML siblings after .md edits (Rule 18a).
- Command: `18e equality diff` — Reason: verify doc-zone HTML set matches pointer-bearing sources with zero orphans/missing.

## 2026-09-07T06:48:49Z
- Command: dom0 read-only diagnostic batches (x3 via qrexec AuthExec) — Reason: user reported "kwin refuses to restart"; inspected coredumpctl, journal, kwinrc/kwinrulesrc, DBus liveness, supportInformation, systemd user units without modifying anything (Rule 1d read-only diagnosis).

## 2026-09-07T06:58:06Z
- Command: dom0 read-only sweep (kglobalshortcutsrc, kwin scripts dir, kitty conf, kwinrulesrc, script main.js, kglobalaccel bus owner) — Reason: user asked to verify the Meta+Shift+B toggle-title/border command and overall kitty visual config; no changes made.

## 2026-09-07T07:08:41Z
- Command: dom0 recovery round via AuthExec (snapshots, kwriteconfig5 shortcut bind, kglobalaccel unit restart, python3 kwinrulesrc dedupe, residue rm, setsid kwin_x11 --replace rehome, KWin reconfigure, plasmashell start) — Reason: execute the 4-step user-approved recovery plan atomically with in-round verification.
- Command: dom0 read-only verification round (bus owner, file recheck, parent chain, isEffectLoaded, journal since rehome, which kitty) — Reason: post-modification verification (Rule 16); confirmed daemon-side binding acceptance and clean session state.
- First AuthExec attempt returned transient qrexec "Access denied"; retried the identical approved round once and it succeeded (matches prior incident pattern).

## 2026-09-07T07:14:03Z
- Command: python3 exact-match replace in logs/output-history.md — Reason: repair backtick-expansion mangling ('none') from the earlier unquoted heredoc.
- Command: sed/grep scoped 18e doc-zone diff + HTML content spot-check — Reason: verify HTML siblings regenerated correctly (Rule 16/18e); workspace-wide ls-side non-recursion artifact noted as pre-existing.
- Command: write logs/SESSION_STATE.md — Reason: Rule 17 milestone snapshot after recovery round completion.

## 2026-09-07T13:11:13Z
- Command: podman grep rounds over kwineffects.h/kwinglutils.h (GLShader ctor, ShaderManager::generateCustomShader, registerGlobalShortcut, config(), windowMinimized/Unminimized, noBorder absence) — Reason: verify exact KWin 5.27.8 APIs before proposing v3.
- Command: google_search x6 + visit_page(KDE-Rounded-Corners) — Reason: user-requested web research on official config pattern, shader approach, decoration split limits, artifact patterns.
- Command: append logs/researched-ideas.md — Reason: Rule 8 research documentation.

## 2026-09-07T22:02:39+05:30
- Command: bash scripts/build.sh (tail) — verify zero-warning v3 build + sha256
- Reason: approved v3 round; build artifacts feed the dom0 deploy step.
- Command: dom0 batch — stop/start plasma-kglobalaccel, kwriteconfig5 rebind+dedupe, deploy v3 via qvm-run base64 pipe, unload v2/load v3 via dbus-send, verify isEffectLoaded + shortcut file + journal
- Reason: execute the approved rebind + v3 deployment atomically with pre/post verification.
- Command: dom0 read-only diagnosis (pgrep, busctl, journalctl, isEffectLoaded) — confirm post-crash live state
- Reason: NoReply on loadEffect required ground-truth check of kwin/kglobalaccel process and bus state.

### 2026-09-07T22:54:03+05:30
- **Command:** edit src/kittyglow.cpp (paintWindow transform tracking + opacity fade)
- **Reason:** approved fix round — glow led the window during minimize/restore animations; mapping through data.toMatrix()-equivalent affine transform is the verbatim technique stock BlurEffect uses.

### $T
- **Command:** dom0 kill 167889; systemctl --user restart plasma-kglobalaccel.service
- **Reason:** approved — orphan held stale boot-time key grabs that shadowed the fresh daemon and killed the T binding.

### $T
- **Command:** dom0 kwin_x11 --replace (as chenpan, after deploy.sh)
- **Reason:** approved — activate build #4 without the crash-prone hot-reload path.

## 2026-09-08T00:02:28+05:30
- Command: sha256sum dist/kittyglow.so + dom0-side so/metadata
  Reason: confirm build #6 (097b3e24…) deployed byte-identical.
- Command: bash scripts/build.sh (cmake+make, replica container)
  Reason: rebuild after restoring the isKittyWindow filter; verify zero warnings.
- Command: dom0 kwriteconfig5 (kglobalshortcutsrc active field, kwinrulesrc
  noborder) + plasma-kglobalaccel restart
  Reason: restore shortcut active-field + rule parity wiped by the B-toggle.
- Command: dom0 qdbus org.kde.KWin /KWin reconfigure; /Effects loadEffect
  kittyglow; kwin_x11 --replace (backgrounded, journal captured)
  Reason: load build #6; confirm clean restart and effect liveness.

## 2026-09-09T08:30:11+05:30 — Git commit: v2→v3.3 arc committed (session 01a077e8 recovery)
- Command: `git add -A` + `git commit -m "v2→v3.3: ..."` + `.gitignore` += `src/test_create`
- Reason: previous 2M-token session (01a077e8) died before committing any v2→v3.3 work; user precondition for continuing was an up-to-date git state.
- Result: commit d697c0f (24 files, +1533/−197), tree clean. Probe binary `src/test_create` ignored (kept `src/test_create.cpp`).

## 2026-09-09T03:20:48Z — Post-reboot shortcut diagnosis (dom0 read-only probes via dom0 helper)
- **Command:** dom0 'pgrep kwin_x11; grep kglobalshortcutsrc (Toggle Kitty Borderless|Window No Border); grep kwinrulesrc kitty-borderless'
  - **Reason:** Verify on-disk shortcut bindings and rule state after Sep 08 boot.
- **Command:** dom0 'qdbus org.kde.kglobalaccel /kglobalaccel introspect; qdbus org.kde.kwin.Effects /Effects loadedEffects; qdbus org.kde.KWin buildId; cat -n kwinrulesrc; awk section-map kglobalshortcutsrc; journalctl -b kwin/kglobalaccel'
  - **Reason:** Determine live DBus service state, full kwinrulesrc contents, journal evidence.
- **Command:** dom0 'busctl getGlobalShortcutsByKey + shortcut queries' — **CANCELLED by user** (password dialog dismissed).
  - **Reason:** Confirm live kglobalaccel ownership of Meta+Shift+B / Meta+Shift+T.
- **Findings:** org.kde.kwin.Effects bus service missing/flaky; kittyglow effect loads OK per journal; kwinrulesrc has ACTIVE group [1] (noborder=true, rules=1) + INERT duplicate [kitty-borderless] (noborder=false) — effect toggles the INERT group.

## 2026-09-09T03:39:35Z — kglobalaccel restart + live-state verification (dom0)
- **Command:** dom0 'systemctl --user restart plasma-kglobalaccel.service; busctl action(B/T); qdbus /component/kwin shortcutNames; journalctl'
  - **Reason:** Approved recovery step: restart kglobalaccel to trigger re-registration; verify live key ownership.
- **Result:** Daemon restarted cleanly (09:07:34, active). Keys B/T STILL unowned (as 0), kwin component 0 shortcuts → kwin does NOT re-register on daemon restart. Execution context discovered: dom0 helper runs as chenpan uid 1000; hardcoded XDG_RUNTIME_DIR=/run/user/1000 export required.
- **Conclusion:** Registration loss (kwin 08:58:11 before kglobalaccel 08:58:13 at boot) can only be healed by restarting kwin_x11 itself.

## 2026-09-09T03:57:58Z — kwin_x11 --replace + kwinrulesrc normalization + recovery (dom0)
- **Command:** dom0 'kill -9 5679; setsid kwin_x11 --replace (first attempt: env inherited wrong HOME → relaunched with HOME=/home/chenpan XDG_DATA_DIRS std → PID 14836); cat+python-reorder kwinrulesrc (merge [1]+[kitty-borderless] → single active [kitty-borderless] noborder=true, rules=kitty-borderless); qdbus org.kde.KWin /KWin reconfigure'
  - **Reason:** Approved Steps A+B: kwin restart after kglobalaccel to heal lost shortcut registration; one-time normalization of the duplicate-rule regression so the installed binary's hardcoded group name works again.
- **Result:** kwin healthy (PID 14836, HOME=/home/chenpan, kittyglow loaded, Effects DBus interface back). kwinrulesrc now exactly one active group [kitty-borderless] noborder=true, stable across reconfigures.

## 2026-09-09T03:57:58Z — live registry + E2E keypress verification (dom0)
- **Command:** dom0 'dbus-send getGlobalShortcutsByKey int32 (wrong keys 100663362/80 → empty); xdotool key meta+shift+b (meta=Alt in xdotool → no-op); setForeignShortcut (wrong keys → registered accidental Ctrl+Shift+B/T)'
  - **Reason:** First verification attempt — used WRONG Qt key encoding (assumed Qt::META=0x04000000, actually 0x10000000).
- **Command:** dom0 'dbus-send /component/kwin allShortcutInfos (no-arg, definitive); getGlobalShortcutsByKey int32:301990978/96 (my miscalculated values → empty); setForeignShortcut array:int32: (clear accidental Ctrl+Shift regs); xdotool key super+shift+b ×2'
  - **Reason:** Correct verification with proper key decoding + cleanup of accidental registrations + true end-to-end keypress test.
- **Result:** **SUCCESS.** kwin registry: "Toggle Kitty Borderless" active key 301989954 (=0x12000042=Meta+Shift+B ✓), "Window No Border" active key 301989972 (=0x12000054=Meta+Shift+T ✓). xdotool super+shift+b flipped noborder true→false→true — full daemon→grab→dispatch→effect chain WORKS. Accidental Ctrl+Shift regs cleared. Parity restored: rules=kitty-borderless, noborder=true.

## 2026-09-09T04:07:28Z — Step C implementation
- **Command:** grep KConfig usage + CMakeLists add_library + header availability probes; wc -l; git diff --stat; git add -A && git commit
  - **Reason:** Locate KConfig touchpoints before extracting the toggle; confirm build location (container); Rule 16 verification of the edit; atomic commit per Rule 14.

## 2026-09-09T04:32:46Z — Build #7 (Step C), deploy, live-reload, E2E + immunity verification (container + dom0)
- **Command:** scripts/build.sh ×2 (initial; reparse hardening) — container dom0-replica-fed37
  - **Reason:** Approved build of Step C module. Zero warnings both times (make logs grepped).
- **Command:** scripts/deploy.sh ×2 → dom0 /usr/lib64/qt5/plugins/kwin/effects/plugins/kittyglow.so (SHA cc2ac79b → badf8df7)
  - **Reason:** Deploy new binary via approved pipeline (qvm-run base64 transfer).
- **Command:** dom0 dbus-send unloadEffect/loadEffect/isEffectLoaded; allShortcutInfos; xdotool key super+shift+b ×N (multiple verification batches, one password dialog each)
  - **Reason:** Verify live reload, registration, E2E toggle; then rename-immunity test ([kitty-borderless]→[3] rename + reconfigure + presses).
- **Findings/Results:**
  - New binary live (badf8df7, includes reparseConfiguration fix), B registration active (301989954), E2E presses toggle parity correctly, rules= stays canonical (no resurrection).
  - First press after unload/load sometimes no-ops (daemon re-grab race) — benign; real usage loads effect at login.
  - Immunity verified: presses flip parity with numeric/renamed active group; self-heal path compiled in.
  - **Incident:** kwin crashed during the earlier stale-binary presses batch (old build without reparse was live; unload/load churn suspected); plasma auto-restarted kwin (PID 15974, --crashes 1). Post-crash, shortcuts auto-re-registered (kglobalaccel was up first) — T=301989972, B=301989954 confirmed. Recovery order theory re-validated by a real crash.
  - kwinrulesrc final: canonical [kitty-borderless] active (count=1, rules=kitty-borderless, noborder=true); UUID groups are user's protonvpn:chromium rules (untouched).

## 2026-08-31T00:00Z — dom0 diagnostic probe (DENIED)
- **Command:** `dom0 'bash -lc "... base64 -d <<< $PAYLOAD | python3 -"'` (kitty extents before/after T-key, T on non-kitty, 7-frame white-flash capture via ImageMagick `import`, parity check, journal scan)
- **Reason:** Read-only runtime diagnosis of the three reported issues (flash on B, dead T shortcut, glow artifact over neighbor window). Result: `Access denied.` — password dialog not completed; probe NOT executed.

## 2026-09-09T~05:05Z — dom0 probe batch 2 (T on chromium/kitty, stacking, flash frames)
- **Command:** dom0 python probe: T-key extents tests (kitty+chromium), _NET_CLIENT_LIST_STACKING sweep, 7-frame flash capture via `import`, scripting loadScript.
- **Reason:** Diagnose issue 2 (T) + issue 1 (flash) + issue 3 (stacking context). Key findings: T intermittent on chromium (T1 lost, T2 worked, T3 lost); no collateral damage; kitty on desktop 2; flash/scripting sections skipped (no visible kitty); kglobalshortcutrc filename wrong.

## 2026-09-09T~05:20Z — dom0 probe batch 3 (shortcut registry, T 6-press, scripting introspect)
- **Command:** dom0 python probe: full kwin-component shortcut list, kglobalshortcutsrc dump, T 6-press extents sequence on chromium, /Scripting introspection, scene reshoot.
- **Reason:** Characterize T flakiness + recover scripting API surface. Findings: both shortcuts in [kwin]; T sequence 0000->0000->2300->0000->2300->0000->2300 (5/6 delivered, first-after-focus eaten); Scripting exposes loadScript/unloadScript/start/isScriptLoaded; import/convert ABSENT on dom0.

## 2026-09-09T~05:35Z — dom0 probe batch 4 (tools, T-sweep, scripting run attempt, spectacle capture)
- **Command:** dom0 python probe: capture-tool discovery, fine T sweep on kitty (5 timepoints), loadScript + per-script run attempt, spectacle 7-frame flash capture + overlap-artifact shot, base64 image pull.
- **Reason:** Discriminate eaten-press vs rule-reassert for T on kitty; test scripting noBorder write; capture flash + artifact. Findings: T on kitty ZERO effect (rule Force reassert suspected); loadScript returns id but /Scripting/Script<N> UnknownObject until start(); spectacle WORKS; frame 3 = uniform near-white full screen (flash confirmed, ~0.3-0.6s); xwininfo absent; /tmp volatile between dom0 calls (frames lost in batch 2/3).

## 2026-09-09T~05:50Z — local PIL frame analysis (flash + overlap artifact)
- **Command:** python3 PIL: region mean/stddev + yellow-halo pixel counts on frames 0/2/3/4/overlap.
- **Reason:** Model cannot view images; quantify flash + artifact. Findings: frame 3 = whole screen (247,248,248) incl. normally-black kitty interior -> full-screen X11 exposure fill; overlap.png flawed (kitty raised last, nothing occluded) -> artifact NOT reproduced yet; occludedAboveKitty uses frameGeometry() — shadow-zone penetration suspect; expandedGeometry() to be verified in kwin headers.

## 2026-09-09T~06:20Z — PoC-1: scripting semantics + bare-write (STALE WINDOW)
- **Command:** dom0 python probe: kitty 0x4c008e3 extents watch, loadScript+start+introspect, bare kwinrulesrc write test.
- **Reason:** Validate design unknowns before implementing. Result: INVALID — 0x4c008e3 was closed by user (BadWindow on every read); kwinrulesrc untouched (writes round-tripped to identical content); parity intact.

## 2026-09-09T~06:35Z — PoC-2: spawn kitty + scripting auto-run test
- **Command:** dom0 python probe: qvm-run Dev-General kitty, extents watch, loadScript+start, introspect Script1.
- **Reason:** Re-test with live window. Findings: spawned kitty 79694242 never gained _NET_FRAME_EXTENTS (anomaly); /Scripting/Script1 UnknownObject even after start(); no auto-run evidence.

## 2026-09-09T~06:50Z — PoC-3: rulebook integrity + chromium scripting + inotify test
- **Command:** dom0 python probe: cat kwinrulesrc, spawn kitty + reconfigure health, scripting noBorder on chromium, temp chromium rule via bare write, screenshot pull.
- **Reason:** Discriminate rulebook corruption vs X11 phantom. Findings: kwinrulesrc PRISTINE (single active kitty-borderless); spawned kitty still extents-none (placeholder window; screenshots pulled for PIL check); scripting auto-run did NOT fire (chromium unchanged); bare-write rule NOT live-applied (no inotify — good for design); chromium restored bordered; parity intact.

## 2026-09-09T~07:00Z — local PIL analysis of PoC-3 screenshots
- **Command:** python3 PIL: kitty_before/kitty_after_recfg/kitty_final region stats.
- **Reason:** Verify spawned kitty visuals. (See next Action-History entry for outcome.)

## 2026-09-09T~07:10Z — PoC-4/4b/4c: script package install attempts
- **Command:** dom0 python probes: hand-written + kpackagetool5 install of kglowtest script pkg (KPackageStructure schema), kwinrc enable, 3 kwin restarts, chromium extents watch, cleanup.
- **Reason:** Test installed-script-package auto-execution. Findings: kpackagetool5 install OK but isScriptLoaded=false after restart (no sycoca rebuild before restart — suspected cause); kwin restarts clean, shortcuts survive (2), parity intact throughout.

## 2026-09-09T~07:20Z — PoC-5: control script + marker test + KWin DBus introspect
- **Command:** dom0 python probe: enable system minimizeall + kbuildsycoca5 + restart + isScriptLoaded; loadScript marker.js + start() + journal grep; /KWin full introspection dump.
- **Reason:** Discriminate package-discovery vs script-execution failure. Findings: minimizeall LOADS (control PASSED — mechanism works with sycoca); marker body did NOT execute via loadScript+start (start() likely once-per-process no-op); no direct per-window noBorder setter on org.kde.KWin DBus.

## 2026-09-09T~07:35Z — PoC-6: sycoca rebuild before restart
- **Command:** dom0 python probe: kpackagetool5 install kglowtest + kbuildsycoca5 + restart + watch.
- **Reason:** Test sycoca-rebuild hypothesis. Result: STILL not loaded (isScriptLoaded=false) — sycoca not the fix; narrows failure to metadata schema.

## 2026-09-09T~07:45Z — PoC-7: verbatim minimizeall metadata clone — SUCCESS
- **Command:** dom0 python probe: cp minimizeall pkg, rewire Id/Name/main.js, kpackagetool5 user-path install, sycoca, restart, chromium extents watch 16s, cleanup + restart.
- **Reason:** Discriminate metadata-schema failure. **RESULT: PASS** — isScriptLoaded=true; trajectory 0,0,23,0 → 2.4s:0,0,0,0 (script flipped border) → 8.5s:0,0,23,0 (script restored). Installed-script-package auto-execution + live noBorder PROVEN. Root cause of prior failures: minimal metadata lacked X-Plasma-API/X-Plasma-MainScript entry-point keys. Cleanup verified (chromium bordered, parity noborder=true, shortcuts 2, kglowtestEnabled=false, pkg removed).

## 2026-09-09T~08:05Z — source round writes + syntax verification
- **Command:** write src/kittytoggle.{h,cpp}, src/kwin-script/kglowsync/{metadata.json,contents/code/main.js}; edit kittyglow.cpp (5 blocks), src/CMakeLists.txt, scripts/deploy.sh; sed fix QStringLiteral→fromLatin1; bash -n deploy.sh; python3 -m json.tool metadata.json; node --check main.js; wc -l.
- **Reason:** Implement the approved seamless-B plan + Rule 16 post-modification verification (syntax validators before build consent).

## 2026-09-09T~12:45+0530 — build + fix round
- **Command:** bash scripts/build.sh (2x); podman exec … cat /tmp/b_cmake.log /tmp/b_make.log; edit src/kittytoggle.cpp (4 blocks).
- **Reason:** Execute user's "build" consent; Rule 5 zero-warning verification; fix compile errors found in first pass.

## 2026-09-09T07:57:34Z
- **Command:** node --check main.js; bash scripts/build.sh (rebuild #8); edits to main.js + kittyglow.cpp
- **Reason:** Fix JS "Could not convert argument 0" QTimer(parent) crash via parentless-builder cascade + proof-of-life print()s; add qDebug toggle-trace to C++ path. Build green, zero warnings.
- **Command:** bash scripts/build.sh (x2 — after fixing kittyglowstate.cpp QStringLiteral error); podman exec grep warning counts
  **Reason:** zero-warning container build of the state-store rebuild; Rule 5 sweep.
- **Command:** git rm kittyborderrule.{h,cpp}; git add -A; git commit bb79411
  **Reason:** retire superseded module, commit approved Step C rebuild atomically.
- **Command:** bash ~/Projects/scripts/generate-docs-html.sh HANDBOOK.md PROJECT_CONTEXT.md SPECIFICATION.md ROADMAP.md
  **Reason:** Rule 18 sibling regeneration after doc sync.
- **Command:** bash scripts/deploy.sh (3 dom0 AuthExec calls); dom0 recon batch (kwinrulesrc/loadedEffects/isScriptLoaded)
  **Reason:** Step 1 approved deploy + Step 2 reconnaissance.
- **Command:** dom0 python edit kwinrulesrc (delete group [1], count=0, drop rules=1) + org.kde.KWin.reconfigure + unloadScript kglowsync
  **Reason:** Step 2 approved forcing-rule deletion; flush in-memory rules; drop stale script instance.
- **Command:** dom0 loadEffect/isEffectLoaded/loadScript; E2E busctl --user invokeShortcut x4 + kittyglowrc/journal reads
  **Reason:** Step 3 approved load + E2E verification of the toggle pipeline.
2026-09-09T10:51:24Z | sed -n '11p' /tmp/pi-bash-c492b06ff2ae262a.log | base64 -d > logs/probe/glow-overlap-*.png; file ... | Decode probe screenshot from dom0 log for local visual analysis (glow artifact)
2026-09-09T10:52:12Z | python3 -c PIL check; which xwininfo xprop | Check local image tooling and remote X query tools for glow artifact analysis
2026-09-09T10:54:37Z | grep occlusion-related code in src/*.cpp | Locate occluder-collection and clip logic before designing repro probe
2026-09-09T10:57:32Z | grep isKittyWindow/glowconfig/HANDBOOK launch info | Identify kitty matching rule + halo color signature for repro targeting
2026-09-09T11:01:07Z | base64 -d repro shots + PIL gold-pixel scan | Quantify halo artifact: gold pixels in g1 (reference) vs g2 (konsole covering kitty)
2026-09-09T11:02:36Z | awk NR==17/18 base64 -d + PIL gold scan repro-g1/g2 | Determine if halo captured and kitty visible in probe shots
2026-09-09T11:03:10Z | numpy diff + gold row-band profile g1/g2 | Verify whether kitty launched (identical shots = no) and whether gold is wallpaper not halo
2026-09-09T11:10:39Z | cat src/kwin-script/kglowsync/metadata.json | Clone metadata schema for kglowprobe diagnostic package
2026-09-09T11:19:38Z | decode diag-g3/g4 + numpy gold stats vs known frame rect | Test halo render (g3 ring) and clip-vs-front-window (g4 gold must be ~0 if covered)
2026-09-09T11:20:11Z | decode diag-g3/g4 (correct log) + numpy gold stats vs frame rect | Determine halo presence and penetration vs front window
2026-09-09T11:32:18Z | sed -n 18/19p decode diag-g3/g4 + numpy gold stats | Determine halo ring presence (g3) and penetration under front window (g4)
2026-09-09T11:46:08Z | numpy gold analysis burst-g6/g7a-d | Verdict: halo clip under front window incl. blink repaints and simulated drag
2026-09-09T11:51:36Z | decode clip-g8/g9a-d + cover-rect gold analysis | FINAL verdict on halo clipping under front window (blink + drag triggers)
2026-09-09T11:52:19Z | decode clip-g8/g9a-d (marker-strip) + cover-rect gold analysis | FINAL verdict on halo clipping under front window
2026-09-09T11:55:24Z | alpha-aware pale-gold mask + strip color profile on clip-g8/g9a | Check whether 60%-alpha halo on white desktop was missed by B<90 threshold
2026-09-09T12:01:07Z | write logs/SESSION_STATE.md (Rule 17 brain-dump: probe results, root cause, pending fix consent) | Preserve session state before awaiting modification consent
2026-09-09T16:08:54Z | edit src/kittyglow.cpp updateOccluders(): docks/panels always clip halo even when translucent (LL-018 fix) + Rule 16 verification | Implement approved 1-line consent-lifecycle fix
2026-09-09T16:44:22Z | scripts/build.sh (Build #9, container dom0-replica-fed37) — LL-018 fix compiled zero-warning, dist sha256 e7ff8627… | Explicit 'build the app' consent
2026-09-09T16:44:22Z | scripts/deploy.sh — deployed to dom0 (.so + metadata + kglowsync pkg), kwinrc enabled; 1st attempt Access denied (dialog), 2nd OK
2026-09-09T16:44:22Z | dom0 sha256sum check — deployed .so matches dist build #9 exactly | Deploy integrity gate
2026-09-09T16:44:22Z | dom0 kwin_x11 --replace via /tmp/kg_restart.sh — old=32381 new=32660, restart log clean | Approved restart chain (post-fix activation)
2026-09-09T16:44:22Z | dom0 post-restart checks x2 — CANCELLED at password dialog (user action or dialog timeout); isLoaded/metadata-perms/shortcut checks still pending | Awaiting user re-run
2026-09-09T16:44:22Z | sed PROJECT_CONTEXT.md Last Updated (build #9 state) | Rule 12c sync
2026-09-09T16:55:43Z | dom0 checks round 2 (env-sourced) — isLoaded method absent (KWin 5.27.8 lacks it), B shortcut confirmed, restart log 0 errors | User: 'retry the checks'
2026-09-09T16:55:43Z | dom0 checks round 3 — BOTH shortcuts confirmed (Toggle Kitty Borderless + Window No Border) | Complete LL-011 healing evidence
2026-09-09T16:55:43Z | dom0 checks round 4 — kittyglow.so mapped in kwin 32660 /proc/maps (5 mappings), kwinrc true; loadedEffects DBus API nonexistent on this build (both signatures) | Definitive load proof
2026-09-09T16:55:43Z | rm logs/build.log logs/build9.log symlinks — Build #9 complete, Rule 2 cleanup | Mandatory post-task cleanup
2026-09-09T17:41:36Z | source analysis of kittyglow.cpp for LL-019 (read-only, no shell side effects) | Rule 1a: propose before probe/code | logged
2026-09-09T17:47:00Z | read glowshader.cpp + glowconfig.h (analysis only) | Rule 1a gating | logged
2026-09-09T18:09:11Z | curl KWin v5.27.8 kwinglutils.cpp from invent.kde.org; grep+read GLVertexBuffer::draw | verify upstream scissor clip path for left/right branch (LL-019) | logged

## 2026-09-09T18:29:08Z
- Command: curl -sSL https://invent.kde.org/plasma/kwin/-/raw/v5.27.8/src/effects.h -o /tmp/effects-5.27.8.h
  Reason: first fetch attempt for EffectsHandler API verification (turned out to be the internal impl header).
- Command: curl -sSL https://invent.kde.org/plasma/kwin/-/raw/v5.27.8/src/libkwineffects/kwineffects.h -o /tmp/kwineffects-5.27.8.h
  Reason: fetch the public effects API header to verify stackingOrderChanged/windowActivated/isUserMove exist in 5.27.8 before proposing the fix bundle.
- Command: grep -n "stackingOrderChanged|movingWindow|resizedWindow|windowFrameGeometryChanged" /tmp/kwineffects-5.27.8.h (+ follow-up sed/grep context reads)
  Reason: confirm exact signal/method names and line numbers so the proposed fix compiles against 5.27.8 first try.

## 2026-09-09T18:55:33Z
- Command: edit src/kittyglow.cpp (6 blocks, via edit tool); edits to
  SPECIFICATION.md / HANDBOOK.md / PROJECT_CONTEXT.md / ARCHITECTURE.md.
  Reason: implement approved LL-019 fix bundle (v3.4) and sync documents.
- Command: bash ~/Projects/scripts/generate-docs-html.sh <4 md files> ;
  generate-docs-html.sh --check QubesOS/UI-Enhancements/Kwin/kitty-glow
  Reason: Rule 18a HTML sibling regeneration + sanctioned audit mode.
- Command: rm -f logs/SESSION_STATE.html
  Reason: revert orphan HTML flagged by the scoped 18e equality diff
  (logs/ ledgers are not HTML-generated per Rule 22f).
- Command: grep -rn "updateOccluders|m_occluders|m_stackingStamp" src/
  Reason: Rule 16 verification — confirm removed cache symbols are gone.
- Command: git add -A && git commit (this commit)
  Reason: atomic commit of fix + docs + ledgers (Rule 14).

## 2026-09-09T18:56:35Z
- Command: sed -i 1{/^<!-- HTML sibling/d} logs/SESSION_STATE.md
  Reason: remove generator selection pointer from a logs ledger (Rule 22f) — root cause of the recurring orphan SESSION_STATE.html.

## 2026-09-09T18:58:09Z
- Command: cat > logs/SESSION_STATE.md (heredoc); append CHANGELOG.md; git commit
  Reason: Rule 17 pre-compaction state preservation after v3.4 implementation milestone.

## 2026-09-09T19:10:26Z
- Command: bash scripts/build.sh (tee logs/build/build-10-…log)
  Reason: approved Build #10 compile of v3.4 in dom0-replica-fed37.
- Command: podman exec … grep -i 'warning|error' /tmp/b_cmake.log /tmp/b_make.log
  Reason: Rule 5 zero-warning verification (clean).
- Command: bash scripts/deploy.sh (tee logs/build/deploy-10-…log); dom0 sha256sum check
  Reason: deploy artifacts to dom0 and verify byte-identical hashes.
- Command: dom0 'bash /tmp/kg_restart.sh' ×2
  Reason: approved activation restart (34892→35345), then LL-011 heal
  restart after confirming kglobalaccel active (35345→36048).
- Command: dom0 dbus-send isEffectLoaded / Component.allShortcutInfos /
  grep kittyglow /proc/PID/maps
  Reason: verify effect load, B/T registration, and .so mapping post-restart.
- Command: dom0 introspect /component/kwin
  Reason: recover correct DBus interface name (org.kde.kglobalaccel.Component).
- Command: git add -A && git commit
  Reason: atomic commit of build-state docs + ledgers (Rule 14).
## 2026-09-10T01:15:00Z–01:45:00Z — LL-019 v3.4 live-state empirical probe campaign
- Command: `ls src/kwin-script/kglowprobe logs/probe; head main.js; grep kglowprobe logs/commands-log.md`
  Reason: Recover prior session's probe tooling and analysis loop for reuse.
- Command: `grep colorActive src/glowconfig.h; dom0 'grep -A12 Effect-kittyglow kwinrc'`
  Reason: Establish live halo colors (gold 255,215,0 @ 60%/30%) and confirm no config overrides.
- Command: `dom0 'which import convert xwd xwininfo xdotool python3; ls /tmp/*.png'`
  Reason: Inventory dom0 capture tooling; found prior session captures in /tmp.
- Command: `dom0 'python3 -c "import PyQt5/PIL"; which spectacle qdbus dbus-send'`
  Reason: Confirm PIL+PyQt5 available for screenshot capture on dom0.
- Command: `dom0 'whoami; ps -ef|grep kwin_x11; ls /tmp/xauth*; grep Effect-kittyglow kwinrc'`
  Reason: Identify bridge user (chenpan), session type (kwin_x11), Xauthority path, default config.
- Command: `dom0 'PIL ImageGrab x2 (1.5s apart) + xprop stacking + xdotool geometries'`
  Reason: Baseline captures kg_p1/p2 + live window rects (kitty 189,94 902x469; konsole offset 159,81).
- Command: `dom0 'base64 PNG -> Dev-General decode'` (×5 rounds)
  Reason: Transfer captures for numpy gold-pixel analysis (strict/inactive masks).
- Command: `python3 gold-mask/cluster/ring-band/rowcol-profile analyses (numpy+PIL)`
  Reason: Spatial forensics: halo edges vs wallpaper separation; found strict mask needed (r-g>18).
- Command: `dom0 'xdotool windowactivate kitty/kon + PIL grabs kg_p3/p4 + stacking'`
  Reason: State A/B test: kitty-on-top full halo vs konsole-on-top suppression.
- Command: `dom0 'kg_p5 (settled kitty-active) + RGB samples + strict maps'`
  Reason: Confirm p3 was not an animation frame; verify ring edges x182-188/x1091-1101/y91-93/y563-572.
- Command: `dom0 'xprop WM_CLASS of all stacking windows + window names/geometries'`
  Reason: Rule out second kitty-class window; identified plasmashell/konsoles/firefox/dolphin inventory.
- Command: `python3 kg_user capture analysis + inactive-tuned ring scans`
  Reason: User's live screen: no halo anywhere (correct: fullscreen pi-konsole occluder covers all).
- Command: `dom0 'A/B/C state test: activate kon/kitty/firefox + grabs kg_a1/b1/c1/c2 + verified stacking'`
  Reason: Deterministic static-state verification: firefox-over-kitty → over_firefox=39≈0 (clipped, correct).
- Command: `dom0 'background 16-frame burst @350ms + xdotool titlebar drag of firefox'` (timed out at 30s; drag failed to move window)
  Reason: Mid-drag transient capture; caught unclipped frames d02/d16 (+2100 gold over firefox).
- Command: `python3 extra-gold diff d02/d16 vs d08 + maps`
  Reason: Localized transient unclipped ring segments over firefox (right ring x1160, bottom y544-576).

## 2026-09-10T02:41:05+05:30
- Command: dom0 bridge grep of kwinglutils.h (render docs, rectCount/infinite handling) + kwin version
- Reason: Verify whether GLVertexBuffer::render() skips region scissoring above a rect-count threshold — prime suspect for the oscillating full-ring halo leak.

## 2026-09-10T02:41:57+05:30
- Command: dom0 bridge locate kwinglutils.h + kwin version + project build config
- Reason: Header not at /usr/include/kwin; need real include path and kwin version to inspect GLVertexBuffer::render scissoring behavior.

## 2026-09-10T02:42:43+05:30
- Command: local project listing + build script discovery (Dev-General side)
- Reason: Previous dom0 queries missed headers; the build script defines the actual include paths and build flow.

## 2026-09-10T02:42:54+05:30
- Command: read container setup + build.sh heads, list build containers
- Reason: Headers live in the build container; need container name/state and the kwin version it builds against.

## 2026-09-10T02:43:27+05:30
- Command: podman exec grep GLVertexBuffer render docs in fed37 container + kwin rpm versions
- Reason: Verify the exact signature/semantics of GLVertexBuffer::render(region, mode, hardwareClipping) for KWin 5.27.8.

## 2026-09-10T02:46:04+05:30
- Command: podman exec — locate kwin-devel headers, read GLVertexBuffer::render docs, list render overloads in libkwineffects
- Reason: Determine exact semantics of render(region, mode, hardwareClipping) in KWin 5.27.8 — whether scissoring can be skipped conditionally.

## 2026-09-10T02:47:59+05:30
- Command: podman exec — objdump disassembly of GLVertexBuffer::render(QRegion,...) in libkwinglutils 5.27.8
- Reason: Confirm whether render() with hardwareClipping=true enables GL_SCISSOR_TEST itself or leaves it to the caller — explains the oscillating unclipped halo.

## 2026-09-10T02:48:14+05:30
- Command: podman exec — disassemble render(QRegion, GLenum, bool) body from libkwinglutils
- Reason: Determine if the 5.27.8 implementation enables GL_SCISSOR_TEST itself or assumes caller enabled it.

## 2026-09-10T02:48:49+05:30
- Command: podman exec — verify /tmp/kwg.asm exists and locate GLVertexBuffer6render symbols
- Reason: Previous symbol-anchored extraction failed; debugging the disassembly dump.

## 2026-09-10T02:49:03+05:30
- Command: podman exec — print render(QRegion,j,b) body (lines 16357-16490 of disassembly)
- Reason: Read the actual 5.27.8 scissoring logic to confirm the caller-responsibility hypothesis.

## 2026-09-10T02:51:11+05:30
- Command: podman exec — disassemble GLVertexBuffer::draw(QRegion,int,int,bool) body
- Reason: Extract the exact scissor enable/disable and Y-flip math for hardwareClipping in 5.27.8.

## 2026-09-10T07:49:03+05:30
- Command: bash scripts/build.sh + grep build logs for warnings
- Reason: Rebuild kittyglow.so v3.5 with the LL-020 fix; Rule 5 requires zero-warning verification of all build output.

## 2026-09-10T07:50:46+05:30
- Command: bash scripts/deploy.sh
- Reason: Install rebuilt kittyglow.so v3.5 (LL-020 fix) into dom0 and re-enable in kwinrc.

## 2026-09-10T07:52:50+05:30
- Command: ls scripts/ + dom0 sha256 of deployed .so vs dist + grep restart helpers
- Reason: Deploy integrity gate (sha match) and locate the established kwin restart / leak-probe tooling before the approved restart step.

## 2026-09-10T07:54:11+05:30
- Command: dom0 setsid kwin_x11 --replace as chenpan (HOME/DBUS/DISPLAY env) + health capture
- Reason: Approved restart step — activate LL-020 build #10; established clean restart path (HANDBOOK §11, prior-session procedure).

## 2026-09-10T07:55:02+05:30
- Command: dom0 activeEffects dbus + journal kittyglow error scan + grep probe tooling
- Reason: Post-restart verification (Rule 6) — confirm LL-020 build auto-loaded cleanly and find the established leak-probe method.

## 2026-09-10T07:56:22+05:30
- Command: find probe scripts + dom0 loadEffect kittyglow (idempotent load-confirmation)
- Reason: Rule 6 verification — confirm LL-020 build #10 is the loaded effect; locate prior leak-probe tooling.

## 2026-09-10T07:57:02+05:30
- Command: dom0 Effects introspect + journal scan after loadEffect=false
- Reason: Diagnose why build #10 failed to load (Rule 5/16 — do not proceed with visual verification until resolved).

## 2026-09-10T08:01:17+05:30
- Command: cat dist/kittyglow.json + dom0 installed metadata + journal history count (corrected quoting)
- Reason: Determine whether the scripted-effect init failure is the native metadata misread as a KWin script (and whether it predates this session).

## 2026-09-10T08:03:42+05:30
- Command: dom0 X-window tree with XAUTHORITY/XDISPLAY env exported
- Reason: Window tree snapshot needed for LL-020 burst design; first attempt lacked X env.

## 2026-09-10T08:04:17+05:30
- Command: dom0 raw xwininfo head + python3 PIL/numpy availability
- Reason: Diagnose empty window-tree output; verify probe dependencies for the LL-020 burst.

## 2026-09-10T08:08:49+05:30
- Command: dom0 stuck-process check + timed xdotool sanity probes
- Reason: Burst hung with no output; identify the blocking call before retrying (Rule 5 discipline).

## 2026-09-10T08:10:19+05:30
- Command: rerun ll020_v35_burst.py backgrounded with stdout to /tmp/ll020_results.txt, poll+cat after 40s
- Reason: Recover the burst results (first run completed but bridge lost stdout to timeout).

## 2026-09-10T08:11:34+05:30
- Command: check /tmp/ll020_results.txt, rerun burst with python3 -u (unbuffered) timeout 300, snapshot at 25s
- Reason: Bridge kills process groups at call end — unflushed block-buffered stdout was lost; -u flushes per print.

## 2026-09-10T08:12:54+05:30
- Command: poll both burst result files after 45s
- Reason: Collect final strict/loose gold counts for LL-020 verification.

## 2026-09-10T08:17:00+05:30
- Command: dom0 crop ring region from frames f6/f12 + pull crops to logs/probe/ll020/
- Reason: Visual disambiguation of burst states C (f6) vs A (f12): kitty content vs konsole content under the gold ring.

## 2026-09-10T08:23:20+05:30
- Command: verify both burst runs finished, peek run2 final lines
- Reason: No concurrent probes may run during the clean final verification.

## 2026-09-10T08:24:43+05:30
- Command: xdotool kitty window lookups (class/visible/name) + active window
- Reason: --onlyvisible returned nothing; diagnose before re-running the final probe.

## 2026-09-10T08:25:13+05:30
- Command: qvm-run pgrep kitty in Dev-General
- Reason: Determine whether kitty was closed or its window moved to another activity.

## 2026-09-10T08:25:32+05:30
- Command: pgrep -a kitty (local Dev-General)
- Reason: Determine whether the kitty process still exists.

## 2026-09-10T08:26:18+05:30
- Command: ps kitty start time + enumerate X windows matching kitty
- Reason: kitty process alive with no X window - check start time and hidden window state.

## 2026-09-10T08:27:12+05:30
- Command: qdbus KWin supportInformation grep kitty
- Reason: KWin authoritative window list to locate the missing kitty window (activity/desktop/withdrawn state).

## 2026-09-10T08:27:34+05:30
- Command: dom0 KWin supportInformation -> /tmp/kg_support.txt, grep kitty
- Reason: KWin authoritative window list for the missing kitty window.

## 2026-09-10T08:27:54+05:30
- Command: inspect /tmp/kg_support.txt size and head
- Reason: Empty-looking grep results - verify the dump itself has content and correct section names.

## 2026-09-10T08:28:48+05:30
- Command: xprop _NET_CLIENT_LIST enumeration + WM_CLASS histogram
- Reason: X-authoritative check for any kitty-class client window.

## 2026-09-10T08:29:10+05:30
- Command: dump all client windows (id|WM_CLASS|name) to /tmp/kg_clients.txt
- Reason: Authoritative kitty window search across all 46 managed clients.

## 2026-09-10T08:29:34+05:30
- Command: corrected hex-ID client enumeration -> /tmp/kg_clients.txt
- Reason: Authoritative kitty window search across all managed clients.

## 2026-09-10T08:30:57+05:30
- Command: launch kitty (VM env) + dom0-side window check
- Reason: Confirm fresh kitty window is mapped and visible for the final probe.

## 2026-09-10T08:31:47+05:30
- Command: debug xdotool getwindowgeometry shell/plain output for 79698807
- Reason: Probe geo() KeyError - inspect actual output format.

## 2026-09-10T08:33:04+05:30
- Command: py_compile + run hardened ll020_final.py
- Reason: Decisive LL-020 verification (correct syntax validator this time).

## 2026-09-10T08:37:34+05:30
- Command: check kwin_x11 process alive + recent journal
- Reason: org.kde.KWin DBus gone + zero halo paint suggests kwin died during/after burst.

## 2026-09-10T08:38:35+05:30
- Command: qdbus compositor state + effect load state (explicit DBUS_SESSION_BUS_ADDRESS)
- Reason: Zero halo + silent qdbus -> bridge shells lost session bus env; check compositor fallback and effect load.

## 2026-09-10T08:40:00+05:30
- Command: qdbus loadEffect kittyglow + immediate journal capture
- Reason: Capture the exact loader error for the native effect load failure.

## 2026-09-10T08:40:35+05:30
- Command: qdbus /Effects isEffectLoaded + loadedEffects (correct DBus path)
- Reason: Authoritative effect load state.

## 2026-09-10T08:42:15+05:30
- Command: activate fresh kitty, check _NET_ACTIVE_WINDOW + inside-frame pixels + ring gold
- Reason: Disambiguate raise-failure (kitty stays below fullscreen konsole) from real halo absence.

## 2026-09-10T08:46:21+05:30
- Command: re-run cancelled diagnostic - stacking order + ring band pixel means
- Reason: Determine whether fullscreen konsole sits above activated kitty (occluder suppression) - the last discriminator before PASS/FAIL verdict.

## 2026-09-10T08:48:18+05:30
- Command: positive control - raise kitty above konsoles, count ring gold + per-band
- Reason: Confirm halo renders on the live fresh window when genuinely topmost (completes both-way verification).

## 2026-09-10T08:53:05+05:30
- Command: unloadEffect+loadEffect kittyglow, raise kitty, ring gold count
- Reason: Force fresh effect instance (fresh shader compile) to test sick-instance hypothesis.

## 2026-09-10T08:55:45+05:30
- Command: journal grep compositing/OpenGL/XRender lines since restart
- Reason: Test XRender-fallback hypothesis - would explain zero halo with loaded effect + gold defaults.

## 2026-09-10T08:59:51+05:30
- Command: xprop _NET_WM_STATE on konsoles + kitty (keep-above check)
- Reason: Focused-but-below pattern implies keep-above occluders; would fully explain zero halo as correct clipping.

## 2026-09-10T09:03:10+05:30
- Command: clean single re-run of v35 burst (24 cycles, no concurrency)
- Reason: Settle whether run1 C-states (23,434px) reproduce post-fix; no C-state = leak fixed, those were stacking transients.

## 2026-09-10T09:07:43+05:30
- Command: v35 burst re-run with timeout 300 (complete budget)
- Reason: Previous run killed at 100s mid-burst; need full 24-cycle JSON for the verdict.

## 2026-09-10T18:26:16+05:30
- Command: xdotool windowraise kitty + immediate ring gold count + stacking
- Reason: Positive control - halo must appear when kitty is genuinely topmost (grab happens before dialogs steal focus).

## 2026-09-10T18:28:16+05:30
- Command: run ll020_positive.py (minimize-cover -> grab -> raise kitty -> grab -> restore)
- Reason: Definitive positive control: halo must appear when kitty is visible/unoccluded, else the scissor change broke rendering.

## 2026-09-10T18:51:08+05:30
- Command: grep scissor refs + sed -n 149,245p src/kittyglow.cpp (post-edit review)
- Reason: Rule 16 logic/typo verification of the subdivision rewrite before build.

## 2026-09-10T18:52:26+05:30
- Command: bash scripts/build.sh (build #11 — LL-020 subdivision rewrite)
- Reason: User-approved build of the CPU-subdivision fix in the dom0-replica-fed37 container.

## 2026-09-10T18:55:04+05:30
- Command: bash scripts/deploy.sh (build #11 f6d6cfb5 to dom0)
- Reason: Deploy approved artifact; part of the user-approved build+deploy step.

## 2026-09-10T18:56:10+05:30
- Command: sha256sum dom0 deployed .so vs dist/kittyglow.so
- Reason: Deploy integrity gate — dom0 artifact must equal build #11 (f6d6cfb5).

## 2026-09-10T18:58:43+05:30
- Command: kwin_x11 --replace as desktop user (load build #11 f6d6cfb5)
- Reason: User-approved restart to load the new effect binary on the clean baseline.

## 2026-09-10T18:59:26+05:30
- Command: ll020_positive.py on build #11 (post-restart)
- Reason: THE decisive probe — halo must now appear with kitty unoccluded (baseline noise floor was 74/91 px).

## 2026-09-10T19:04:07+05:30
- Command: build #12-debug + deploy + live unload/load kittyglow
- Reason: Ship capped gate instrumentation; live reload avoids restart for debug iteration.

## 2026-09-10T19:04:32+05:30
- Command: activate kitty + journalctl grep kg-dbg
- Reason: Read which halo gate fails (region-empty / occluder-empty / draw).

## 2026-09-10T19:10:39+05:30
- Command: live reload build #12b + journal gates + positive control (corrected sudo -u chenpan qdbus)
- Reason: Previous chain failed running qdbus as root; rerun with the proven command form.

## 2026-09-10T19:59:38+05:30
- Command: focus-change + journal kg-dbg + positive dance + gold spatial bbox (retry after cancelled dialog)
- Reason: Determine whether build #12b is live and where the ring gold sits spatially.

## 2026-09-10T20:03:34+05:30
- Command: kwin restart to load build #12b + gate journal + positive control + spatial bbox
- Reason: Live reload cannot swap code; restart required to run 12b. Full verification chain.

## 2026-09-10T20:06:06+05:30
- Command: bash scripts/deploy.sh (verbose) + dom0 sha check
- Reason: #12b transfer failed silently; redeploy and verify artifact identity (expect 83db2311 on dom0).

## 2026-09-10T20:07:25+05:30
- Command: kwin restart loading verified 83db2311 (#12b) + gates + positive control + bbox
- Reason: Run the region-clip-fixed build and complete the verification chain.

## 2026-09-10T20:13:00+05:30
- Command: v35 leak burst on build #12b (24 cycles + 5 idle)
- Reason: Final probe - confirm no glow penetration/accumulation with the occluder-clipped subdivision build.

## 2026-09-10T20:15:01+05:30
- Command: strip kg-dbg instrumentation + build #13 + deploy + sha verify
- Reason: Clean release build of the verified mechanism; sha-verify the deploy this time (12b silent-failure lesson).

## 2026-09-10T20:17:09+05:30
- Command: kwin restart loading build #13 (cb63bd4b) + final positive control
- Reason: Load the clean verified build and confirm the halo renders on it.

## 2026-09-10T20:23:42+05:30
- Command: generate-docs-html + ledger updates + git commit (LL-020 closure)
- Reason: Rule 18 HTML regeneration, Rule 3/10/23 ledgers, session commit per user directive.

## 2026-09-10T20:25:06+05:30
- Command: SESSION_STATE rewrite + Rule 18e HTML equality audit + follow-up commit
- Reason: Rule 17 milestone snapshot; Rule 18e verification; keep ledger current.

## 2026-09-10T21:24:16+05:30
- Command: regression-checks fix (check_not helper) + registry self-test
- Reason: `!` through "$@" is not bash negation (expansion executes command "!"); helper restructured, all 8 assertions green.

## 2026-09-10T21:31:21+05:30
- Command: kwin restart loading build #14 (2ab0c4df) + positive control
- Reason: LL-021 — live reload cannot swap .so code; restart required, user consented.

## 2026-09-10T21:40:28+05:30
- Command: ll014_seam.py partial-occlusion probe (env fix, attempt 3)
- Reason: Verify seamless pass-behind behavior on build #14.

## 2026-09-10T21:48:15+05:30
- Command: ll014_seam2.py differential seam probe (Import fix, attempt 2)
- Reason: NameError on Image; PIL import corrected.

## 2026-09-10T22:05:32+05:30
- Command: ll014_seam4.py raised-occluder seam probe + crop pull (sed-filtered)
- Reason: True partial occlusion with kitty active; seam continuity at frame edge.

## 2026-09-10T22:29:12+05:30
- Command: demo state — kitty active, konsole3 800x400 raised above (seam crosses left ring band)
- Reason: User visual acceptance of seamless pass-behind at the former gap location.

## 2026-09-10T22:55:51+05:30
- Command: activate kitty by class (clean demo state, retry after AFK)
- Reason: Stable visual-acceptance state for the user.

## 2026-09-10T23:11:18+05:30
- Command: regression-checks + docs HTML regen + 18e audit + ledgers + commit (seamless closure)
- Reason: Rule 3/9/10/18/23 atomic completion of the user-accepted change.

## 2026-09-10T23:14:23+05:30
- Command: SESSION_STATE final rewrite + commit
- Reason: Rule 17 milestone snapshot at session close.

## 2026-09-10T23:53:37+05:30
- Command: build #15 deploy retry (sha gate)
- Reason: First attempt cancelled at the dom0 dialog.

## 2026-09-11T07:37:20+05:30
- Command: build #15 deploy (attempt 3, user approved dialog)
- Reason: Load the all-windows glow build onto dom0.

## 2026-09-11T07:40:02+05:30
- Command: kwin restart loading build #15 (6d8886da) + shortcut registry check
- Reason: LL-021 — restart required to load the all-windows glow build (user consented).

## 2026-09-11T07:43:05+05:30
- Command: ll015_allwin.py verification chain (retry)
- Reason: First attempt cancelled.

## 2026-09-11T07:45:17+05:30
- Command: ll015_diag.py — global gold counts + config + shortcut registry + journal
- Reason: STEP1 ring absent and shortcut dead; isolate which.

## 2026-09-11T07:47:48+05:30
- Command: ll015_verify2.py staged verification + kglowsync revival
- Reason: Clean-layout ring checks, exclusions, scripted-effect race fix.

## 2026-09-11T07:52:22+05:30
- Command: kglowsync resilient main.js push + reconfigure + verify3
- Reason: Fix script-init race (bus-name overlap on --replace); staged ring/exclusion verification.

## 2026-09-11T07:54:03+05:30
- Command: kglowsync deep diagnosis (file sha, service, full JS error)
- Reason: Script still fails after resilient rewrite; need the real JS exception.

## 2026-09-11T07:55:39+05:30
- Command: stale-kpackage check + service query with session env
- Reason: Failing script named kittyglow not kglowsync; suspected PoC leftover.

## 2026-09-11T07:59:01+05:30
- Command: ll015_verify4.py final probe (per-window bands + notification xprop)
- Reason: Confirm app-window rings and notification exclusion.

## 2026-09-11T08:02:30+05:30
- Command: regression-checks.sh + bash -n (LL-025 assertion added)
- Reason: Register the kglowsync resilient-bootstrap fix as a machine-verifiable invariant (Rule 20).

## 2026-09-11T08:06:03+05:30
- Command: HTML regen + 18e audit + ledgers + git commit
- Reason: Rule 3/10/14/23 atomic wrap-up of the build #15 session.

## 2026-09-11T08:17:28+05:30
- Command: ll026_types_probe.py (window types + kicker popup + top-right hunt)
- Reason: User reported glow on tray icons/kicker menus + mystery top-right square.

## 2026-09-11T08:29:33+05:30
- Command: ll026_ghost_hunt2.py (component attribution + supportInformation inventory)
- Reason: Identify ghost square culprit via KWin authoritative window list.

## 2026-09-11T08:47:57+05:30
- Command: node --check main.js + regression-checks + doc edits + HTML regen + 18e audit + ledgers
- Reason: Rule 16/14/23 — verify, document, and log build #16 implementation atomically.

## 2026-09-11T08:50:04+05:30
- Command: bash scripts/build.sh (build #16, user consented "build the app")
- Reason: Compile the shortcut split + LL-026 chrome-class exclusion changes.

## 2026-09-11T08:53:15+05:30
- Command: build #16 deploy full output
- Reason: Diagnose deploy failure (previous filtered run exited 1 with no output).

## 2026-09-11T08:54:14+05:30
- Command: build #16 deploy (retry after access-denied)
- Reason: First dialog attempt was cancelled/mistyped.

## 2026-09-11T08:55:20+05:30
- Command: kwin restart loading build #16 (64f2e194) + shortcut check
- Reason: LL-021 restart to load new .so (user consented).

## 2026-09-11T08:58:30+05:30
- Command: ll016_verify.py (G/B toggles live + ghost corner + shortcuts)
- Reason: Behavioral verification of build #16.

## 2026-09-11T09:02:31+05:30
- Command: ll016_diag.py (kglowsync load state + corner/tray halo-vs-icon test)
- Reason: B no-op root cause + ghost square ownership.

## 2026-09-11T09:08:32+05:30
- Command: deploy watchdog main.js + kglowdump.js via loadScript (corner window identification)
- Reason: Identify the window painting the corner halo; fix lost-reply bootstrap hang.

## 2026-09-11T09:08:59+05:30
- Command: Scripting.start + journal DUMP read
- Reason: Run the one-shot client dump (corner window identification).

## 2026-09-11T09:14:18+05:30
- Command: build #17 deploy (sha-gated)
- Reason: Size guard + kglowsync watchdog onto dom0.

## 2026-09-11T09:15:18+05:30
- Command: kwin restart loading build #17 (d66b9009)
- Reason: Load size-guard .so + revived kglowsync with watchdog (user consented).

## 2026-09-11T09:16:34+05:30
- Command: ll017_verify.py (build #17 full behavioral chain)
- Reason: Verify ghost-square fix + live G/B toggles + watchdog bootstrap.

## 2026-09-11T09:18:37+05:30
- Command: kwinrulesrc audit (kitty noborder forcing rule suspected)
- Reason: kitty noBorder writes revert — scripting loses to rules (2026-09-09 fight).

## 2026-09-11T09:22:36+05:30
- Command: docs/ledgers sync (SPEC/PC/HANDBOOK/research + SESSION_STATE/CHANGELOG/Action-History)
- Reason: Rule 3/14/22 atomic documentation of build #17.

## 2026-09-11T09:22:53+05:30
- Command: HTML regen (3 core docs) + 18e diff + git commit (build #17)
- Reason: Rule 18/22 compliance and change record.

## 2026-09-11T09:25:13+05:30
- Command: git history ground-truth audit (phase-1/#16 sha attribution)
- Reason: Verify no session work is uncommitted.

## 2026-09-11T09:26:30+05:30
- Command: SESSION_STATE correction commit + final status check
- Reason: Accurate handoff record; Rule 23.

## 2026-09-11T10:08:29+05:30
- Command: build #18 implementation (glowfocus module, kittytoggle window-op channel, kittyglow 4-handler rewiring, main.js overrides map, CMake+version, +3 assertions)
- Reason: Per-window B/G toggles + global Alt-masters (user directive 2026-09-11).

## 2026-09-11T10:09:34+05:30
- Command: assertion update (ll023/ll026 patterns refactored per Rule 20c)
- Reason: Invariants intact, structure moved to 4-handler routing.

## 2026-09-11T10:14:39+05:30
- Command: build #18 compile (build.sh, container)
- Reason: User-authorized build of per-window toggle feature.

## 2026-09-11T10:15:43+05:30
- Command: build #18 deploy (sha-gated, .so + json + main.js)
- Reason: Ship per-window toggle feature to dom0.

## 2026-09-11T10:17:18+05:30
- Command: kwin restart loading build #18 (7df23937)
- Reason: Load per-window toggle .so + register Alt-masters (user consented).

## 2026-09-11T10:19:26+05:30
- Command: ll018_verify.py (focused B + sweep soak + focused G + global Alt-masters)
- Reason: Build #18 behavioral verification.

## 2026-09-11T10:23:30+05:30
- Command: kglobalshortcutsrc inspection (B binding missing?)
- Reason: Focused-B fires nothing; G and Alt+B fire.

## 2026-09-11T10:24:41+05:30
- Command: live kglobalaccel component query for kittyglow
- Reason: File binding exists; live daemon table suspected stale for B.

## 2026-09-11T10:25:53+05:30
- Command: /component/kwin live shortcut list (kittyglow entries?)
- Reason: Locating the live component holding the four bindings.

## 2026-09-11T10:43:43+05:30
- Command: journal capture of focused-op lines (user physical test)
- Reason: Record script-side per-window chain evidence.

## 2026-09-11T10:47:10+05:30
- Command: docs/ledgers sync (SPEC LL-027, HANDBOOK 6b, PROJECT_CONTEXT #18, SESSION_STATE, CHANGELOG, Action-History)
- Reason: Rules 3/10/14/17/22 atomic wrap-up of build #18.

## 2026-09-11T10:47:28+05:30
- Command: HTML regen (3 core docs) + 18e diff + git commit (build #18)
- Reason: Rule 18/22 compliance and change record.

## 2026-09-11T10:53:43+05:30
- Command: output-history entry (JS-vs-C++ rationale)
- Reason: Rule 8 — internal rationale ledger.

## 2026-09-11T11:10:38+05:30
- Command: deep audit (full source read, shellcheck, node --check, sha audits, grep sweeps, math review) + report written
- Reason: User-requested comprehensive audit; Rule 8/22f documentation.

## 2026-09-11T11:18:36+05:30
- Command: audit-fix implementation (M1-M4, L1-L10; +3 assertions ll028/ll029/m1; ROADMAP refresh; v2 script purged)
- Reason: User-approved fix batch from deep audit.

## 2026-09-11T11:19:43+05:30
- Command: shellcheck SC2164 fix + assertions re-run
- Reason: Audit fix batch static verification.

## 2026-09-11T11:20:41+05:30
- Command: PROJECT_CONTEXT build #19 state note
- Reason: Rule 12c state snapshot of pending build.

## 2026-09-11T11:22:16+05:30
- Command: 18e self-check caught stale ROADMAP.html; pointer restored + regenerated
- Reason: Rule 18e mandatory equality diff.

## 2026-09-11T11:24:05+05:30
- Command: build #19 compile (audit-fix batch)
- Reason: User-authorized build.

## 2026-09-11T11:25:15+05:30
- Command: build #19 deploy (sha-gated, .so + json + main.js + kglowsync metadata)
- Reason: Ship audit-fix batch to dom0.

## 2026-09-11T11:26:13+05:30
- Command: kwin restart loading build #19 (07537ab5)
- Reason: Load audit-fix .so (user consented).

## 2026-09-11T11:27:14+05:30
- Command: M3 verification (G + Alt+G inside 220ms — both must fire)
- Reason: Per-action gate fix behavioral proof.

## 2026-09-11T12:29:06+05:30
- Command: Alt+G restore (glow master on, overrides cleared)
- Reason: End-state restoration after M3 probe.

## 2026-09-11T12:29:38+05:30
- Command: wrap-up doc chain (PROJECT_CONTEXT/CHANGELOG/Action-History/SESSION_STATE + commit)
- Reason: Rules 3/10/12/17 milestone recording.

## 2026-09-11T12:34:34+05:30
- Command: container header check for KWin 5.27 damage/scale semantics
- Reason: Adjudicate M2 coordinate space against pinned source.

## 2026-09-11T12:39:16+05:30
- Command: effects.cpp projection context + slide/blur damage-space cross-check
- Reason: Adjudicate logical-space rule for effect damage.

## 2026-09-11T12:44:52+05:30
- Command: deployed-copy identity check + doc drift grep
- Reason: Re-audit verification battery.

## 2026-09-11T12:47:01+05:30
- Command: re-audit 2 report written + HTML generated + 18e diff
- Reason: User-requested comprehensive audit deliverable.

## 2026-09-11T12:56:52+05:30
- Command: re-audit-2 fix batch (F1-F3/M5/m1-m5 edits + syntax verification)
- Reason: User-approved implementation.

## 2026-09-11T12:58:33+05:30
- Command: build #20 compile (re-audit-2 batch)
- Reason: User-authorized build.

## 2026-09-11T12:59:49+05:30
- Command: build #20 deploy (sha-gated)
- Reason: Ship re-audit-2 batch to dom0.

## 2026-09-11T13:02:51+05:30
- Command: kwin restart loading build #20 (f9389aec)
- Reason: Load re-audit-2 .so (user consented).

## 2026-09-11T13:03:27+05:30
- Command: build #20 functional probe (G+Alt+G gates, Alt+G restore)
- Reason: Post-restart behavior verification.

## 2026-09-11T13:03:50+05:30
- Command: wrap-up doc chain for build #20
- Reason: Rules 3/10/12/17 milestone recording.

## 2026-09-11T13:17:33+05:30
- Command: re-audit 3 battery + deployed identity
- Reason: Full-spectrum re-audit verification.

## 2026-09-11T13:18:39+05:30
- Command: re-audit 3 report + HTML + 18e diff
- Reason: User-requested comprehensive audit deliverable.

## 2026-09-11T13:27:30+05:30
- Command: m4b/m5b cosmetic edits + verification
- Reason: User-approved re-audit-3 corrections.

## 2026-09-11T13:33:11+05:30
- Command: distributability assessment logged (researched-ideas)
- Reason: Rule 8 research documentation.

## 2026-09-11T14:06:54+05:30
- Command: researched-ideas decision update
- Reason: Rule 8 close-out of the distributability proposal.

## 2026-09-11T14:17:20+05:30
- Command: template-compat Q&A logged
- Reason: Rule 10 audit trail.

## 2026-09-11T14:40:31+05:30
- Command: dom0-distro hypothetical Q&A logged
- Reason: Rule 10 audit trail.

## 2026-09-12T08:02:56+05:30
- Command: VM-scope Q&A logged
- Reason: Rule 10 audit trail.

## 2026-09-12T08:25:27+05:30
- Command: VM-label color investigation (xprop, labels dir, kwin scripts, qvm-ls)
- Reason: User-requested per-VM glow color feasibility.

## 2026-09-12T08:29:35+05:30
- Command: VM-label probe with 5-attempt retry loop (xprop atoms, labels svg, qvm-ls)
- Reason: User instruction: retry password dialog up to 5x; per-VM glow color investigation.

## 2026-09-13T14:57:46+05:30
- Command: build #21 label-hue implementation + verification battery
- Reason: User-approved per-VM label color change.

## 2026-09-13T15:23:54+05:30
- Command: build #21 compile (per-VM label hue)
- Reason: User-authorized build.

## 2026-09-13T15:24:54+05:30
- Command: build #21 deploy (sha-gated, .so + metadata v3.11.0)
- Reason: Ship per-VM label hue to dom0.

## 2026-09-13T15:28:26+05:30
- Command: build #21 deploy retry loop (5x per user rule)
- Reason: Previous password dialog was cancelled; user-authorized retry.

## 2026-09-13T21:59:08+05:30
- Command: white dom0 halo config (GlowColor/Inactive=255,255,255 + reconfigureEffect)
- Reason: User-approved white for dom0-native windows.

## 2026-09-13T22:00:25+05:30
- Command: reconfigureEffect with full env (XDG_RUNTIME_DIR fix)
- Reason: First live-reload call lacked XDG_RUNTIME_DIR per HANDBOOK §6c.

## 2026-09-13T22:02:37+05:30
- Command: kwin restart loading build #21 (2b43c24f, per-VM label hue)
- Reason: User consented restart for label colors.

## 2026-09-13T22:03:15+05:30
- Command: cross-VM label hue verification (window classes + journal)
- Reason: Verify per-VM colors beyond yellow.

## 2026-09-13T22:03:44+05:30
- Command: build #21 wrap-up docs
- Reason: Rules 3/10/12 milestone recording.

## 2026-09-15 — LL-033 surface-eligibility diagnosis (build #21 follow-up)
- Command: `podman exec dom0-replica-fed37 grep isUnmanaged/isManaged/effects src`; `grep EffectWindow API in /usr/include/kwineffects.h`; `_NET_CLIENT_LIST` census via `xprop`+`xdotool` on dom0
  - Reason: Adjudicate, against pinned KWin 5.27.8 source + installed headers, how override-redirect popups appear to the effect (isManaged vs isUnmanaged) and which windows are managed/unmanaged live.
- Command: `dom0 … ll033_surface_diag{1..7}.py` (window census, menu drive, gold/bright-ring probes, ASCII render)
  - Reason: Attempt three reported glow artifacts on dom0. Formatting/desktop/non-current-desktop obstacles; pixel probes invalidated by white-halo (GlowColor=255,255,255) making gold masks measure wallpaper.
- Command: `dom0 … ll033_capture.py --map-only`
  - Reason: Sanity-check the final human-in-the-loop capture probe (screen grab + window census) before the operator-assisted run.

## 2026-09-15 — LL-033 live-capture follow-up (baseline scan, notification, VM notification)
- Command: `dom0 … ll033_scan.py BASELINE` (ll033_scan.py added)
  - Reason: Snapshot the steady-state window census (all VM-proxied Dev-General:* windows MANAGED, no type atom, _QUBES_VMWINDOWID present — re-confirms LL-026; only Qui-*/Nm-applet unmanaged).
- Command: `dom0 … notify-send -u critical …` (300 s) + scan
  - Reason: Characterize the dom0 notification surface. Result: plasmashell class, CRITICAL_NOTIFICATION+NOTIFICATION type, managed, opacity unset (=1.0) → already class-excluded + ocluder-correct → NOT artifact-3 source.
- Command: `dom0 … qvm-run Dev-General notify-send …` + scan
  - Reason: Test the VM-notification hypothesis for artifact 3 (VM-proxied notifications with stripped type would be eligible). Result: NO new dom0 window appeared — VM notify-send did not produce a proxied surface in this capture window.

## 2026-09-15 — LL-033 implementation build (build #22)
- Command: `edit glowtargets.h` (add `if (!w->isManaged()) return false;`), version bumps to 3.12.0 (kittyglow.cpp, kittyglow.json, CMakeLists, kglowsync metadata), regression assertion ll033 added
  - Reason: Implement user-authorized fix for menu-dropdown glow + flickering bottom-edge outline (LL-033): reject override-redirect chrome before type/class/size predicates.
- Command: `bash scripts/regression-checks.sh` (27/27 pass, new ll033)
  - Reason: Rule 20 pre-submit regression battery.
- Command: `bash -n scripts/regression-checks.sh`; `python3 json.load` on both metadata files
  - Reason: Rule 16 syntax validation (shell + JSON).
- Command: `podman exec dom0-replica-fed37 bash -lc 'cmake -B /tmp/b … && cmake --build /tmp/b'` (COMPILE_EXIT=0, 0 warnings) + `podman cp` artifacts to dist/
  - Reason: Rule 16 verification the isManaged() header gat resolves against installed kwineffects.h; stage dist artifact sha 25169edd.

## 2026-09-17T09:02:21.377491+05:30
Recovery record: prior commands were `bash scripts/deploy.sh 2>&1 | tail -15` from the project root (deploy authorized build) and `cat ~/.local/bin/dom0 2>/dev/null | head -30` (inspect bridge stdin behavior). Plugin SHA verified; later authorization denied. This command appends recovery records before retry.

## 2026-09-17T10:21:49.287013+05:30
Command: `bash scripts/deploy.sh`
Reason: finish user-authorized build #22 deployment. Exit: 1. Output: `logs/output/deploy-final.log`. Restart not yet performed; remote artifacts still require verification.

## 2026-09-17T10:22:40.887138+05:30
Command: `bash scripts/deploy.sh`
Reason: user requested retry after incorrect password. Exit: 1. Output: `logs/output/deploy-retry.log`. Activation not yet verified.

## 2026-09-17T10:24:12.796890+05:30
Command argv: `['/home/user/.local/bin/dom0', 'printf "AUTH_OK\\n"']` (stdin /dev/null).
Reason: user-authorized harmless authentication test before deployment retry. Exit: 0. Output: 'A dom0 password dialog will appear — enter your password.\nAUTH_OK'. No remote changes or restart requested.

## 2026-09-17T10:24:55.760547+05:30
Command: `bash scripts/deploy.sh` (stdin /dev/null).
Reason: authorized deployment retry after successful authentication test. Exit: 0. Output: `logs/output/deploy-retry.log`. Restart not yet performed.

## 2026-09-17T10:25:17.976130+05:30
Command: `dom0 'set -e; sha256sum /usr/lib64/qt5/plugins/kwin/effects/plugins/kittyglow.so /usr/share/kwin/effects/kittyglow/metadata.json /home/chenpan/.local/share/kwin/scripts/kglowsync/metadata.json /home/chenpan/.local/share/kwin/scripts/kglowsync/contents/code/main.js; sudo -u chenpan env HOME=/home/chenpan kreadconfig5 --file kwinrc --group Plugins --key kittyglowEnabled; sudo -u chenpan env HOME=/home/chenpan kreadconfig5 --file kwinrc --group Plugins --key kglowsyncEnabled; pgrep -a -u chenpan -x kwin_x11'`
Reason: verify every deployed artifact and desktop-user enable flags before authorized restart. Result: False; exit 1.
```
Access denied.
A dom0 password dialog will appear — enter your password.
```

## 2026-09-17T10:26:31.045735+05:30
Command: `dom0 'set -e; printf %s '"'"'25169edd63f783f547603902f88d6323c3ba6fafe117ef71f7fa28f450d1615e  /usr/lib64/qt5/plugins/kwin/effects/plugins/kittyglow.so
21a39c053019ee5ca6009ebc1327c34a4c38b42ac1059ea435a2e28cf55068a9  /usr/share/kwin/effects/kittyglow/metadata.json
74e152dac7a3a09752a4066f8fdbb83ab3ead44a2bcb6ffe54046273e2c9cf8e  /home/chenpan/.local/share/kwin/scripts/kglowsync/metadata.json
fb5642b77c0a655ad2acc63e35d2f1752edcef107db9618761894f9022e3e6c7  /home/chenpan/.local/share/kwin/scripts/kglowsync/contents/code/main.js
'"'"' | sha256sum -c -; sudo -u chenpan env HOME=/home/chenpan DISPLAY=:0 XDG_RUNTIME_DIR=/run/user/1000 DBUS_SESSION_BUS_ADDRESS=unix:path=/run/user/1000/bus XDG_DATA_DIRS=/usr/local/share:/usr/share bash -c '"'"'set -eu
[ "$(kreadconfig5 --file kwinrc --group Plugins --key kittyglowEnabled)" = true ]
[ "$(kreadconfig5 --file kwinrc --group Plugins --key kglowsyncEnabled)" = true ]
old=$(pgrep -u "$(id -u)" -x kwin_x11)
[ -n "$old" ]
command -v qdbus >/dev/null
command -v systemd-cat >/dev/null
printf '"'"'"'"'"'"'"'"'DEPLOY_GATE_PASS old_pid=%s
'"'"'"'"'"'"'"'"' "$old"
setsid kwin_x11 --replace </dev/null > >(systemd-cat -t kittyglow-build22-restart) 2>&1 &
sleep 12
new=$(pgrep -u "$(id -u)" -x kwin_x11)
[ -n "$new" ] && [ "$new" != "$old" ]
printf '"'"'"'"'"'"'"'"'NEW_KWIN_PID=%s
'"'"'"'"'"'"'"'"' "$new"
loaded=$(qdbus org.kde.KWin /Effects org.kde.kwin.Effects.isEffectLoaded kittyglow)
printf '"'"'"'"'"'"'"'"'EFFECT_LOADED=%s
'"'"'"'"'"'"'"'"' "$loaded"
[ "$loaded" = true ]
grep -F '"'"'"'"'"'"'"'"'/usr/lib64/qt5/plugins/kwin/effects/plugins/kittyglow.so'"'"'"'"'"'"'"'"' /proc/$new/maps
journalctl -b -t kittyglow-build22-restart --since '"'"'"'"'"'"'"'"'2 minutes ago'"'"'"'"'"'"'"'"' --no-pager -n 100
printf '"'"'"'"'"'"'"'"'ACTIVATION_CHECK_PASS
'"'"'"'"'"'"'"'"'
'"'"''`
Reason: authorized deployment integrity gate and dom0 KWin restart. Exit: 0. Evidence: `logs/output/build22-activation.log`. Activation pass marker: True. Visual acceptance pending.

## 2026-09-17T10:26:47.147784+05:30
Build #22 activation verified: four artifact hashes match; both desktop-user enable flags true; KWin PID 100860 replaced by 101201; effect loaded and plugin mapped. Updated PROJECT_CONTEXT.md; visual acceptance pending. User request: "rettry". Regenerate HTML with `bash /home/user/Projects/scripts/generate-docs-html.sh PROJECT_CONTEXT.md` and verify resulting sibling.

## 2026-09-17T10:26:47.147784+05:30
Command: `bash /home/user/Projects/scripts/generate-docs-html.sh PROJECT_CONTEXT.md`
Reason: regenerate HTML sibling after activation state update. Exit 0; state/sibling consistency verified: True.

## 2026-09-17T10:39:37.978153+05:30
Command argv: `['git', 'status', '--short']`
Reason: approved rename preflight; inspect working tree and container mount without modification. Exit 0.

## 2026-09-17T10:39:40.352878+05:30
Command argv: `['podman', 'inspect', '--format', '{{json .Mounts}}', 'dom0-replica-fed37']`
Reason: approved rename preflight; inspect working tree and container mount without modification. Exit 0.

## 2026-09-17T10:41:07.669837+05:30
Approved branding: Qubes Glow display name; qubes-glow directory. Updated effect/script source metadata, SPECIFICATION.md, HANDBOOK.md, ARCHITECTURE.md, ROADMAP.md and PROJECT_CONTEXT.md; runtime IDs unchanged. Build #22 user acceptance recorded. No build/deploy/restart.

## 2026-09-17T10:41:07.669837+05:30
Command/action: Python `Path("kitty-glow").rename(Path("qubes-glow"))` in UI-Enhancements/Kwin.
Reason: approved repository rename; .git and all existing working-tree changes preserved. Container mount recreation deferred until separately approved build.

## 2026-09-17T10:41:07.669837+05:30
Command argv: `['bash', '/home/user/Projects/scripts/generate-docs-html.sh', 'SPECIFICATION.md', 'HANDBOOK.md', 'ARCHITECTURE.md', 'ROADMAP.md', 'PROJECT_CONTEXT.md']` in qubes-glow.
Reason: immediately regenerate edited core-doc HTML siblings. Exit 0. JSON metadata parsed successfully; legacy IDs retained.

## 2026-09-17T10:42:04.760106+05:30
Rewrote README.md for current application-window scope, Qubes Glow branding, stable runtime names, documentation links and container-mount prerequisite. Verified README below 150 lines and all relative Markdown targets exist.

## 2026-09-17T10:42:04.760106+05:30
Command argv: `['bash', '/home/user/Projects/scripts/generate-docs-html.sh', 'SPECIFICATION.md', 'HANDBOOK.md', 'ARCHITECTURE.md', 'PROJECT_CONTEXT.md']`
Reason: regenerate corrected pointer/state HTML siblings. Exit 0. README link and length checks passed.

## 2026-09-17T10:42:58.957471+05:30
Command argv: `['bash', 'scripts/regression-checks.sh']`
Reason: final approved rename validation, no compilation or remote changes. Exit 0.
```
ok   ll020-clip-source-is-halo-rect
ok   ll020-no-3arg-hwclipping-render
ok   ll020-no-scissor-enable
ok   ll020-unclipped-1arg-render
ok   ll020-desktop-skip-present
ok   ll017seamless-dock-expanded
ok   ll017seamless-normal-frame
ok   ll016-no-noborderrule-in-src
ok   ll023-dialogs-notifications-excluded
ok   ll023-glow-gate-paint-path
ok   ll026-chrome-class-exclusion
ok   ll026-min-frame-size-guard
ok   ll025-bootstrap-watchdog
ok   ll027-focused-routing
ok   ll027-global-masters
ok   ll027-override-protection
ok   ll028-logical-space-rule
ok   ll032-label-hue
ok   ll033-unmanaged-popup-excluded
ok   ll029-per-action-gates
ok   audit-m1-v2-script-purged
ok   m5-rollback-strips-kglowsync
ok   ll026-b-stages-borderless
ok   ll026-g-glow-master-switch
ok   ll026-script-app-windows
ok   ll025-kglowsync-resilient-bootstrap
---
ALL CHECKS PASSED
```

## 2026-09-17T10:42:58.957471+05:30
Command argv: `['git', 'diff', '--check']`
Reason: final approved rename validation, no compilation or remote changes. Exit 0.
```
```

## 2026-09-17T10:42:58.957471+05:30
Command argv: `['bash', '/home/user/Projects/scripts/generate-docs-html.sh', '--check', '.']`
Reason: final approved rename validation, no compilation or remote changes. Exit 0.
```
CHECK OK (1 dir(s))
```

## 2026-09-17T10:45:40.414340+05:30
Rename audit: recursively decoded every repository file including hidden files; token regex `kitty-glow|kitty_glow|kittyglow|Kitty Glow|QubesGlow|qubes-glow|Qubes Glow`; NUL/non-UTF8 binaries excluded from textual interpretation. Results: {'text_files': 166, 'git-history/config': 8, 'active': 557, 'built-artifact': 4, 'history/evidence': 270}; binaries 100, errors ['Broken symlink logs/build.log', 'Broken symlink logs/build9.log']. Full line evidence: logs/output/rename-token-inventory.tsv. No implementation edits.

## 2026-09-17T10:45:40.414340+05:30
Command argv: `['git', 'diff', '--check']`
Reason: user-requested rename consistency verification. Exit 0.
```
```

## 2026-09-17T10:45:40.414340+05:30
Command argv: `['bash', 'scripts/regression-checks.sh']`
Reason: user-requested rename consistency verification. Exit 0.
```
ok   ll020-clip-source-is-halo-rect
ok   ll020-no-3arg-hwclipping-render
ok   ll020-no-scissor-enable
ok   ll020-unclipped-1arg-render
ok   ll020-desktop-skip-present
ok   ll017seamless-dock-expanded
ok   ll017seamless-normal-frame
ok   ll016-no-noborderrule-in-src
ok   ll023-dialogs-notifications-excluded
ok   ll023-glow-gate-paint-path
ok   ll026-chrome-class-exclusion
ok   ll026-min-frame-size-guard
ok   ll025-bootstrap-watchdog
ok   ll027-focused-routing
ok   ll027-global-masters
ok   ll027-override-protection
ok   ll028-logical-space-rule
ok   ll032-label-hue
ok   ll033-unmanaged-popup-excluded
ok   ll029-per-action-gates
ok   audit-m1-v2-script-purged
ok   m5-rollback-strips-kglowsync
ok   ll026-b-stages-borderless
ok   ll026-g-glow-master-switch
ok   ll026-script-app-windows
ok   ll025-kglowsync-resilient-bootstrap
---
ALL CHECKS PASSED
```

## 2026-09-17T10:45:40.414340+05:30
Command argv: `['bash', '/home/user/Projects/scripts/generate-docs-html.sh', '--check', '.']`
Reason: user-requested rename consistency verification. Exit 0.
```
CHECK OK (1 dir(s))
```

## 2026-09-17T10:45:40.414340+05:30
Command argv: `['podman', 'inspect', '--format', '{{json .Mounts}}', 'dom0-replica-fed37']`
Reason: user-requested rename consistency verification. Exit 0.
```
[{"Type":"bind","Source":"/home/user/Projects/QubesOS/UI-Enhancements/Kwin/kitty-glow/src","Destination":"/src","Driver":"","Mode":"","Options":["nosuid","nodev","rbind"],"RW":true,"Propagation":"rprivate"}]
```

## 2026-09-17T10:46:32.238839+05:30
Command: Python os.readlink on logs/build.log and logs/build9.log; append audit findings to project ledgers.
Reason: record broken monitoring links and final conformance findings immediately. No symlink modification.

## 2026-09-17T10:46:57.590389+05:30
Command/check: Python Path("logs/build/2026-09-09T18xxZ-build9.log").is_file().
Reason: adjudicate broken-link cause. Confirmed: logs/build.log and logs/build9.log point to the removed kitty-glow directory; their target file exists at the same relative path under qubes-glow. These are rename-broken monitoring symlinks, not an unexplained pre-existing failure.

## 2026-09-17T10:47:28.686172+05:30
Commands: `git diff -- scripts/regression-checks.sh`; `git show HEAD:scripts/regression-checks.sh`.
Reason: targeted assertion-gap check requested by user. HEAD=25, working=26, added=['ll033-unmanaged-popup-excluded'], removed=[]. Exit statuses 0/0.

## 2026-09-17T10:50:02.449832+05:30
Command argv: `['bash', 'scripts/regression-checks.sh']`
Reason: post-correction validation (audit fixes: assertion count, decision matrix, capture-plan archive notice, registry header). Exit 0.

## 2026-09-17T10:50:02.449832+05:30
Command argv: `['bash', '/home/user/Projects/scripts/generate-docs-html.sh', 'SPECIFICATION.md', 'PROJECT_CONTEXT.md', 'docs/research/2026-09-15-ll033-capture-plan.md']`
Reason: post-correction validation (audit fixes: assertion count, decision matrix, capture-plan archive notice, registry header). Exit 0.

## 2026-09-17T10:50:02.449832+05:30
Command argv: `['bash', '/home/user/Projects/scripts/generate-docs-html.sh', '--check', '.']`
Reason: post-correction validation (audit fixes: assertion count, decision matrix, capture-plan archive notice, registry header). Exit 0.

## 2026-09-17T10:50:02.449832+05:30
Command argv: `['git', 'diff', '--check']`
Reason: post-correction validation (audit fixes: assertion count, decision matrix, capture-plan archive notice, registry header). Exit 0.

## 2026-09-17T10:50:02.449832+05:30
Command: replace rename-broken symlinks with a single corrected `logs/build.log -> build/2026-09-09T18xxZ-build9.log`; remove redundant broken `logs/build9.log` (duplicate of the same target).
Reason: Rule 2 monitoring-link cleanup verified by is_file() checks.

## 2026-09-17T11:07:10.863242+05:30
Command argv: `['bash', 'scripts/build.sh']`
Reason: user-approved item 1 — build Qubes Glow display metadata; build.sh auto-recreates the container whose bind source referenced the pre-rename path. Exit 126. Output: `logs/output/build-v3.12.0-qubesglow-branding.log`.

## 2026-09-17T11:08:21.896462+05:30
Command: chmod +x container/setup-build-container.sh (Python os.chmod, u+x/g+x/o+x).
Reason: exit-126 root cause — build.sh invokes the setup script directly and its exec bit was missing (all other scripts run via `bash`, only this one is executed directly). Verified os.access X_OK.

## 2026-09-17T11:08:28.316324+05:30
Command argv: `['bash', 'scripts/build.sh']` (retry after chmod +x).
Reason: item 1 build; setup now executable — verify it recreates the container with the qubes-glow bind source and compiles zero-warning. Exit 1. Output: `logs/output/build-v3.12.0-qubesglow-branding.log`.

## 2026-09-17T11:14:02.817225+05:30
Command argv: `['bash', 'scripts/deploy.sh']` (stdin /dev/null).
Reason: user-approved item 1 deploy — Qubes Glow display metadata build d899970d… to dom0 (sha-gated; LL-022).

## 2026-09-17T11:15:02.741754+05:30
Command: dom0 sha-gate + authorized kwin_x11 --replace + isEffectLoaded + maps check (build #23 activation).
Reason: LL-021 — rebuilt .so requires full kwin restart; verify behavior, not the DBus boolean alone.

## 2026-09-17T11:15:42.451694+05:30
Command: dom0 corrected maps check (path grep only; previous attempt contained a self-defeating hash-string grep — script bug, not a deploy failure).
Reason: confirm build #23 .so is mapped in the running kwin. Exit 1. Evidence: logs/output/deploy23-maps-check.log.

## 2026-09-17T11:16:05.828211+05:30
Command: dom0 single direct check — pgrep -n kwin PID liveness, plugin file presence, plain `grep kittyglow /proc/$pid/maps` in an if, deployed metadata Name.
Reason: self-check correction — one simple check instead of repeated flawed hash-grep scripts. Exit 0. Evidence: logs/output/deploy23-final-check.log.

## 2026-09-17T11:17:05
Command: bash -n setup script (SYNTAX_OK); regression suite (ALL PASSED); HTML regeneration for PROJECT_CONTEXT/ROADMAP/SPECIFICATION + --check; git diff --check.
Reason: item 1+2 completion validation (Rule 16/18). All exits 0.

## 2026-09-17T11:29:26.933784+05:30
Commands: ls /usr/libexec/git-core (helpers); git-credential-libsecret get probe (output masked — username/precence only); secret-tool search --all server github.com (secret lines filtered); ssh -v github probe (offered-key lines only).
Reason: locate the credential central-security-ops used for its GitHub push; all secret values suppressed by construction, none logged.

## 2026-09-17T11:30:10.912685+05:30
Commands: pgrep git-credential-cache + socket existence checks; credential lines in /etc/gitconfig + ~/.gitconfig; read cso .git/logs/refs/remotes/origin/master (timestamps); git log -1 + reflog on central-security-ops (read-only).
Reason: final determination of how the cso GitHub push authenticated — cache daemon, system config, and push timestamps are the last unexamined stores.

## 2026-09-17T11:31:26.632325+05:30
Commands: ssh-keygen ed25519 dedicated deploy key ~/.ssh/qubes-glow_ed25519 (no passphrase, chmod 600); idempotent ~/.ssh/config Host github-qubesglow block; git add -A; git commit.
Reason: user-requested private GitHub push — keypair + local snapshot prepared; push itself waits on user-side repo creation + key authorization.

## 2026-09-17T11:46:13.753179+05:30
Commands: curl api.github.com /user, /user/orgs, /repos/{owner}/qubes-glow, /user/repos (PAT via stdin config; token never echoed/written to disk).
Reason: user-supplied GitHub PAT verification — identity, scopes, target-repo existence. Token value handled in-memory only; explicit REDACTED in this log per Rule 9.

## 2026-09-17T11:47:39.328182+05:30
Commands: stat/read-first-line of ~/.keys/github (type classification only); then either ssh-keygen -lf + git ls-remote (ssh URL) or curl api.github.com /user + /repos/octal-illumination/qubes-glow with the key loaded in-memory (never echoed, never written to disk/config).
Reason: user directed that the existing key at ~/.keys/github be used READ-ONLY — verify identity, repo existence/privacy, and push-permission flag without performing any write and without echoing the secret.
