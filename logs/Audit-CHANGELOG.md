# Audit-CHANGELOG

## 2026-09-06T23:40:28Z — dom0 deployment audit (carried from prior session)
- Effect plugin deployed into dom0 privileged paths:
  - `/usr/lib64/qt5/plugins/kwin/effects/kittyglow.so` (root:root, mode 644)
  - `/usr/share/kwin/effects/kittyglow/metadata.json` (root:root, mode 644)
- Transfer method: `qvm-run` (dom0 → Dev-General) base64 pipe, written via `sudo`.
  Rationale: `qvm-copy` Filecopy RPC *into* dom0 was refused; `qvm-run` exec is allowed.
- Permission hardening: files must be world-readable (644). A `600` mode silently
  prevents KWin (running as the user) from loading the plugin — caught and fixed
  (Lesson LL-004).
- Activation (`kwin_x11 --replace`) remains unexecuted — requires explicit approval.
- No new attack surface introduced: the effect only draws GL geometry; it does not
  handle input, network, or read cross-VM data.

## 2026-09-07T22:02:39+05:30 — Audit: dom0 kglobalaccel rewrite + system effect deployment
- Stopped plasma-kglobalaccel.service, edited kglobalshortcutsrc via kwriteconfig5, restarted unit (backup taken first)
- System-wide file written: /usr/lib64/qt5/plugins/kwin/effects/plugins/kittyglow.so (sha256-verified) + /usr/share/kwin/effects/kittyglow/metadata.json
- KWin X11 crashed once during hot-swap; recovered via auto-restart. No coredumps; no shader/GL errors post-restart

## 2026-09-07T23:05:00+05:30 — process + deployment audit
- Killed orphan kglobalaccel5 PID 167889 (boot-time daemon holding stale pre-rebind
  X11 key grabs that shadowed shortcut routing — root cause of dead Meta+Shift+T).
  Explicit user consent obtained (Rule 1c). SIGTERM; single-daemon state verified.
- Restarted plasma-kglobalaccel.service user unit; verified exactly one daemon
  (209480) owns the bus, both shortcuts registered on /component/kwin.
- Deployed kittyglow.so build #4 (sha256 8fca358a4b5a…) to dom0; deployed hash
  verified byte-identical to dist/. KWin restarted cleanly via --replace (209874).

## 2026-09-08T00:02:28+05:30 — v3.3 quality incident + containment
- Incident: build #5 (d445c704…) shipped with paintWindow()'s isKittyWindow()
  filter accidentally deleted during the minimize-guard edit → halo drawn
  around every painted window incl. the panel (user-reported artifacts of
  varying shapes/sizes).
- Root cause: guard line replaced by a comment-only edit; compound predicate
  dropped; post-edit grep verification (Rule 16) not run before deploy.
- Containment/fix: filter restored (build #6, 097b3e24…); sha-verified deploy;
  clean kwin --replace (PID 211257); LL-010 recorded in SPECIFICATION.md.
- Process integrity: two dom0 auth refusals during reload (user-cancelled,
  then access-denied) were honored — no retry until explicit "RETRY".

## 2026-09-09T16:08:54Z — audit: LL-018 fix (src/kittyglow.cpp)
- Change: occluder eligibility — \`!w->isDock() &&\` guard added before opacity test. Security-relevant? No (visual-clip logic only). Verified: region re-read, brace/paren balance 0, single isDock call site.

## 2026-09-09T16:44:22Z — audit: Build #9 deploy + kwin restart
- dom0 metadata dir perms anomaly noted (chenpan cannot traverse /usr/share/kwin/effects/kittyglow; fix queued: chmod 755 dir, 644 file — required for KWin metadata read).

## 2026-09-09T16:55:43Z — audit: dom0 system changes during Build #9 verification
- /usr/share/kwin/effects/kittyglow/ dir chmod 700→755 (file 644 unchanged) — root-created dir was not traversable by kwin user; normalized to standard. System change recorded per Rule 3 scope.

## 2026-09-09T18:55:33Z — Doc-zone audit + LL-019 fix review
- Rule 18e audit: workspace-wide literal command is structurally broken
  (recursive grep -rl vs non-recursive ls; hundreds of pre-existing pairs
  outside this project) — reported to user, rule text untouched (Rule 11
  gates rule edits). Corrected scoped check: kitty-glow pointer set ↔ HTML
  set equal after deleting orphan logs/SESSION_STATE.html (Rule 22f).
- generate-docs-html.sh --check: OK for this project directory.
- Rule 21 (isolation): all edits confined to
  QubesOS/UI-Enhancements/Kwin/kitty-glow/; no cross-project references
  introduced.

## 2026-09-10T20:23:41+05:30
- Audit: deploy pipeline failure mode (LL-022) — deploy.sh exits 0 on swallowed password-dialog failure leaving stale artifact on dom0; mitigated by mandatory post-deploy sha verification (documented in HANDBOOK §5); deploy.sh hardening proposed (pending approval).
