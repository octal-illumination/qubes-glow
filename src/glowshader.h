// glowshader.h — kittyglow GLSL sources + SDF glow rendering helpers.
// Note: This code is purely AI-generated.
#pragma once

#include <QColor>
#include <QMatrix4x4>
#include <QRectF>
#include <QVector4D>
#include <memory>

namespace KWin {
class GLShader;
}

namespace KittyGlow {

// Geometry of one glow draw call, already in device pixels.
struct GlowGeometry {
    QRectF frame;        // window frame rect (device px)
    QVector4D extents;   // left, top, right, bottom glow width (device px)
    float radius;        // corner radius (device px)
};

class GlowShader {
public:
    // Builds the SDF glow program for the active GLSL flavor (1.10 vs 140
    // core). Returns null when compilation or linking failed. Caller owns
    // the shader; rebuild on reconfigure() like stock KWin effects do.
    static std::unique_ptr<KWin::GLShader> create();

    // Uploads per-window uniforms to the already-pushed shader. Caller owns
    // push/pop, blend state, vertex buffer fill and render().
    static void bind(KWin::GLShader *shader, const QMatrix4x4 &mvp,
                     const QColor &color, const GlowGeometry &geo);
};

}  // namespace KittyGlow
