// kittyglow — v3.6: occlusion-clipped SDF glow (per-paint occluder rebuild,
// CPU-subdivided halo quads — LL-020), animation-mapped halo, repeat-gated
// shortcut.
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
// [kitty-borderless] (kwinrulesrc, Force rule) and stages the new value for
// the kglowsync KWin script (60 ms DBus poll, kittytoggle.cpp) which applies
// noBorder live — no reconfigure, no kwin restart, no flash (LL-016); the
// native per-focused-window toggle "Window No Border" is rebound to
// Meta+Shift+T by v2-rollout-round.sh.
#include "glowconfig.h"
#include "glowshader.h"

#include <kwineffects.h>
#include <kwinglutils.h>
#include <epoxy/gl.h>
#include <KGlobalAccel>
#include <QElapsedTimer>
#include <QDebug>
#include <KConfigGroup>
#include <KSharedConfig>
#include <QAction>
#include <QKeySequence>

#include "kittyglowstate.h"
#include "kittytoggle.h"
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
    void repaintAllKittyHalos();
    QRegion occludedAboveKitty(KWin::EffectWindow *kitty, const QRectF &halo,
                               qreal scale) const;
    KittyGlow::GlowConfig m_cfg;
    std::unique_ptr<KWin::GLShader> m_shader;
    // Key autorepeat made Meta+Shift+B flip the rule dozens of times per hold
    // (flicker + final parity depended on hold duration); gate per press.
    QElapsedTimer m_toggleGate;
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

    // LL-019: the halo clip comes from the stacking at paint time, but
    // nothing repainted the halo when stacking CHANGED (window raised above
    // kitty) — the one unclipped transition frame then persisted forever,
    // because an unfocused kitty never repaints by itself. Repaint every
    // kitty halo on raise/lower; the next frame re-clips from fresh state.
    connect(KWin::effects, &KWin::EffectsHandler::stackingOrderChanged, this,
            [this]() { repaintAllKittyHalos(); });

    // LL-019 (stale color): the halo color is picked at paint time from
    // activeWindow(), so the halo stayed ACTIVE-gold after focus moved away
    // until kitty's next repaint. Repaint on every activation change.
    connect(KWin::effects, &KWin::EffectsHandler::windowActivated, this,
            [this](KWin::EffectWindow *) { repaintAllKittyHalos(); });

    // Seamless toggle channel: DBus pull-service + kglowsync poller script
    // (see kittytoggle.h). The script applies noBorder live so the toggle
    // never needs org.kde.KWin.reconfigure() — the LL-016 white flash.
    KittyToggle::init();

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
    // SDF is fragment-position-based, so subdivided partial draws of the same
    // quad are pixel-identical to an unclipped draw.
    // LL-019: occluders are rebuilt on EVERY halo paint — see
    // occludedAboveKitty(); the old 120 ms cache lagged restacks.
    // The scene's paint region for kitty covers only its frame: the ring
    // widening done in prePaintWindow does NOT propagate into paintWindow's
    // region parameter in KWin 5.27.8 (gate logging, 2026-09-10: region∩halo
    // == frame exactly). The rasterizer does not enforce the region — that
    // unenforcement WAS the original LL-020 leak — so the scene region is
    // advisory only. Clip against the OCCLUDERS alone and draw every sub-quad
    // of the full ring; pixels outside the region land correctly, and ring
    // damage tracking is handled by the prePaintWindow widening + the LL-019
    // stacking/activation repaint hooks.
    QRegion clip = halo.toRect();
    clip -= occludedAboveKitty(w, halo, s);
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
    // LL-020 (two-part lesson). (a) render(region, mode, hwClipping=true)
    // documents that the CALLER must enable GL_SCISSOR_TEST; relying on
    // whatever state the stock pipeline left made frames where the test was
    // off paint the FULL quad over the front windows (the glow-penetration
    // artifact). (b) Enabling the test proved worse: KWin 5.27.8's per-rect
    // scissor boxes (Y-flip via GLFramebuffer::currentFramebuffer() height)
    // clip the draw to NOTHING in this paint context (2026-09-10 positive
    // control: 74 px wallpaper noise floor with kitty fully unoccluded).
    // The scissor path is abandoned: CPU-subdivide the halo into ONE QUAD
    // PER CLIP RECT and draw each unclipped via the 1-arg render() overload
    // (single glDrawArrays, no region iteration, no GL state touched).
    // Clip rects share the space the full-quad intersection used; the *s
    // matches the device-px vertex space above (renderTargetScale is 1 on
    // this system, so logical and device px coincide).
    for (const QRect &cr : clip) {
        const float rx0 = static_cast<float>(cr.x() * s);
        const float ry0 = static_cast<float>(cr.y() * s);
        const float rx1 = static_cast<float>((cr.x() + cr.width()) * s);
        const float ry1 = static_cast<float>((cr.y() + cr.height()) * s);
        const float v[12] = {rx0, ry0, rx1, ry0, rx1, ry1,
                             rx0, ry0, rx1, ry1, rx0, ry1};
        vb->setData(6, 2, v, nullptr);
        vb->render(GL_TRIANGLES);
    }
    glDisable(GL_BLEND);
    KWin::ShaderManager::instance()->popShader();
}

// Repaint the full halo ring of every kitty window (stacking/activation
// hooks); the next paint pass recomputes each halo's clip from fresh state.
void KittyGlowEffect::repaintAllKittyHalos() {
    const auto stack = KWin::effects->stackingOrder();
    for (auto *w : stack) {
        if (w && !w->isDeleted() && isKittyWindow(w))
            repaintHalo(w->frameGeometry());
    }
}

// Occluders for ONE halo paint: windows logically stacked ABOVE the PAINTED
// kitty window that paint fully opaque frames, as a device-px region cut out
// of the halo. Rebuilt on EVERY halo paint — the old 120 ms stacking snapshot
// lagged raise/drag transitions (LL-019): one frame drew unclipped and an
// unfocused kitty never repainted it away. Anchoring to the painted window
// (not "the topmost kitty") also fixes the occluder set with 2+ kitty
// windows. Cost: one stackingOrder() walk per halo paint — negligible.
// NOTE: stackingOrder() is the logical bottom→top order and is NOT reordered
// while a window is dragged (elevation is paint-time only).
QRegion KittyGlowEffect::occludedAboveKitty(KWin::EffectWindow *kitty,
                                            const QRectF &halo, qreal scale) const {
    QRegion occl;
    const auto stack = KWin::effects->stackingOrder();
    int kittyIdx = -1;
    for (int i = 0; i < stack.size() && kittyIdx < 0; ++i) {
        if (stack.at(i) == kitty) kittyIdx = i;
    }
    if (kittyIdx < 0) return occl;  // not in stack (closing): draw unclipped
    for (int i = kittyIdx + 1; i < stack.size(); ++i) {
        KWin::EffectWindow *w = stack.at(i);
        if (!w || w->isDeleted() || w->isMinimized()) continue;
        // Desktop windows (plasma's fullscreen desktop containment) are by
        // definition beneath all windows, but KWin 5.27 can leave them high
        // in stackingOrder() after restacks — without this skip such a
        // window would empty the clip and silence the halo entirely
        // (observed during LL-020 verification, 2026-09-10).
        if (w->isDesktop()) continue;
        // Docks/panels are screen chrome and may be translucent (adaptive
        // plasma panel) — they ALWAYS clip the halo (LL-018). Other
        // translucent windows still intentionally let the halo bloom through.
        if (!w->isDock() && w->opacity() < 0.99) continue;
        if (!w->isOnCurrentDesktop() || !w->isOnCurrentActivity()) continue;
        // expandedGeometry() spans the frame AND the window shadow: front
        // windows paint translucent shadow gradients well past frameGeometry(),
        // and clipping only the frame let the halo shine through those shadows
        // (LL-017 penetration artifact).
        const QRectF gf = w->expandedGeometry();
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

    // State-based toggle (kittyglowstate.cpp): the value lives in our own
    // kittyglowrc, so no in-memory forcing rule can fight the kglowsync
    // script's noBorder writes (the 2026-09-09 write-revert fight). The
    // kwinrulesrc kitty rule is retired; see HANDBOOK.md migration note.
    const bool flipped = KittyGlowState::toggleNoBorder();
    // No reconfigure here — that call is the LL-016 white flash. Stage the
    // new value; the kglowsync script applies it to kitty windows within
    // 60 ms and the halo follows via windowFrameGeometryChanged.
    KittyToggle::requestApply(flipped);
    qWarning() << "toggle: flipped noborder ->" << flipped
               << "and staged for kglowsync";
}

KWIN_EFFECT_FACTORY(KittyGlowEffect, "kittyglow.json")
#include "kittyglow.moc"
