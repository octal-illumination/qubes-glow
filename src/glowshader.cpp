// glowshader.cpp — kittyglow GLSL sources + shader acquisition/binding.
// Note: This code is purely AI-generated.
#include "glowshader.h"

#include <kwinglplatform.h>
#include <kwinglutils.h>
#include <QByteArray>
#include <memory>

namespace KittyGlow {
namespace {

const char kVertexSource110[] = R"GLSL(
#version 110
uniform mat4 modelViewProjectionMatrix;
attribute vec2 position;
varying vec2 v_pos;
void main() {
    v_pos = position;
    gl_Position = modelViewProjectionMatrix * vec4(position, 0.0, 1.0);
}
)GLSL";

const char kVertexSource140[] = R"GLSL(
#version 140
uniform mat4 modelViewProjectionMatrix;
in vec2 position;
out vec2 v_pos;
void main() {
    v_pos = position;
    gl_Position = modelViewProjectionMatrix * vec4(position, 0.0, 1.0);
}
)GLSL";

// SDF glow: alpha falls off quadratically from the rounded-box frame edge
// outward. u_rect = frame (x, y, w, h), u_extents = per-side glow width
// (left, top, right, bottom), u_radius = corner rounding — all device px.
const char kFragmentSource110[] = R"GLSL(
#version 110
uniform vec4 u_color;
uniform vec4 u_rect;
uniform vec4 u_extents;
uniform float u_radius;
varying vec2 v_pos;

void main() {
    vec2 c = u_rect.xy + u_rect.zw * 0.5;
    vec2 p = v_pos - c;
    vec2 b = u_rect.zw * 0.5;
    vec2 q = abs(p) - b + vec2(u_radius);
    float d = min(max(q.x, q.y), 0.0) + length(max(q, 0.0)) - u_radius;
    if (d <= 0.0) { gl_FragColor = vec4(0.0); return; }
    vec2 cp = c + clamp(p, -b, b);
    vec2 n = p - cp;
    float nl = length(n);
    if (nl < 0.0001) { gl_FragColor = vec4(0.0); return; }
    n /= nl;
    float ext = max(n.x < 0.0 ? -n.x * u_extents.x : n.x * u_extents.z,
                    n.y < 0.0 ? -n.y * u_extents.y : n.y * u_extents.w);
    if (ext <= 0.0) { gl_FragColor = vec4(0.0); return; }
    float t = clamp(d / ext, 0.0, 1.0);
    float a = u_color.a * (1.0 - t) * (1.0 - t);
    if (a < 0.004) { gl_FragColor = vec4(0.0); return; }
    gl_FragColor = vec4(u_color.rgb, a);
}
)GLSL";

const char kFragmentSource140[] = R"GLSL(
#version 140
uniform vec4 u_color;
uniform vec4 u_rect;
uniform vec4 u_extents;
uniform float u_radius;
in vec2 v_pos;
out vec4 fragColor;

void main() {
    vec2 c = u_rect.xy + u_rect.zw * 0.5;
    vec2 p = v_pos - c;
    vec2 b = u_rect.zw * 0.5;
    vec2 q = abs(p) - b + vec2(u_radius);
    float d = min(max(q.x, q.y), 0.0) + length(max(q, 0.0)) - u_radius;
    if (d <= 0.0) { fragColor = vec4(0.0); return; }
    vec2 cp = c + clamp(p, -b, b);
    vec2 n = p - cp;
    float nl = length(n);
    if (nl < 0.0001) { fragColor = vec4(0.0); return; }
    n /= nl;
    float ext = max(n.x < 0.0 ? -n.x * u_extents.x : n.x * u_extents.z,
                    n.y < 0.0 ? -n.y * u_extents.y : n.y * u_extents.w);
    if (ext <= 0.0) { fragColor = vec4(0.0); return; }
    float t = clamp(d / ext, 0.0, 1.0);
    float a = u_color.a * (1.0 - t) * (1.0 - t);
    if (a < 0.004) { fragColor = vec4(0.0); return; }
    fragColor = vec4(u_color.rgb, a);
}
)GLSL";

}  // namespace

std::unique_ptr<KWin::GLShader> GlowShader::create() {
    auto *platform = KWin::GLPlatform::instance();
    if (!platform) return nullptr;
    const bool core = platform->glslVersion() >= KWin::kVersionNumber(1, 40);
    auto shader = KWin::ShaderManager::instance()->generateCustomShader(
        KWin::ShaderTrait::UniformColor,
        QByteArray(core ? kVertexSource140 : kVertexSource110),
        QByteArray(core ? kFragmentSource140 : kFragmentSource110));
    if (!shader || !shader->isValid()) {
        qWarning("kittyglow: glow shader compilation failed");
        return nullptr;
    }
    return shader;  // caller-owned (unique_ptr); rebuild on reconfigure()
}

void GlowShader::bind(KWin::GLShader *shader, const QMatrix4x4 &mvp,
                      const QColor &color, const GlowGeometry &geo) {
    shader->setUniform(KWin::GLShader::ModelViewProjectionMatrix, mvp);
    shader->setUniform("u_color", color);
    shader->setUniform("u_rect",
                       QVector4D(static_cast<float>(geo.frame.x()),
                                 static_cast<float>(geo.frame.y()),
                                 static_cast<float>(geo.frame.width()),
                                 static_cast<float>(geo.frame.height())));
    shader->setUniform("u_extents", geo.extents);
    shader->setUniform("u_radius", geo.radius);
}

}  // namespace KittyGlow
