// kittyglow — v3.9: ALL-WINDOW occlusion-clipped SDF glow (per-paint
// occluder rebuild, CPU-subdivided halo quads — LL-020), animation-mapped
// halo. Meta+Shift+G = glow master switch; Meta+Shift+B = class-wide
// borderless toggle (build #16, 2026-09-11).
// Note: This code is purely AI-generated.
//
// Render path: one hardware-blended triangle fan quad around the frame rect;
// the GLSL SDF shader (glowshader.cpp) computes per-side soft falloff, so
// thickness/corner radius/colors come from kwinrc [Effect-kittyglow] and are
// live-reloadable via org.kde.kwin.Effects.reconfigureEffect. The quad is
// mapped through the scene's animation transform (scale/translation on
// WindowPaintData), so the halo tracks minimize/restore mid-flight instead of
// snapping to the destination geometry, and fades with data.opacity().
// Eligibility: every real application window — dialogs, notifications,
// OSDs, splashes, tooltips, popups, utility palettes, desktop and
// docks/panels are excluded by type (glowtargets.h); plasma surfaces,
// Qubes tray-widget ghosts, xembedsniproxy and krunner are excluded by
// window class because qubes-gui strips _NET_WM_WINDOW_TYPE (LL-026).
//
// Meta+Shift+G toggles the GLOW globally: flips kittyglowrc
// [General] glowEnabled and addRepaintFull()s — live, no restart, no flash.
// Meta+Shift+B toggles class-wide BORDERLESS (frame+titlebar) for every
// eligible app window: flips kittyglowrc [General] noBorder and stages the
// new value for the kglowsync script (kittytoggle nextSource channel),
// which writes Client.noBorder live. (Build #15 briefly repurposed B as
// the glow switch — reverted by user directive 2026-09-11. Per-window
// borderless remains on Meta+Shift+T, KWin's native action.)
#include "glowconfig.h"
#include "glowshader.h"
#include "glowtargets.h"

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

// Eligibility moved to glowtargets.h (Rule 13) when the halo generalized
// to all application windows (build #15).

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
    void toggleBorderless();
    void toggleGlow();
    void repaintHalo(const QRectF &frame);
    void repaintAllGlowHalos();
    QRegion occludedAbove(KWin::EffectWindow *painted, const QRectF &halo,
                          qreal scale) const;
    KittyGlow::GlowConfig m_cfg;
    std::unique_ptr<KWin::GLShader> m_shader;
    // Glow master switch (Meta+Shift+G since build #16); persisted in kittyglowrc.
    bool m_glowEnabled = true;
    // Key autorepeat made the toggles flip their state dozens of times per
    // hold (flicker + final parity depended on hold duration); gate per press.
    QElapsedTimer m_toggleGate;
};

KittyGlowEffect::KittyGlowEffect() {
    // Meta+Shift+B — class-wide border (frame+titlebar) toggle, applied
    // live by the kglowsync script to every eligible app window (user
    // directive 2026-09-11: "extend that for all windows"). objectName
    // kept as the historical "Toggle Kitty Borderless": it is the
    // kglobalaccel registration id, and re-registering a new id for the
    // same binding risks the daemon silently rejecting the key.
    QAction *b = new QAction(this);
    b->setObjectName(QStringLiteral("Toggle Kitty Borderless"));
    b->setText(QStringLiteral("Toggle Window Borders"));
    KGlobalAccel::self()->setDefaultShortcut(
        b, QList<QKeySequence>() << (Qt::META | Qt::SHIFT | Qt::Key_B));
    KGlobalAccel::self()->setShortcut(
        b, QList<QKeySequence>() << (Qt::META | Qt::SHIFT | Qt::Key_B));
    connect(b, &QAction::triggered, this, &KittyGlowEffect::toggleBorderless);

    // Meta+Shift+G — glow master switch (user directive 2026-09-11: the
    // glow toggle moves off B, which returns to border/titlebar duty).
    QAction *g = new QAction(this);
    g->setObjectName(QStringLiteral("Toggle Glow"));
    g->setText(QStringLiteral("Toggle Glow"));
    KGlobalAccel::self()->setDefaultShortcut(
        g, QList<QKeySequence>() << (Qt::META | Qt::SHIFT | Qt::Key_G));
    KGlobalAccel::self()->setShortcut(
        g, QList<QKeySequence>() << (Qt::META | Qt::SHIFT | Qt::Key_G));
    connect(g, &QAction::triggered, this, &KittyGlowEffect::toggleGlow);

    // The halo lies OUTSIDE the frame rect, so damage must be widened or the
    // ring smears during moves and lingers after minimize. Repaint the halo
    // area of both the new and the old geometry, and on minimize/unminimize.
    connect(KWin::effects, &KWin::EffectsHandler::windowFrameGeometryChanged, this,
            [this](KWin::EffectWindow *w, const QRectF &old) {
        if (!KittyGlowTargets::isGlowWindow(w)) return;
        repaintHalo(w->frameGeometry());
        repaintHalo(old);
    });
    connect(KWin::effects, &KWin::EffectsHandler::windowMinimized, this,
            [this](KWin::EffectWindow *w) {
        if (KittyGlowTargets::isGlowWindow(w)) repaintHalo(w->frameGeometry());
    });
    connect(KWin::effects, &KWin::EffectsHandler::windowUnminimized, this,
            [this](KWin::EffectWindow *w) {
        if (KittyGlowTargets::isGlowWindow(w)) repaintHalo(w->frameGeometry());
    });

    // LL-019: the halo clip comes from the stacking at paint time, but
    // nothing repainted the halo when stacking CHANGED (window raised above
    // kitty) — the one unclipped transition frame then persisted forever,
    // because an unfocused kitty never repaints by itself. Repaint every
    // glow halo on raise/lower; the next frame re-clips from fresh state.
    connect(KWin::effects, &KWin::EffectsHandler::stackingOrderChanged, this,
            [this]() { repaintAllGlowHalos(); });

    // LL-019 (stale color): the halo color is picked at paint time from
    // activeWindow(), so a halo stayed ACTIVE-gold after focus moved away
    // until the window's next repaint. Repaint on every activation change.
    connect(KWin::effects, &KWin::EffectsHandler::windowActivated, this,
            [this](KWin::EffectWindow *) { repaintAllGlowHalos(); });

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
    m_glowEnabled = KittyGlowState::loadGlowEnabled();
    if (KWin::effects->isOpenGLCompositing()) {
        KWin::effects->makeOpenGLContextCurrent();
        m_shader = KittyGlow::GlowShader::create();  // caller-owned; rebuild here
    }
    KWin::effects->addRepaintFull();
}

void KittyGlowEffect::prePaintWindow(KWin::EffectWindow *w, KWin::WindowPrePaintData &data,
                                     std::chrono::milliseconds presentTime) {
    KWin::effects->prePaintWindow(w, data, presentTime);
    if (!m_glowEnabled || !KittyGlowTargets::isGlowWindow(w)) return;
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
    if (!m_glowEnabled || !KittyGlowTargets::isGlowWindow(w)) return;  // glow master switch + eligibility
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
    // occludedAbove(); the old 120 ms cache lagged restacks.
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
    clip -= occludedAbove(w, halo, s);
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

// Repaint the full halo ring of every glow window (stacking/activation
// hooks); the next paint pass recomputes each halo's clip from fresh state.
void KittyGlowEffect::repaintAllGlowHalos() {
    const auto stack = KWin::effects->stackingOrder();
    for (auto *w : stack) {
        if (w && !w->isDeleted() && KittyGlowTargets::isGlowWindow(w))
            repaintHalo(w->frameGeometry());
    }
}

// Occluders for ONE halo paint: windows logically stacked ABOVE the PAINTED
// window that paint fully opaque frames, as a device-px region cut out of
// the halo. Rebuilt on EVERY halo paint — the old 120 ms stacking snapshot
// lagged raise/drag transitions (LL-019): one frame drew unclipped and an
// unfocused window never repainted it away. Anchoring to the painted window
// (not "the topmost window") also keeps the occluder set correct with any
// number of stacked windows. Cost: one stackingOrder() walk per halo paint —
// negligible.
// NOTE: stackingOrder() is the logical bottom→top order and is NOT reordered
// while a window is dragged (elevation is paint-time only).
QRegion KittyGlowEffect::occludedAbove(KWin::EffectWindow *painted,
                                       const QRectF &halo, qreal scale) const {
    QRegion occl;
    const auto stack = KWin::effects->stackingOrder();
    int paintedIdx = -1;
    for (int i = 0; i < stack.size() && paintedIdx < 0; ++i) {
        if (stack.at(i) == painted) paintedIdx = i;
    }
    if (paintedIdx < 0) return occl;  // not in stack (closing): draw unclipped
    for (int i = paintedIdx + 1; i < stack.size(); ++i) {
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
        // Docks/panels keep expandedGeometry(): they must ALWAYS clip the
        // halo (LL-018) and carry no meaningful soft shadow to dim it.
        // Normal windows clip at frameGeometry() ONLY: the halo paints
        // BENEATH them, so their translucent shadow gradient dims it
        // progressively right up to the frame edge — a seamless pass-behind
        // with no wallpaper gap between halo-end and the occluding border
        // (user report 2026-09-10; supersedes the LL-017 hard cut at
        // expandedGeometry, which was the right call for the old full-alpha
        // leak but wrong for the occluder-clipped architecture).
        const QRectF gf = w->isDock() ? w->expandedGeometry()
                                      : w->frameGeometry();
        // Device-px rect (+1 px fatten so no halo seam shows at occluder
        // edges), same top-left-origin space as the paint region —
        // GLVertexBuffer::draw() flips scissor rects for GL itself.
        occl += QRect(static_cast<int>(gf.x() * scale), static_cast<int>(gf.y() * scale),
                      static_cast<int>(gf.width() * scale) + 1,
                      static_cast<int>(gf.height() * scale) + 1);
    }
    return occl.intersected(halo.toRect());
}

void KittyGlowEffect::toggleBorderless() {
    // Same autorepeat gate as the glow toggle: one flip per physical press
    // (repeats arrive 25-33 ms apart and keep restarting the timer).
    if (m_toggleGate.isValid() && !m_toggleGate.hasExpired(220)) return;
    m_toggleGate.start();

    // Class-wide borderless (build #16, user directive 2026-09-11): persist
    // in kittyglowrc so the kglowsync bootstrap restores it after kwin
    // restarts, and stage for the script's 60 ms poll — live, no restart,
    // no reconfigure flash. Which windows receive noBorder is the script's
    // mirror of KittyGlowTargets (LL-026).
    const bool next = KittyGlowState::toggleNoBorder();
    KittyToggle::requestApply(next);
    qWarning() << "toggle: borderless ->" << (next ? "on" : "off");
}

void KittyGlowEffect::toggleGlow() {
    // kglobalaccel re-emits triggered() for every autorepeat of a held key;
    // gate to one toggle per physical press (repeats arrive 25-33 ms apart
    // and keep restarting the timer; a release + new press is always later
    // than the 220 ms window).
    if (m_toggleGate.isValid() && !m_toggleGate.hasExpired(220)) return;
    m_toggleGate.start();

    // Glow master switch (Meta+Shift+G since build #16; B carried it only
    // in build #15): persisted in kittyglowrc so it survives kwin
    // restarts; a full repaint re-evaluates every window's halo on the
    // very next frame — live, no restart, no flash (the LL-016 white
    // flash came from reconfigure(), which we avoid). The borderless
    // plumbing (kglowsync) is NOT touched here: windows keep their
    // persisted borderless state (Meta+Shift+B); per-window borderless =
    // Meta+Shift+T.
    m_glowEnabled = KittyGlowState::toggleGlowEnabled();
    KWin::effects->addRepaintFull();
    qWarning() << "toggle: glow ->" << (m_glowEnabled ? "on" : "off");
}

KWIN_EFFECT_FACTORY(KittyGlowEffect, "kittyglow.json")
#include "kittyglow.moc"
