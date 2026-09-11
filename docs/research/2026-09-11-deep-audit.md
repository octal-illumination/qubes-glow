# Deep Audit — kitty-glow build #18 (sha 7df23937, kwin 62946)

> **Note:** All documentation and code in this project are purely AI-generated.

2026-09-11 · Scope: every source file (C++, GLSL, JS, JSON, CMake, bash),
deployed artifacts, docs-vs-code drift, and the build/verify process chain.
Method: full read of all sources, shellcheck/node --check, sha audits,
grep sweeps for dangerous constructs, line-by-line math review of the SDF
render path, race/leak/security analysis. No code was modified.

## Verdict

**No critical or high findings.** 4 medium, 11 low findings; 1 confirmed
misedited comment; 3 stale-document drifts. Everything else verified clean.
Proposed fixes are listed per finding and await consent (none applied).

## Verified clean

- **Math (SDF render path)**: `glowshader.cpp` implements the standard
  rounded-box SDF (`q = abs(p) − b + r; d = min(max(q.x,q.y),0) +
  length(max(q,0)) − r`) — correct. Quadratic falloff `(1−t)²` matches the
  documented behavior; α cutoff 0.004 ≈ 1/255 avoids vanishing-alpha
  fragments; opacities `qBound(0..100)`; corner radius clamped 0–64 and
  scaled by `min(sx,sy)` under non-uniform animation scale (consistent
  approximation); inside-frame early-out (`d ≤ 0`) is exact.
- **Buffer safety**: no raw C buffers anywhere; the only fixed array is the
  12-float vertex scratch (`6 verts × 2 floats`, exact size used, `vb->setData(6, 2, …)`).
  No `strcpy/sprintf/memcpy/alloca/malloc` (a `gets` grep hit was the
  substring inside "tar**gets**" — tool artifact, not code).
- **Memory**: RAII throughout — shader in `unique_ptr` rebuilt on
  reconfigure; all QObjects parented (`SyncService` → app, QActions →
  effect); GLVertexBuffer streaming buffer reused; blend/shader state
  pushed and popped symmetrically. `GlowFocus` pointer set pruned on
  `windowDeleted` with `this` receiver context (kittyglow.cpp:182);
  JS overrides map is id-keyed (no dangling references possible).
- **Security**: no network I/O, no secrets, no dangerous syscalls; DBus
  surface (`org.kde.kittyglow`) lives on the session bus = same-user trust
  domain (accepted); deploy is sha-gated (LL-022) with 644 perms; build.sh
  and deploy.sh shellcheck-clean; no shell interpolation of untrusted
  input; chrome-class/size-guard exclusions are cosmetic (no privilege
  surface) — a VM app naming itself `qui-*` only *loses* glow.
- **Races**: staging slots (`s_pending`, `s_pendingOp`) are same-thread
  (effect GUI thread = DBus dispatch, documented contract); 220 ms autorepeat
  gate > 60 ms poll prevents command interleaving; bootstrap is idempotent
  (`booted` first-reply-wins) with a 5 s watchdog for lost replies; sweep
  pauses on stale heartbeat; `clientAdded` class-lag covered by the 400 ms
  sweep (commented, verified).
- **Consistency**: deployed `main.js` byte-identical to repo (sha
  `65ae4aa1…` both sides); dist sha matches PROJECT_CONTEXT; 20/20
  regression assertions; `node --check` clean; JSON valid; kglowsync
  metadata (`Id: kglowsync`, `X-Plasma-API: javascript`) matches loader
  expectations; `X-KDE-Library`/`X-KDE-PluginKeyword` = `kittyglow` match
  `KWIN_EFFECT_FACTORY`.
- **Doc claims traced to code**: 60 ms poll, 400 ms sweep, 5 s watchdog,
  220 ms gate, 48 px guard, class lists — identical in C++, JS, and docs.

## Medium findings

- **M1. Stale deploy script resurrects retired mechanism**
  (`scripts/v2-rollout-round.sh`): v2-era script hardcodes an ANCIENT build
  sha (`e2e9cef0…`) and the RETIRED kwinrulesrc "kitty borderless rule"
  flow. Executing it today would downgrade dom0 to an old .so and
  re-trigger the LL write-revert fight. shellcheck also flags SC2086 there.
  → **Propose: delete it** (git history preserves it) or move to
  docs/research/ with a prominent dead-script banner.
- **M2. Latent HiDPI scale bug in damage widening**
  (`src/kittyglow.cpp:195,215`): `prePaintWindow`/`repaintHalo` widen by
  `maxExtent()` in LOGICAL px, but paint maps the halo by
  `renderTargetScale` and animation scale. Harmless while `s == 1`
  (this system, documented at kittyglow.cpp:330), but on a HiDPI target
  the damage under-covers the halo → ring smearing. → **Propose: scale
  `e` by `renderTargetScale()` at both sites** (2-line change, needs a
  rebuild).
- **M3. Shared autorepeat gate drops cross-toggle presses**
  (`m_toggleGate`, kittyglow.cpp:87; four call sites): pressing B then G
  within 220 ms silently drops the second toggle (any toggle gates ALL).
  Heuristic error with user-visible effect. → **Propose: per-action
  QElapsedTimer members** (or gate keyed on `sender()`).
- **M4. ROADMAP.md stale since build #7** (violates Rule 22e): still
  describes "Phase 3 gated", kwinrulesrc-era state. Misleads session
  continuity. → **Propose: refresh to build-#18 reality** (or fold into
  PROJECT_CONTEXT and delete).

## Low findings

- **L1. Effect metadata Description drift** (`src/kittyglow.json:5`):
  still "around kitty terminal windows" — all-windows since build #15.
- **L2. kglowsync metadata Version static at 1.0**
  (`src/kwin-script/kglowsync/metadata.json`) across builds #15–#18
  (script changed substantially; no changelog anchor).
- **L3. State-module comment drift**: `kittyglowstate.h:26-28` still says
  "Meta+Shift+B flips this [glow]"; `kittyglowstate.cpp:8-11` layout
  comment omits `glowEnabled`.
- **L4. Dead config path + weak validation in `readColor`**
  (`glowconfig.h:44-64`): a 4-part color's 4th alpha component is parsed
  then always overwritten by `GlowOpacity`; `toInt` accepts out-of-range
  ints and `QColor(int,int,int)` with values outside 0–255 is UB-ish →
  `qBound(0, v, 255)`.
- **L5. Unbounded glow widths** (`glowconfig.h:37-40`): `GlowLeft=100000`
  would be honored (giant damage region); cornerRadius is bounded, widths
  are not. → propose `qBound(0.0f, w, 200.0f)`.
- **L6. Focused-B 60 ms focus race** (`kittytoggle.h`): the script flips
  `workspace.activeClient` at poll time; an alt-tab inside the ~60 ms
  window flips the NEW window. Accepted design (avoids crossing window
  ids over DBus) but undocumented as an edge → document in HANDBOOK §11.
- **L7. Single-slot staging last-wins parity edge**: if the 60 ms poll
  stalls past two gated presses (needs a >220 ms stall), the second
  command overwrites the first → one flip instead of two. Improbable
  (local bus), document only.
- **L8. Log injection via `scriptLog`** (`kittytoggle.cpp:80`): multi-line
  JS strings reach journald unfiltered (`noquote`). Cosmetic; sanitize
  newlines for hygiene.
- **L9. Orphaned duplicate comment** (`src/kittyglow.cpp:142-144`):
  build-#18 edit left a second copy of the pruning note at the actions
  site, separated from the actual `windowDeleted` connect (line 181-183).
  Confirmed misedited comment — remove the orphan.
- **L10. `test_create.cpp` lacks the AI-generated disclaimer header**
  (Rule 19 consistency; container-only probe, never deployed).
- **L11. Build container Fedora 37 is EOL** (build-time supply-chain
  hygiene only; nothing runtime inherits from it). Rebase to a supported
  Fedora/KWin when convenient — needs container setup rework.

## Operational notes (accepted, documented)

- Probe scripts write predictable `/tmp/*.py` paths via the privileged
  dom0 bridge; `mktemp` would be textbook. Local-dom0 access is already
  the trust boundary, so accepted — noted for future probes.
- Idle DBus chatter: 2 calls / 60 ms between two in-process endpoints —
  negligible; mergeable into one composite call if ever profiled hot.
- Per-paint occluder region rebuild is O(n) per halo paint — deliberate
  LL-019 trade-off, user-accepted.

## AI self-audit (hallucination / drift / misses / misedits)

- **Functional hallucinations: none found.** Every documented behavior
  (timers, gates, guards, exclusions, semantics) traced to implementing
  code; journal evidence matches claims (focused-op lines, scope-named
  toggles, sweep sizes).
- **Drift found and listed**: L1, L2, L3, M4 (docs/metadata lagging code).
- **Misedits found**: L9 (orphaned comment). Nothing else — assertions,
  syntax checks, sha audits and cross-file greps all agree.
- **Misses acknowledged**: the synthetic-key discovery (LL-027) revealed
  the probe suite can report false negatives for shortcut delivery;
  physical-key verification is now part of the documented procedure.
