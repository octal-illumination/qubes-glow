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
