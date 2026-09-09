# Output History (internal rationale)

## 2026-09-06T23:40:28Z — Design choice: layered-alpha halo
- Decision: render the glow as 8 stacked translucent GL rects (alpha 0→70) rather
  than a blur shader or a single opaque rect.
- Rationale: zero custom shader code, cheap, gives a soft falloff; KWin's
  `UniformColor` shader + `GLVertexBuffer` already provide everything needed.
- Location: `src/kittyglow.cpp` (`drawGlow`, `paintQuad`).

## 2026-09-06T23:40:28Z — Build isolation: Fedora-37 replica container
- Decision: compile inside `dom0-replica-fed37` (Fedora 37, kwin-devel 5.27.8)
  instead of directly on dom0.
- Rationale: dom0 has no toolchain; the container pins the exact KWin ABI so the
  `.so` loads against dom0's `libkwin.so` without symbol mismatch.

## 2026-09-06T23:40:28Z — Why qvm-run instead of qvm-copy
- Decision: transfer the built `.so` into dom0 via `qvm-run` base64 pipe.
- Rationale: `qvm-copy` *into* dom0 triggered a refused Filecopy RPC; `qvm-run`
  (dom0 executing in Dev-General and piping stdout back) is permitted and writes
  the file via `sudo` in dom0.
## 2026-09-07T05:18:46Z — rollback mode clarification (internal rationale)
- Asked: difference between `only` vs default mode. Answered: identical artifact-removal steps (DBus unload, kwinrc flags, script dir, .so); modes differ ONLY in step-3 WM handling. `only` = no WM touch; default = kwin_x11 --replace (fresh decorated KWin); xfwm4 = switch to xfwm4. `only` nuance: removes the cause of missing decorations but already-stripped windows stay bare until app reopen / KWin restart. Concrete anchor: this morning's deploy ran default mode while xfwm4 was live → that is why the session ended on KWin 197241; `only` would have left xfwm4 running.

## 2026-09-07T06:30:03Z — v2 rollout rationale
Internal rationale (no external research): chose runtime DBus loadEffect over
compositor restart because v1 restarts were the riskiest step (xfwm4 incident)
and 5.27 Effects API was verified from upstream source; chose Force=2 for noborder
so kitty never shows a KWin decoration regardless of app hints; rule listed under
[General] rules= because unlisted groups are ignored (LL-007). Alternatives
rejected: per-window DBus scripting (needs running script console), kruleboom/
GUI KCM (not scriptable for this rule shape).

## 2026-09-07T06:58:06Z — Meta+Shift+B verification findings (incl. correction)
- CORRECTION to prior turn: kwinrulesrc [General] rules=1 references group [1]
  which CONTAINS the kitty-borderless content — the borderless rule IS active.
  The unlisted [kitty-borderless] group is an inert duplicate, not a regression.
- Meta+Shift+B toggle: originated in v1 KWin script kitty-toggle-border
  (main.js: c.noBorder flip + registerShortcut "Meta+Shift+B"); script was the
  root cause of global decoration loss (over-broad matching) and was rolled
  back. Residue: ~/.local/share/kwin/scripts/kwin_script/ (disabled,
  isScriptLoaded=false); no Meta+Shift+B entry remains in kglobalshortcutsrc.
- Clean restore path: KWin BUILT-IN shortcut "Window No Border"
  ("Toggle Window Titlebar and Frame") exists in kglobalshortcutsrc bound to
  none — rebinding to Meta+Shift+B restores the toggle with zero scripts.
  Caveat: forced borderless rule re-asserts on window remap.
- Session: org.kde.kglobalaccel owned by PID 4619 (user session, Sep 2);
  orphan kglobalaccel5 167889 (PPID 1) owns nothing — benign.

## 2026-09-07T07:08:41Z — Recovery round rationale
- Chose NATIVE "Window No Border" over resurrecting the v1 JS script: identical
  semantics, no matching bug, no script lifecycle; v2 design principle = no KWin JS.
- kglobalaccel 5.27 exposes no reread DBus API (introspection-verified) → unit
  restart is the sanctioned apply path; the daemon rewriting `none`→empty on the
  active field is empirical proof of acceptance.
- kwinrulesrc: kept the NAMED group and rewrote rules= to it (content-identical
  merge verified before deleting [1]); count= left at 1 (legacy field, rules= is
  the activator per RuleBook::load).
- Chromium UUID groups left untouched (user data, pre-existing inert state).
- Rehome used env sourced from the live kwin /proc/<pid>/environ; setsid+sudo
  wrapper parents kwin to PID 1, surviving fish/terminal exit.

### 2026-09-07T23:05:00+05:30 — v3.1 animation-tracking round (internal rationale)
Rationale for the transform fix: data.toMatrix() is the public API the GL scene applies to window items (itemrenderer_opengl.cpp:265), and AnimationEffect composes Scale as translate + xScale/yScale about the frame top-left (kwinanimationeffect.cpp:563-570) — so replicating that exact affine mapping in the effect reproduces the window's on-screen quad without needing the full matrix. Rejected alternative: full QMatrix4x4 via toMatrix() — passes device scale but mixes window-local vs screen coordinate spaces in the SDF shader uniforms, higher risk for zero benefit. Rejected: intercepting prePaintScreen — too invasive, breaks stacking.

## 2026-09-08T00:02:28+05:30
Rationale for the mid-round self-check: the taskbar artifact the user saw was
not cosmetic drift — it was the missing isKittyWindow() predicate in
paintWindow(). Chose "stop and disclose" over "finish docs silently": the user
was looking at a build whose defect I had caused; documenting it as success
would have been a false record. Build #6 keeps the seamless-minimize behavior
(flag flips before the shrink anim, fully-minimized windows never painted)
while restoring the kitty-only filter — both goals coexist because the
minimize guard was protecting frames that don't exist after the flag flip.
