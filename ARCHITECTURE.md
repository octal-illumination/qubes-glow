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
 └──────────────────┘                │        │ paintWindow hook      │
                                     │        ▼                      │
                                     │   GL halo quads ─▶ screen    │
                                     └──────────────────────────────┘
```

## 2. Component Details

### 2.1 `KittyGlowEffect` (src/kittyglow.cpp)
Subclasses `KWin::Effect`. On construction it connects `windowAdded` /
`windowDeleted`, seeds the current stacking order, and inserts any window whose
`windowClass()` contains "kitty" (case-insensitive) into `m_windows`
(`QSet<EffectWindow*>`). Overrides `paintWindow()`.

### 2.2 `drawGlow(EffectWindow*)`
Skips fullscreen windows. Reads `frameGeometry()`, then draws `layers = 8`
concentric translucent rects, each inset by `t * margin` (margin = 22 px) and
with alpha `70 * t` (brighter toward the window edge) in `RGB(255,221,0)`.

### 2.3 `paintQuad(QRect, QColor)`
The actual GL work:
```cpp
ShaderManager *sm = ShaderManager::instance();
GLShader *sh = sm->pushShader(KWin::ShaderTrait::UniformColor);
GLVertexBuffer *vb = GLVertexBuffer::streamingBuffer();
vb->setColor(c);
vb->setData(4, 2, verts, nullptr);   // 4 verts, 2D, no texcoords
vb->render(GL_TRIANGLE_STRIP);
sm->popShader();
```

## 3. Data Flow

```
[window opens] windowAdded ─▶ m_windows.insert(w)
        │
[each frame] KWin calls paintWindow(w, …)
        │
        ├─ if w ∉ m_windows OR not OpenGLCompositing → just paintWindow()
        └─ else: drawGlow(w)  (8 alpha-ramped rects, BEHIND window)
                 └─ effects->paintWindow(w, …)   // real window on top
```
Because the halo is drawn before the window's own paint, painter's-order keeps
it behind the terminal content.

## 4. Key Abstractions

- `KWin::Effect` — base class; `paintWindow` is the per-frame hook.
- `KWin::ShaderManager::pushShader(ShaderTrait::UniformColor)` — solid-color GL program.
- `KWin::GLVertexBuffer::streamingBuffer()` — reusable dynamic vertex buffer.
- `KWin::EffectsHandler::compositingType()` — guards GL-only drawing.

## 5. Error Handling Strategy

- **Non-OpenGL compositing:** effect is a no-op (guard in `paintWindow`).
- **Fullscreen kitty:** skipped to avoid tinting the whole screen.
- **GL errors:** `UniformColor` shader + `GLVertexBuffer` are KWin-internal and
  error-tolerant; a bad quad at worst renders nothing, not a crash.
- **Missing/invalid plugin:** KWin logs a load failure and continues; it does not
  abort the compositor (verified intent — see HANDBOOK verification steps).

## 6. Problem Context

The compositor (not the app) owns window framing, so the glow must be a KWin
plugin in dom0. Full problem statement and conformance rules: SPECIFICATION.md.
Build/deploy/activate steps: HANDBOOK.md.
