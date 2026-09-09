// kittyglow — v3.3: occlusion-clipped SDF glow, animation-mapped halo, repeat-gated shortcut.
// Note: This code is purely AI-generated.
//
// Render path: one hardware-blended triangle fan quad around the frame rect;
// the GLSL SDF shader (glowshader.cpp) computes per-side soft falloff, so
// thickness/corner radius/colors come from kwinrc [Effect-kittyglow] and are
// live-reloadable via org.kde.kwin.Effects.reconfigureEffect. The quad is
// mapped through the scene's animation transform (scale/translation on
// WindowPaintData), so the halo tracks minimize/restore mid-flight instead of
// snapping to the destination geometry, and fades with data.opacity().
//
// Shortcuts: Meta+Shift+B (here) flips the persistent kitty window rule
// [kitty-borderless] (kwinrulesrc, Force rule → applies to live windows);
// the native per-focused-window toggle "Window No Border" is rebound to
// Meta+Shift+T by v2-rollout-round.sh.
#include "glowconfig.h"
#include "glowshader.h"

#include <kwineffects.h>
#include <kwinglutils.h>
#include <epoxy/gl.h>
#include <KGlobalAccel>
#include <QElapsedTimer>
#include <KConfigGroup>
#include <KSharedConfig>
#include <QAction>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingCall>
#include <QKeySequence>
#include <QRectF>
#include <QRegion>
#include <chrono>
#include <memory>

namespace {

bool isKittyWindow(KWin::EffectWindow *w) {
    if (w->isDesktop() || w->isDock()) return false;
    return w->windowClass().toLower().contains(QStringLiteral("kitty"));
}

}  // namespace

class KittyGlowEffect : public KWin::Effect {
    Q_OBJECT
public:
    KittyGlowEffect();
    void reconfigure(ReconfigureFlags flags) override;
    void prePaintWindow(KWin::EffectWindow *w, KWin::WindowPrePaintData &data,
                        std::chrono::milliseconds presentTime) override;
    void paintWindow(KWin::EffectWindow *w, int mask, QRegion region,
                     KWin::WindowPaintData &data) override;

private:
    void toggleKittyBorderless();
    void repaintHalo(const QRectF &frame);
    void updateOccluders();
    QRegion occludedAboveKitty(const QRectF &halo, qreal scale) const;
    KittyGlow::GlowConfig m_cfg;
    std::unique_ptr<KWin::GLShader> m_shader;
    // Key autorepeat made Meta+Shift+B flip the rule dozens of times per hold
    // (flicker + final parity depended on hold duration); gate per press.
    QElapsedTimer m_toggleGate;
    // Windows stacked above kitty whose opaque frames must clip the halo.
    QVector<KWin::EffectWindow *> m_occluders;
    QElapsedTimer m_stackingStamp;
};

KittyGlowEffect::KittyGlowEffect() {
    QAction *a = new QAction(this);
    a->setObjectName(QStringLiteral("Toggle Kitty Borderless"));
    a->setText(QStringLiteral("Toggle Kitty Borderless"));
    KGlobalAccel::self()->setDefaultShortcut(
        a, QList<QKeySequence>() << (Qt::META | Qt::SHIFT | Qt::Key_B));
    KGlobalAccel::self()->setShortcut(
        a, QList<QKeySequence>() << (Qt::META | Qt::SHIFT | Qt::Key_B));
    connect(a, &QAction::triggered, this, &KittyGlowEffect::toggleKittyBorderless);

    // The halo lies OUTSIDE the frame rect, so damage must be widened or the
    // ring smears during moves and lingers after minimize. Repaint the halo
    // area of both the new and the old geometry, and on minimize/unminimize.
    connect(KWin::effects, &KWin::EffectsHandler::windowFrameGeometryChanged, this,
            [this](KWin::EffectWindow *w, const QRectF &old) {
        if (!isKittyWindow(w)) return;
        repaintHalo(w->frameGeometry());
        repaintHalo(old);
    });
    connect(KWin::effects, &KWin::EffectsHandler::windowMinimized, this,
            [this](KWin::EffectWindow *w) {
        if (isKittyWindow(w)) repaintHalo(w->frameGeometry());
    });
    connect(KWin::effects, &KWin::EffectsHandler::windowUnminimized, this,
            [this](KWin::EffectWindow *w) {
        if (isKittyWindow(w)) repaintHalo(w->frameGeometry());
    });

    m_toggleGate.start();
    reconfigure(ReconfigureAll);
}

void KittyGlowEffect::repaintHalo(const QRectF &frame) {
    const int e = m_cfg.maxExtent();
    KWin::effects->addRepaint(frame.toRect().adjusted(-e, -e, e, e));
}

void KittyGlowEffect::reconfigure(ReconfigureFlags flags) {
    Q_UNUSED(flags)
    m_cfg = KittyGlow::loadGlowConfig(KSharedConfig::openConfig(QStringLiteral("kwinrc")));
    if (KWin::effects->isOpenGLCompositing()) {
        KWin::effects->makeOpenGLContextCurrent();
        m_shader = KittyGlow::GlowShader::create();  // caller-owned; rebuild here
    }
    KWin::effects->addRepaintFull();
}

void KittyGlowEffect::prePaintWindow(KWin::EffectWindow *w, KWin::WindowPrePaintData &data,
                                     std::chrono::milliseconds presentTime) {
    KWin::effects->prePaintWindow(w, data, presentTime);
    if (!isKittyWindow(w)) return;
    const int e = m_cfg.maxExtent();
    const QRect g = w->frameGeometry().toRect();
    data.paint |= g.adjusted(-e, -e, e, e);
    data.setTranslucent();
}

void KittyGlowEffect::paintWindow(KWin::EffectWindow *w, int mask, QRegion region,
                                  KWin::WindowPaintData &data) {
    KWin::effects->paintWindow(w, mask, region, data);  // normal window first
    // Never touch GL when the compositor is not OpenGL-based (e.g. QPainter
    // fallback after a crash) — raw GL calls there take the whole WM down.
    if (!KWin::effects->isOpenGLCompositing()) return;
    // No isMinimized() guard: KWin sets the flag BEFORE the shrink animation
    // runs (and clears it AFTER restore), so guarding here skips exactly the
    // animation frames. While fully minimized the window is not in the paint
    // loop at all, so no halo leaks in the steady state; the alpha guard below
    // still fades the halo if an effect animates opacity.
    if (!isKittyWindow(w)) return;  // halo only for kitty (filter must stay!)
    if (!m_shader) return;
    const QRectF g = w->frameGeometry();
    const float s = static_cast<float>(KWin::effects->renderTargetScale());

    // Minimize/restore/slide animations draw the window through the scene's
    // animation transform (data.toMatrix(): scale about the frame top-left,
    // then a translation that toMatrix() converts to device px). Map the halo
    // through the same affine transform so it tracks the window mid-flight
    // instead of snapping to the destination geometry — the same technique
    // stock BlurEffect uses for transformed windows (blur.cpp shouldBlur/shape).
    const qreal sx = data.xScale(), sy = data.yScale();
    const QPointF anchor(g.x(), g.y());
    const QPointF tr(data.xTranslation() * s, data.yTranslation() * s);
    const auto map = [&](const QPointF &p) {
        return QPointF(anchor.x() + (p.x() - anchor.x()) * sx,
                       anchor.y() + (p.y() - anchor.y()) * sy) + tr;
    };
    const QPointF k0 = map(g.topLeft());
    const QPointF k1 = map(g.bottomRight());

    // Fade with the window (opacity animations, fade-out end states).
    const float alpha = static_cast<float>(data.opacity());
    if (alpha <= 0.01f) return;

    const QRectF halo(k0.x() - m_cfg.widthLeft * sx, k0.y() - m_cfg.widthTop * sy,
                      (k1.x() - k0.x()) + (m_cfg.widthLeft + m_cfg.widthRight) * sx,
                      (k1.y() - k0.y()) + (m_cfg.widthTop + m_cfg.widthBottom) * sy);
    if (region.intersected(halo.toRect()).isEmpty()) return;

    // The halo is drawn after kitty's own paint pass with blending and no
    // depth test, so it also lands on windows stacked ABOVE kitty whenever
    // those windows' frames overlap the halo ring (drag/resize paths paint
    // without culling). Cut the occluded pieces out of the clip region: the
    // SDF is fragment-position-based, so scissored partial draws of the same
    // quad are pixel-identical to an unclipped draw.
    QRegion clip = region.intersected(halo.toRect());
    const bool cacheFresh = m_stackingStamp.isValid() && !m_stackingStamp.hasExpired(120);
    if (!cacheFresh) updateOccluders();
    if (!m_occluders.isEmpty())
        clip -= occludedAboveKitty(halo, s);
    if (clip.isEmpty()) return;

    QColor color = KWin::effects->activeWindow() == w ? m_cfg.colorActive
                                                      : m_cfg.colorInactive;
    color.setAlphaF(color.alphaF() * alpha);

    KittyGlow::GlowGeometry geo;
    geo.frame = QRectF(k0.x() * s, k0.y() * s,
                       (k1.x() - k0.x()) * s, (k1.y() - k0.y()) * s);
    geo.extents = QVector4D(static_cast<float>(m_cfg.widthLeft * sx * s),
                            static_cast<float>(m_cfg.widthTop * sy * s),
                            static_cast<float>(m_cfg.widthRight * sx * s),
                            static_cast<float>(m_cfg.widthBottom * sy * s));
    geo.radius = static_cast<float>(m_cfg.cornerRadius * qMin(sx, sy) * s);

    KWin::ShaderManager::instance()->pushShader(m_shader.get());
    KittyGlow::GlowShader::bind(m_shader.get(), data.projectionMatrix(), color, geo);
    KWin::GLVertexBuffer *vb = KWin::GLVertexBuffer::streamingBuffer();
    vb->setUseColor(false);  // color comes from u_color; useColor would touch a stock uniform
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    const float x0 = static_cast<float>(halo.x() * s);
    const float y0 = static_cast<float>(halo.y() * s);
    const float x1 = static_cast<float>((halo.x() + halo.width()) * s);
    const float y1 = static_cast<float>((halo.y() + halo.height()) * s);
    const float v[12] = {x0, y0, x1, y0, x1, y1, x0, y0, x1, y1, x0, y1};
    vb->setData(6, 2, v, nullptr);
    vb->render(clip, GL_TRIANGLES, true);
    glDisable(GL_BLEND);
    KWin::ShaderManager::instance()->popShader();
}

// Rebuild the occluder list: windows logically stacked ABOVE kitty that paint
// fully opaque frames. stackingOrder() is the logical bottom→top order and is
// NOT reordered while a window is dragged (elevation is paint-time only), so
// it is exactly the relationship the halo clip needs. Cached briefly — the
// list is only consulted while halo pixels are repainting (moves/drags).
void KittyGlowEffect::updateOccluders() {
    m_occluders.clear();
    const auto stack = KWin::effects->stackingOrder();
    int kittyIdx = -1;
    for (int i = 0; i < stack.size(); ++i) {
        KWin::EffectWindow *w = stack.at(i);
        if (w && !w->isDeleted() && isKittyWindow(w)) kittyIdx = i;
    }
    if (kittyIdx < 0) return;
    for (int i = kittyIdx + 1; i < stack.size(); ++i) {
        KWin::EffectWindow *w = stack.at(i);
        if (!w || w->isDeleted() || w->isMinimized()) continue;
        if (w->opacity() < 0.99) continue;  // translucent windows let the halo show
        if (!w->isOnCurrentDesktop() || !w->isOnCurrentActivity()) continue;
        m_occluders.append(w);
    }
    m_stackingStamp.start();
}

QRegion KittyGlowEffect::occludedAboveKitty(const QRectF &halo, qreal scale) const {
    QRegion occl;
    for (auto *w : m_occluders) {
        const QRectF gf = w->frameGeometry();
        // Device-px rect (+1 px fatten so no halo seam shows at occluder
        // edges), same top-left-origin space as the paint region —
        // GLVertexBuffer::draw() flips scissor rects for GL itself.
        occl += QRect(static_cast<int>(gf.x() * scale), static_cast<int>(gf.y() * scale),
                      static_cast<int>(gf.width() * scale) + 1,
                      static_cast<int>(gf.height() * scale) + 1);
    }
    return occl.intersected(halo.toRect());
}

void KittyGlowEffect::toggleKittyBorderless() {
    // kglobalaccel re-emits triggered() for every autorepeat of a held key;
    // each pass rewrote kwinrulesrc and restarted kwin_x11 (white/black
    // flicker, final parity random). Gate to one toggle per physical press:
    // repeats arrive 25-33 ms apart and keep restarting the timer, so a held
    // key suppresses itself; a release + new press is always later than the
    // 220 ms window.
    if (m_toggleGate.isValid() && !m_toggleGate.hasExpired(220)) return;
    m_toggleGate.start();

    auto rules = KSharedConfig::openConfig(QStringLiteral("kwinrulesrc"));
    KConfigGroup g(rules, QStringLiteral("kitty-borderless"));
    if (!g.exists()) return;
    g.writeEntry("noborder", !g.readEntry("noborder", true));
    rules->sync();
    // Loopback reconfigure: Workspace::slotReconfigure reloads the RuleBook
    // and re-applies the Force rule to mapped kitty windows immediately.
    QDBusMessage msg = QDBusMessage::createMethodCall(
        QStringLiteral("org.kde.KWin"), QStringLiteral("/KWin"),
        QStringLiteral("org.kde.KWin"), QStringLiteral("reconfigure"));
    QDBusConnection::sessionBus().asyncCall(msg);
}

KWIN_EFFECT_FACTORY(KittyGlowEffect, "kittyglow.json")
#include "kittyglow.moc"
