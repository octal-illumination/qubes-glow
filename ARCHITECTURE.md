<!-- HTML sibling: regenerate ONLY via bash ~/Projects/scripts/generate-docs-html.sh QubesOS/UI-Enhancements/Kwin/kitty-glow/ARCHITECTURE.md — Rule 18c left-aligned CSS. Never hand-roll pandoc. -->
# kitty-glow — Architecture

> **Note:** All documentation and code in this project are purely AI-generated.

## 1. High-Level Architecture

```
 Dev-General (AppVM)                 dom0 (X11 + KWin compositor)
 ┌──────────────────┐   qvm-run      ┌──────────────────────────────┐
 │ src/kittyglow.*  │  base64 pipe   │ /usr/lib64/.../effects/      │
 │ build container  │ ─────────────▶ │   kittyglow.so  (loaded by   │
 │ (F37/KWin 5.27)  │  (sudo cp)     │   KWin via kwinrc Plugins)   │
 └──────────────────┘                │        │ paintWindow hook     │
                                     │        ▼                      │
                                     │   SDF halo quad ─▶ screen    │
                                     └──────────────────────────────┘
```

## 2. Component Details

### 2.1 `KittyGlowEffect` (src/kittyglow.cpp)
`KWin::Effect` subclass. Constructor wires the shortcut (Meta+Shift+B →
`toggleKittyBorderless()`, autorepeat-gated 220 ms) and the repaint hooks:
- `windowFrameGeometryChanged` / `windowMinimized` / `windowUnminimized` →
  repaint the halo ring (old + new geometry) so the halo never smears;
- `stackingOrderChanged` → `repaintAllKittyHalos()` — a window raised above
  kitty re-clips the halo on the very next frame (LL-019);
- `windowActivated` → `repaintAllKittyHalos()` — halo recolors
  ACTIVE/inactive on focus changes (LL-019);
- `KittyToggle::init()` — DBus apply channel for the borderless state
  (`kittyglowrc` via `kittyglowstate.cpp`; kwinrulesrc retired, LL-016).

### 2.2 Render path (v3.4, one SDF quad)
`prePaintWindow()` widens kitty's repaint to the halo ring and marks the
window translucent. `paintWindow()` paints kitty normally, then draws ONE
hardware-blended triangle-fan quad around the frame rect with the GLSL SDF
shader (`glowshader.cpp`): per-side soft falloff from `u_extents`, corner
radius, active/inactive color. The quad is mapped through the scene's
animation transform (scale about frame top-left + translation) so the halo
tracks minimize/restore mid-flight, and fades with `data.opacity()`.

### 2.3 Occlusion clip + CPU subdivision (`occludedAboveKitty`, per paint)
```
clip = haloRect                                  // scene region NOT a clip source
clip -= union of occluder rect per opaque wi logically ABOVE
        the painted kitty: DOCKS use expandedGeometry() (frame + shadow
        margin; LL-018 always-clip); NORMAL windows use frameGeometry()
        ONLY — the halo paints beneath them, so their translucent shadow
        gradient dims it progressively right up to the border (seamless
        pass-behind, LL-017 superseded; no wallpaper gap)
        the painted kitty in stackingOrder() (same desktop/activity, not
        minimized; desktop windows skipped; docks/panels always count —
        LL-018; other translucent windows are skipped → bloom-through)
for each clip rect: upload 2-triangle sub-quad of the halo quad and draw
                    it unclipped (1-arg GLVertexBuffer::render(GL_TRIANGLES))
```
The scene `region` passed to paintWindow covers only the window's frame
area in KWin 5.27.8 (the prePaintWindow ring widening does not propagate),
and the rasterizer does not enforce the region anyway — that unenforcement
WAS the LL-020 leak. The SDF is fragment-position-based, so sub-rects are
pixel-identical to the full quad (LL-020 for the full three-part lesson).
Rebuilt on EVERY halo paint — no cache: the old 120 ms stacking snapshot
lagged raise/drag transitions and one unclipped frame then persisted forever
(an unfocused kitty never repaints; LL-019). Anchored to the PAINTED kitty
window, not "the topmost kitty", so multi-kitty stacks clip correctly.

### 2.4 `GlowConfig` (glowconfig.h)
Reads kwinrc `[Effect-kittyglow]` (thickness per side, corner radius,
active/inactive colors) with live reload via `reconfigureEffect` — no
compositor restart for tuning.

## 3. Data Flow

```
[stacking/focus/geometry change]
        │ stackingOrderChanged / windowActivated / windowFrameGeometryChanged
        ▼
addRepaint(halo ring) ──▶ KWin repaints the damaged region
        │
[kitty's paintWindow]
        ├─ effects->paintWindow(w, …)           // real window first
        └─ clip = halo − occluders              // occluders built THIS frame
                 └─ per clip rect: unclipped sub-quad draw (1-arg render)
```
The halo draws after the window's own paint (blending, no depth test); the
occluder subtraction keeps it off front applications and the desktop.

## 4. Key Abstractions

- `KWin::Effect` / `EffectsHandler` — paint hooks + repaint requests.
- `KWin::GLShader` + `ShaderManager::pushShader/popShader` — the SDF program
  (effect-owned, rebuilt in `reconfigure`).
- `KWin::GLVertexBuffer::streamingBuffer()` — per-frame quad upload;
  `render(GLenum)` = single unclipped glDrawArrays. The 3-arg hw-clipping
  overload is BANNED in this effect (LL-020: caller-must-enable scissor +
  degenerate KWin boxes).
- `stackingOrder()` — logical bottom→top order; NOT reordered during a drag
  (elevation is paint-time only) — see the note in `occludedAboveKitty`.

## 5. Error Handling Strategy

- **Non-OpenGL compositing:** `paintWindow` returns before any GL call
  (QPainter-fallback safety).
- **Window not in stacking order:** `occludedAboveKitty` fails OPEN (draws
  unclipped) rather than clipping the halo against the whole stack.
- **Empty clip / alpha ≤ 0.01:** early return before shader/vertex setup.
- **Missing/invalid plugin:** KWin logs the load failure and continues; it
  does not abort the compositor.

## 6. Problem Context

The compositor (not the app) owns window framing, so the glow must be a KWin
plugin in dom0. Full problem statement and lessons registry (LL-01x):
SPECIFICATION.md. Build/deploy/activate steps: HANDBOOK.md.
