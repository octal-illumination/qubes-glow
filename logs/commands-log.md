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
