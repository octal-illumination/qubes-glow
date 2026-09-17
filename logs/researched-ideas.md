# Researched Ideas

## [Complete] KWin 5.27 effect metadata service-type key
- **What was asked:** Will KWin recognise the effect if its embedded metadata uses
  the modern top-level `ServiceTypes` key instead of legacy `X-KDE-ServiceTypes`?
- **Why the search was done:** The build-time JSON only had
  `"ServiceTypes": ["KWin/Effect"]`; it was unclear whether KWin's loader would
  match it (the canonical built-in effects use `X-KDE-ServiceTypes`).
- **Summary of Findings:** Inspected KF5 `kpluginmetadata.h` (Fedora 37, 5.108).
  Line 57 documents `ServiceTypes → serviceTypes()`. KWin's `EffectLoader` filters
  plugins via `serviceTypes().contains("KWin/Effect")`, so the modern key IS
  recognised. No rebuild required. (An external `metadata.json` with both keys was
  also deployed for belt-and-suspenders.)
- **Final Decision & Rationale:** Keep the embedded `ServiceTypes` key; the effect
  is discoverable. Rejected adding `X-KDE-ServiceTypes` to the embedded JSON as
  unnecessary churn.

## v3 Glow — Web Research Round (2026-09-07T13:11:13Z) [Complete]
**1. What was asked / The Problem:** Can the kitty glow be made configurable
(thickness per side, corner radius, colors) the official KDE way, and can the
move-jitter and minimize-lag artifacts be fixed? Also: split border/titlebar
toggle keys.
**2. Why the search was done:** User requested multiple web-search rounds and
"official KDE plasma way" configuration before proposing v3.
**3. Summary of Findings:**
- Official config: kwinrc group `[Effect-kittyglow]`, read via
  `effects->config()` (kwineffects.h:1390) in reconfigure() override —
  same pattern as KWin's own effects.
- Shader: `ShaderManager::generateCustomShader(traits, vert, frag)` exists in
  5.27 (kwinglutils.h:261); GLSL SDF rounded-box (iq, sdRoundBox with vec4
  per-corner radii) gives exact anti-aliased glow with configurable radius,
  per-side extents, and color/alpha falloff — replaces the 16-rect ring hack.
- Shortcuts: `effects->registerGlobalShortcut(...)` is the official effect
  shortcut API (kwineffects.h:898 block).
- Border/titlebar split: KWin X11 treats _MOTIF_WM_HINTS as all-or-nothing
  (KDE bug tracker); NO native title-vs-border split. Possible via window
  rules: BorderSize=None keeps titlebar (borders vanish); noborder removes
  everything. "Keep borders, remove only titlebar" is NOT possible natively.
- Move jitter: mature effects repaint old AND new geometry each frame during
  windowUserMovedResized/geometryShapeChanged; our v2 only expanded the new
  frame rect and stale ring pixels at the old position were left unrepainted.
- Minimize lag: glow must be suppressed when w->isMinimized() plus explicit
  addRepaint of the glow region on windowMinimized/windowUnminimized signals
  (both exist, kwineffects.h:1676-1682); v2 painted glow during the unmap
  animation and left stale pixels until the next full repaint.
**4. Final Decision & Rationale:** Propose v3 C++ effect (SDF shader +
config + repaint fixes + optional glow toggle shortcut) and native
window-rule-based border/title toggles. Shader math is deterministic and
testable; config keys follow the same kwinrc convention KWin ships with.

## 2026-09-08T00:02:28+05:30 — Scissor-based occlusion clipping for window-glow effects
Status: [Complete]
- What was asked: can the halo be hidden behind windows stacked above kitty
  without pixel probes?
- Why: opacity-based fading is per-effect, not per-pixel; shapes leak.
- Findings: KWin 5.27 GLVertexBuffer::render() accepts a QRegion clip
  (scissor path, GL_TRIANGLES) — verified against the Fedora-37 replica
  headers. Logical stackingOrder() gives occluders; convert to device px via
  mapToRenderTarget().
- Decision & rationale: adopt clip-region rendering (region-granular,
  GPU-side). Rejected per-pixel readback (GL_READ_PIXELS per frame — too
  slow) and accepting overlap (artifact the user actually saw at panel edge).

## Ghost Square (top-left 0,0) — Qubes tray-widget source windows get halos [Not Completed]
- **What was asked / The Problem:** User reported "a small empty square with glow on the top right corner of the screen that doesn't seem to represent anything" (2026-09-11, build #15 session) and asked to figure out why it exists.
- **Why the search was done:** Build #15's all-windows glow was producing a halo with no visible window inside it; needed identification before implementing the chrome-exclusion fix so the predicate change is evidence-based, not guesswork.
- **Summary of Findings:** Pixel-level component analysis of the full screen (strict-gold mask) found a 452 px ring at bbox (0,0)–(40,44): the 22 px-margin halo of four stale `Qui-updates`/`Qui-domains`/`Qui-clipboard`/`Qui-disk-space` 16×16 windows pinned at (0,0) — VM-side source windows of Qubes tray widgets that qubes-gui keeps mapped while their visible XEMBED copies live in the panel. They paint no content and carry no `_NET_WM_WINDOW_TYPE` (qubes-gui does not replicate the property — verified on konsole/kitty/firefox/Qui-*), so KWin classifies them normal and the type-based exclusions in `isGlowWindow()` never match VM-proxied windows. Full write-up: `docs/research/2026-09-11-ghost-square-qubes-tray-ghosts.md`.
- **Final Decision & Rationale:** Fix = class-based chrome exclusion (`plasmashell`, `Qui-*`, `xembedsniproxy`, `krunner`) in glowtargets.h + main.js, because WM_CLASS is the only window property that reliably survives the Qubes GUI proxy; type predicates remain for dom0-native windows. Implementation proposed 2026-09-11, awaiting user "implement changes"; follow-up questions (top-right vs top-left discrepancy, Qube Manager halo, VM-internal dialogs, working supportInformation method) recorded as the Global-TODO Step 12 task.

## Distributability of kitty-glow beyond dom0 (2026-09-11T13:33:11+05:30)
Status: [Not Completed] -> DECIDED AGAINST (2026-09-11T14:06:54+05:30)
1. **What was asked / The Problem** — User: if uploaded to GitHub for the
   world, shouldn't it work on any system this KDE Plasma version works?
2. **Why the search was done** — Assess dom0/Qubes coupling vs portable
   core before proposing a distribution roadmap.
3. **Summary of Findings** — Effect core is 100% standard KDE API (no
   dom0 paths/qvm calls in src/); blockers are all in packaging/process:
   no CMake install rules, dom0 helper/qvm-run deploy, hardcoded
   /usr/lib64 + chenpan, container-only build, Qubes eligibility
   heuristics (qui-*, 48px guard) leaking into stock behavior, missing
   LICENSE, stale README, GPL/MIT inconsistency. Compat: Plasma 5.27 X11
   = target (5.2x likely, needs CI); Plasma 6 = API break, NOT supported;
   5.27 Wayland = API-compatible + F1-F3 fixes make space handling right,
   but untested -> experimental. dom0 flow must be preserved alongside
   (Rule 21).
4. **Final Decision & Rationale** — User decision (2026-09-11T14:06:54+05:30): STICK WITH THE
   QUBES IMPLEMENTATION — the project was developed for Qubes use and
   stays Qubes-first. Phase A rejected: no config-ification, the qui-
   exclusion and 48px ghost-square guard remain hardcoded, no auto-preset.
   Effect behavior stays byte-identical. Note for the future: publishing
   to GitHub as-is requires zero code changes (Qubes heuristics are inert
   no-ops on non-Qubes systems); a world-facing README + LICENSE are the
   only honest-framing items if that ever becomes wanted.

## LL-033 — How KWin 5.27.8 classifies override-redirect popup surfaces for effects
- **Status:** [Complete]
- **What was asked:** Which effect-visible predicate can exclude menu drop-downs /
  popup windows that currently receive the glow?
- **Why searched:** Three user-reported glow artifacts (menus, bottom-edge outline,
  notifications) after build #21; the type- and class-based exclusions appeared not
  to fire.
- **Summary of Findings:** EffectWindow exposes `isManaged()` ("whether it's managed
  or override-redirect"), NOT `isUnmanaged()`. `managed = window->isClient()` is
  captured at construction precisely so effects can detect unmanaged popups after
  the Deleted-reparent. Unmanaged windows reach effects (unmanagedAdded wiring).
  Qt QMenu popups are override-redirect, unmanaged, type-less, and inherit the
  parent WM_CLASS — invisible to every current exclusion. Live: only tray icons
  (Qui-*) are unmanaged; app/menu surfaces are on the current desktop question.
- **Final Decision & Rationale:** The fix gate should be `if (!w->isManaged())
  return false;` in glowtargets.h. Pinned-source-verified, cheap, and matches the
  framework's documented intent. Direct per-surface capture still pending (see
  capture plan) before implementing, per Rule 1a consent.

## 2026-09-17T10:42:04.760106+05:30 — Naming [Complete]
### What was asked / The Problem
Choose KDE/KWin-appropriate names after the effect expanded beyond kitty.
### Why the search was done
Distinguish display branding, repository names, plugin identity and packages.
### Summary of Findings
- https://develop.kde.org/docs/plasma/kwineffect/ demonstrates Name "Hello World" and Id "hello-world"; this is an example, not a universal naming mandate.
- https://api.kde.org/kpluginmetadata.html distinguishes user-visible Name and pluginId; current KF6 C++ IDs derive from library filenames. Do not apply KF6 metadata changes blindly to this KF5 installation.
- https://github.com/taj-ny/kwin-effects-forceblur uses display name Better Blur.
- https://github.com/ekaaty/kwin-effect-rounded-corners uses Rounded Corners; package naming varies.
### Final Decision & Rationale
User approved Qubes Glow display name and qubes-glow project directory.
Reject mandatory CamelCase branding and a global identifier replacement.
Preserve kittyglow/kglowsync, config keys, DBus and shortcut identities.
kwin-effect-qubes-glow is a possible future package name, not a shipped package.
