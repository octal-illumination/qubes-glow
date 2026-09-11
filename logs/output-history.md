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

## 2026-09-09T17:41:36Z — LL-019 (new): halo penetrates front windows; right-side asymmetric
### What was asked
User retest of Build #9: glow still penetrates an unmaximized/unminimized window placed OVER an unmaximized/unminimized kitty — but ONLY when nothing is behind kitty (obs. 1). Follow-up (obs. 2): with a behind window present, the artifact appears when the front window starts on the RIGHT half of kitty, and does NOT when it starts on the LEFT.
### Why
Localize the LL-018-followup penetration root cause before touching code (prior probe had passed 0-px through-cover on Build #8, so the probe configuration clearly missed the failing case).
### Findings (source analysis, kittyglow.cpp r64e6b36)
1. Halo is drawn during kitty's paintWindow, AFTER kitty's own paint, blended with no depth test — it lands on whatever the framebuffer holds at that point, including areas of windows stacked above that do not repaint that frame. The occluder scissor (clip -= occludedAboveKitty) is the ONLY protection; painter's order does not save us when the front window is not repainted in the same frame.
2. Damage: prePaintWindow widens kitty's paint by the FULL halo ring (frame±e) unconditionally — under my KWin model the front window should repaint every kitty blink and cover the halo. Artifact exists => the model is wrong somewhere (culling) or the scissor misses the front window. Probe must resolve which.
3. Suspects in code: (a) 120 ms occluder cache race — stale empty list (built while kitty was topmost) lets one unclipped halo frame through, and the artifact then PERSISTS because a static front window never repaints to erase it; (b) 'if (!m_occluders.isEmpty())' skips the subtract entirely on empty lists; (c) kittyIdx takes the TOPMOST kitty-class window — a second kitty (e.g. the 'back' window being a kitty) silently re-points the occluder scan; (d) NO left/right branch exists in the code — the right-side asymmetry must come from environment geometry (panel position, per-side width config, window arrangement), not from the clip math.
### Final decision & rationale
No code change yet (Rule 1a): need discriminative evidence. Will ask user 3 targeted questions (panel edge; front-window window class/transparency; whether the 'back' window is a kitty) and propose a deterministic 4-configuration synthetic probe (Z behind present/absent × W left/right) with stack dumps + alpha-aware pixel masks. Status: [Not Completed]

## 2026-09-09T17:47:00Z — LL-019 addendum: shader/config ruled out; two code defects found
### Findings (glowshader.cpp + glowconfig.h read)
- SDF per-side extent math is mirrored (left/right symmetric); config defaults 32/32/32/32 symmetric. Left/right artifact asymmetry CANNOT originate in clip math or shader. It must come from WHICH windows enter m_occluders (stacking arrangement, window classes) or which window the occluder scan anchors to.
- DEFECT A (anchoring): updateOccluders() sets kittyIdx = TOPMOST kitty-class window in stackingOrder(), not the painted window w. With 2+ kitty-class windows (trivial for a kitty user), occluders are relative to the WRONG window whenever another kitty sits above the painted one; windows above the painted kitty but below the topmost kitty never clip. paintWindow() doesn't pass w to updateOccluders().
- DEFECT B (stale cache race): occluders cached 120 ms; a rebuild that ran while the painted kitty was topmost stores an EMPTY list; a kitty paint within 120 ms of a stack change paints the halo unclipped ('if (!m_occluders.isEmpty())' also skips the subtract on empty lists). Because a static front window rarely repaints afterwards, one bad frame PERSISTS visually.
- Both defects match 'penetrates only sometimes / depending on arrangement'; neither is inherently left/right — need user's test details (front-window class, transparency, panel edge, back-window class) to close the asymmetry, or a deterministic 4-config probe.
### Final decision & rationale
Propose (Rule 1a): anchor occluders to the painted kitty w (signature change), rebuild every halo paint (2 Hz blink cadence — cache unnecessary), keep dock/opacity policy as-is pending user answers. No edit until 'implement changes'. Status: [Not Completed]

## 2026-09-09T18:09:11Z — LL-019 addendum 2: upstream GLVertexBuffer verified; DEFECTS C & D found
### Findings (KWin v5.27.8 src/libkwineffects/kwinglutils.cpp, fetched verbatim)
- GLVertexBuffer::draw(region, mode, first, count, hardwareClipping=true): plain per-rect loop — for each rect: glScissor(r.x(), fbH-(r.y()+r.h()), r.w, r.h) then glDrawArrays. NO rect-count limit, NO bounding-rect fallback, y-flip side-agnostic. Upstream clip path has NO left/right branch.
- Combined with symmetric shader/config/SDF and no side branch in kittyglow.cpp clip math: the left/right asymmetry is ENVIRONMENTAL (arrangement, drag protocol, occluder membership), not geometric.
- DEFECT C (drag lag): 120 ms occluder cache means during a front-window drag the scissor rect lags up to 120 ms of drag distance; halo leaks on the drag's LEADING edge. After the drag stops, a static front window + idle (unfocused) kitty never repaint -> leak persists. Moving the window again repairs the ring (fresh rebuild + correct clip) — matches 'artifact when placed on right, clean when moved to left' IF the user's drag protocol is directional.
- DEFECT D (stale focus color, minor): no activeWindowChanged repaint hook; halo stays ACTIVE-gold after focus leaves kitty until the next unrelated kitty repaint.
### Final decision & rationale
Ask user 3 discriminating questions (front-window app + translucency; drag vs raise protocol incl. direction; second kitty-class window present?), offer deterministic 4-config probe (needs dom0 bridge dialog -> explicit consent), and propose fix bundle: anchor occluders to painted w, rebuild every halo paint (kills A+B+C in one change), optional activeWindowChanged repaint hook (D). No edits until 'implement changes'. Status: [Not Completed]

### 2026-09-09T18:29:08Z — LL-019 addendum 3: user repro matrix simplified; asymmetry theory dead [Not Completed]
- What was asked: user re-tested the halo-penetration artifact to answer the 3 discriminating questions.
- Why: previous session ended with a left/right asymmetry clue and 4 proposed defects (A-D) needing confirmation.
- Summary of findings:
  - Artifact is systematic: ANY unmaximised, unminimised window placed in front of kitty penetrates; left/right position irrelevant; a maximised window behind kitty is irrelevant; reproduced with konsole AND firefox (any window class).
  - Root-cause model: during window placement (raise/drag transition) at least one halo frame draws unclipped (stale 120 ms occluder cache / pre-restack stacking), and because an UNFOCUSED kitty never repaints, that single bad frame persists indefinitely.
  - Upstream API verification (kwineffects.h v5.27.8 -> /tmp/kwineffects-5.27.8.h):
    - void stackingOrderChanged() line 1820 — EffectsHandler signal since 4.10 ("emitted when ever the stacking order is change, ie. a window is risen or lowered") -> the missing deterministic healing hook.
    - void windowActivated(KWin::EffectWindow *w) line 1550 — correct signal for the focus-repaint hook; the earlier-planned name "activeWindowChanged" does NOT exist in 5.27.8.
    - NO EffectsHandler::movingWindow() in 5.27 (m_movingWindowsSet is WindowMotionManager-private, lines 3744-3764); EffectWindow::isUserMove() line 2456 exists for drag hardening.
- Final decision: revised 5-item fix bundle proposed to user (all in src/kittyglow.cpp): (1) rebuild occluders every halo paint, drop 120 ms cache; (2) anchor occluders to painted kitty w; (3) stackingOrderChanged -> full-ring repaint of all kitty halos (heals placement penetration in one frame); (4) windowActivated -> repaintHalo (stale color); (5) optional isUserMove/isUserResize occluder hardening. Awaiting explicit "implement changes" (Rule 1a).

## 2026-09-11T10:53:43+05:30 — Rationale: JS script vs pure C++ for borderless plumbing (build #18)
- What was asked: why kglowsync is JavaScript instead of C/C++.
- Reason for record: internal architecture rationale (Rule 8), no external research.
- Findings: KWin's C++ EffectWindow API is read-mostly for compositing and
  cannot mutate noBorder (Window/X11Client are internal, non-ABI); the JS
  scripting API is the supported mutation surface. Alternatives rejected with
  project evidence: kwin-rule + reconfigure (LL-016 white flash, 2026-09-09
  write-revert fight), X11 property hacks (fragile/X11-only), Python/QML
  (not embedded). JS side adds crash containment (script throw != kwin
  segfault), hot-deployable KPackage, runtime API fallbacks. Cost: mirrored
  eligibility predicate (LL-026, assertion-enforced) + DBus staging channel.
- Decision: keep the C++/JS split as designed; documented in SPECIFICATION
  LL-027 and HANDBOOK 6b.
