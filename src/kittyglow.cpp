// kittyglow — v3.12: unmanaged-popup exclusion (build #22, 2026-09-15,
// LL-033). v3.11: per-VM Qubes label hue (build #21, 2026-09-11).
// v3.10.1: per-window toggles + global masters (build #20, re-audit 2
// space corrections). ALL-WINDOW occlusion-clipped
// SDF glow (per-paint occluder
// rebuild, CPU-subdivided halo quads — LL-020), animation-mapped halo.
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
// Meta+Shift+G toggles the GLOW on the FOCUSED window only (runtime-only
// override; glow stays ON at launch for every app window). Meta+Shift+B
// toggles BORDER/TITLEBAR on the FOCUSED window only (runtime-only; windows
// launch borderless). The GLOBAL masters moved to Meta+Shift+Alt+G / B:
// those flip kittyglowrc [General] glowEnabled / noBorder and (for borders)
// stage the sweep for the kglowsync script (kittytoggle nextSource
// channel), which writes Client.noBorder live. Per-window border flips
// travel the same bus as a window-op command (kittytoggle nextWindowOp);
// the script resolves the focused window itself and shields that window
// from the 400 ms safety-net sweep via its overrides map. (Build #15
// briefly repurposed B as the glow switch — reverted by user directive
// 2026-09-11. Per-window borderless on Meta+Shift+T remains KWin's native
// per-window action.)
//
// COLOR (build #21, v3.11): with LabelColor=true (default) the halo hue of
// each VM window comes from its Qubes label — _QUBES_LABEL_COLOR
// (0x00RRGGBB) set by qubes-guid on every proxied window; active/inactive
// remains an opacity distinction on that hue. Dom0-native windows and
// LabelColor=false windows keep the configured gold (LL-032).
#include "glowconfig.h"
#include "glowfocus.h"
#include "glowlabel.h"
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
    void toggleBorderlessFocused();   // Meta+Shift+B: focused window only
    void toggleGlowFocused();         // Meta+Shift+G: focused window only
    void toggleBorderlessGlobal();    // Meta+Shift+Alt+B: sweep + persist
    void toggleGlowGlobal();          // Meta+Shift+Alt+G: master + persist
    void repaintHalo(const QRectF &frame);
    void repaintAllGlowHalos();
    QRegion occludedAbove(KWin::EffectWindow *painted, const QRectF &halo) const;
    KittyGlow::GlowConfig m_cfg;
    std::unique_ptr<KWin::GLShader> m_shader;
    // Glow master switch (Meta+Shift+G since build #16); persisted in kittyglowrc.
    bool m_glowEnabled = true;
    // Key autorepeat made the toggles flip their state dozens of times per
    // hold (flicker + final parity depended on hold duration). ONE shared
    // gate dropped CROSS-toggle presses (B then G within 220 ms lost G —
    // audit finding M3): every toggle owns its gate now.
    QElapsedTimer m_gateBorderFocused;
    QElapsedTimer m_gateGlowFocused;
    QElapsedTimer m_gateBorderGlobal;
    QElapsedTimer m_gateGlowGlobal;

    // One flip per physical press: true = this press is a repeat/duplicate.
    bool gated(QElapsedTimer &t) {
        if (t.isValid() && !t.hasExpired(220)) return true;
        t.start();
        return false;
    }
};

KittyGlowEffect::KittyGlowEffect() {
    // Meta+Shift+B — FOCUSED-window border (frame+titlebar) toggle (build
    // #18, user directive 2026-09-11: toggles act on the focused window,
    // not globally). The command travels the kittytoggle nextWindowOp
    // channel; the kglowsync script resolves ITS focused window and shields
    // it from the 400 ms sweep. objectName kept as the historical "Toggle
    // Kitty Borderless": it is the kglobalaccel registration id, and
    // re-registering a new id for the same binding risks the daemon
    // silently rejecting the key.
    QAction *b = new QAction(this);
    b->setObjectName(QStringLiteral("Toggle Kitty Borderless"));
    b->setText(QStringLiteral("Toggle Window Borders"));
    KGlobalAccel::self()->setDefaultShortcut(
        b, QList<QKeySequence>() << (Qt::META | Qt::SHIFT | Qt::Key_B));
    KGlobalAccel::self()->setShortcut(
        b, QList<QKeySequence>() << (Qt::META | Qt::SHIFT | Qt::Key_B));
    connect(b, &QAction::triggered, this, &KittyGlowEffect::toggleBorderlessFocused);

    // Meta+Shift+G — FOCUSED-window glow toggle (build #18; the global
    // master moved to Meta+Shift+Alt+G).
    QAction *g = new QAction(this);
    g->setObjectName(QStringLiteral("Toggle Glow"));
    g->setText(QStringLiteral("Toggle Glow"));
    KGlobalAccel::self()->setDefaultShortcut(
        g, QList<QKeySequence>() << (Qt::META | Qt::SHIFT | Qt::Key_G));
    KGlobalAccel::self()->setShortcut(
        g, QList<QKeySequence>() << (Qt::META | Qt::SHIFT | Qt::Key_G));
    connect(g, &QAction::triggered, this, &KittyGlowEffect::toggleGlowFocused);

    // Meta+Shift+Alt+B — GLOBAL border master (build #18): the old
    // class-wide sweep, persisted in kittyglowrc + re-applied by the
    // script's bootstrap.
    QAction *bAll = new QAction(this);
    bAll->setObjectName(QStringLiteral("Toggle Borders All Windows"));
    bAll->setText(QStringLiteral("Toggle Borders All Windows"));
    KGlobalAccel::self()->setDefaultShortcut(
        bAll, QList<QKeySequence>() << (Qt::META | Qt::SHIFT | Qt::ALT | Qt::Key_B));
    KGlobalAccel::self()->setShortcut(
        bAll, QList<QKeySequence>() << (Qt::META | Qt::SHIFT | Qt::ALT | Qt::Key_B));
    connect(bAll, &QAction::triggered, this, &KittyGlowEffect::toggleBorderlessGlobal);

    // Meta+Shift+Alt+G — GLOBAL glow master (build #18): flips the
    // persisted glowEnabled and resets every per-window glow override.
    QAction *gAll = new QAction(this);
    gAll->setObjectName(QStringLiteral("Toggle Glow All Windows"));
    gAll->setText(QStringLiteral("Toggle Glow All Windows"));
    KGlobalAccel::self()->setDefaultShortcut(
        gAll, QList<QKeySequence>() << (Qt::META | Qt::SHIFT | Qt::ALT | Qt::Key_G));
    KGlobalAccel::self()->setShortcut(
        gAll, QList<QKeySequence>() << (Qt::META | Qt::SHIFT | Qt::ALT | Qt::Key_G));
    connect(gAll, &QAction::triggered, this, &KittyGlowEffect::toggleGlowGlobal);

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

    // Build #18: per-window glow overrides hold EffectWindow pointers —
    // prune the destroyed window's entry or the paint path dereferences a
    // dangling pointer on the next frame (glowfocus.h). The label-hue cache
    // holds the same pointers — same pruning contract (glowlabel.h).
    connect(KWin::effects, &KWin::EffectsHandler::windowDeleted, this,
            [](KWin::EffectWindow *w) {
                GlowFocus::pruneWindow(w);
                GlowLabel::pruneWindow(w);
            });

    // Seamless toggle channel: DBus pull-service + kglowsync poller script
    // (see kittytoggle.h). The script applies noBorder live so the toggle
    // never needs org.kde.KWin.reconfigure() — the LL-016 white flash.
    KittyToggle::init();

    reconfigure(ReconfigureAll);
}

void KittyGlowEffect::repaintHalo(const QRectF &frame) {
    // LOGICAL-px widening (re-audit 2 F1): every effect-facing region —
    // damage, addRepaint, occluders — is logical px in KWin 5.27.8; the ONE
    // scale boundary is the vertex upload (see prePaintWindow).
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
    if (!m_glowEnabled || !GlowFocus::glowAllowed(w)
        || !KittyGlowTargets::isGlowWindow(w)) return;
    // LOGICAL-px widening (re-audit 2 F1 — supersedes audit M2): damage
    // regions are logical in KWin 5.27.8 (Scene::addRepaint intersects the
    // logical viewport unscaled, scene.cpp:92; the GL scissor converts via
    // mapToRenderTarget internally, itemrenderer_opengl.cpp:331). The OLD
    // pre-M2 logical widening was right; M2's device-px version over-widened
    // at s > 1 (benign) and LL-028 recorded the wrong rule. The ONE scale
    // boundary is the vertex upload in paintWindow, which maps logical
    // geometry by renderTargetScale * animation scale.
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
    if (!m_glowEnabled || !GlowFocus::glowAllowed(w)
        || !KittyGlowTargets::isGlowWindow(w)) return;  // master + per-window + eligibility
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
    // Translation is LOGICAL px (PaintData::toMatrix applies * deviceScale
    // internally, kwineffects.cpp:208; blur consumes it unscaled) — re-audit
    // 2 F3. The old * s double-scaled it at s > 1: once here, once via the
    // geo upload below.
    const QPointF tr(data.xTranslation(), data.yTranslation());
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
    clip -= occludedAbove(w, halo);
    if (clip.isEmpty()) return;

    QColor color = KWin::effects->activeWindow() == w ? m_cfg.colorActive
                                                      : m_cfg.colorInactive;
    // Per-VM label hue (build #21, v3.11): resolve the Qubes label color
    // and re-apply the active/inactive OPACITY on top, so the distinction
    // stays an alpha one regardless of hue (LL-032). Cached one read per
    // window lifetime — never a per-frame property round trip.
    if (m_cfg.labelColor) {
        const QColor label = GlowLabel::colorFor(w);
        if (label.isValid()) {
            const float op = (KWin::effects->activeWindow() == w)
                                 ? static_cast<float>(m_cfg.colorActive.alphaF())
                                 : static_cast<float>(m_cfg.colorInactive.alphaF());
            color = label;
            color.setAlphaF(op);
        }
    }
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
// window that paint fully opaque frames, as a LOGICAL-px region cut out of
// the halo (re-audit 2 F2: the old device-px rects were consumed logically
// and re-scaled at upload — double-scaled at s > 1, over-clipping the halo).
// Rebuilt on EVERY halo paint — the old 120 ms stacking snapshot
// lagged raise/drag transitions (LL-019): one frame drew unclipped and an
// unfocused window never repainted it away. Anchoring to the painted window
// (not "the topmost window") also keeps the occluder set correct with any
// number of stacked windows. Cost: one stackingOrder() walk per halo paint —
// negligible.
// NOTE: stackingOrder() is the logical bottom→top order and is NOT reordered
// while a window is dragged (elevation is paint-time only).
QRegion KittyGlowEffect::occludedAbove(KWin::EffectWindow *painted,
                                       const QRectF &halo) const {
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
        // LOGICAL-px rect (+1 px fatten so no halo seam shows at occluder
        // edges) — same top-left-origin space as the halo rect it is
        // subtracted from and the paint region (see F2 note above).
        occl += QRect(static_cast<int>(gf.x()), static_cast<int>(gf.y()),
                      static_cast<int>(gf.width()) + 1,
                      static_cast<int>(gf.height()) + 1);
    }
    return occl.intersected(halo.toRect());
}

void KittyGlowEffect::toggleBorderlessFocused() {
    // Autorepeat gate: one flip per physical press (own gate — M3).
    if (gated(m_gateBorderFocused)) return;

    // Focused-window border toggle (build #18, user directive: "toggling
    // glow and titlebar and border should be for the focused window not
    // globally"). The effect stages a window-op; the kglowsync script
    // resolves ITS focused window on the next 60 ms poll, flips that
    // window's noBorder and shields it from the 400 ms sweep via its
    // overrides map (runtime-only — a restart restores the launch default:
    // borderless).
    KittyToggle::requestWindowOp(1);
    qWarning() << "toggle: border (focused window) staged";
}

void KittyGlowEffect::toggleGlowFocused() {
    if (gated(m_gateGlowFocused)) return;

    // Focused-window glow toggle (build #18): runtime-only override in the
    // GlowFocus set; the persisted global default is untouched, so the
    // halo returns at the next kwin restart (launch default: glow on).
    KWin::EffectWindow *w = KWin::effects->activeWindow();
    if (!w || !KittyGlowTargets::isGlowWindow(w)) {
        qWarning() << "toggle: glow (focused) — no eligible focused window";
        return;
    }
    const bool on = GlowFocus::toggleGlow(w);
    KWin::effects->addRepaintFull();
    qWarning() << "toggle: glow (focused" << w->windowClass() << ") ->"
               << (on ? "on" : "off");
}

void KittyGlowEffect::toggleBorderlessGlobal() {
    // Autorepeat gate (repeats arrive 25-33 ms apart and keep restarting
    // the timer): one flip per physical press (own gate — M3).
    if (gated(m_gateBorderGlobal)) return;

    // GLOBAL border master (build #18, Meta+Shift+Alt+B — the build #16
    // class-wide sweep moved here): persist in kittyglowrc so the kglowsync
    // bootstrap restores it after kwin restarts, and stage for the script's
    // 60 ms poll — live, no restart, no reconfigure flash. The sweep
    // re-imposes the global default on every eligible window, resetting any
    // per-window overrides (documented reset semantics). Which windows
    // receive noBorder is the script's mirror of KittyGlowTargets (LL-026).
    const bool next = KittyGlowState::toggleNoBorder();
    KittyToggle::requestApply(next);
    qWarning() << "toggle: borderless (global) ->" << (next ? "on" : "off");
}

void KittyGlowEffect::toggleGlowGlobal() {
    if (gated(m_gateGlowGlobal)) return;

    // GLOBAL glow master (build #18, Meta+Shift+Alt+G): persisted in
    // kittyglowrc so it survives kwin restarts; a full repaint re-evaluates
    // every window's halo on the very next frame — live, no restart, no
    // flash (the LL-016 white flash came from reconfigure(), which we
    // avoid). Also resets every per-window glow override: the master switch
    // means "everything back to the new default".
    m_glowEnabled = KittyGlowState::toggleGlowEnabled();
    GlowFocus::clear();
    KWin::effects->addRepaintFull();
    qWarning() << "toggle: glow (global) ->" << (m_glowEnabled ? "on" : "off");
}

KWIN_EFFECT_FACTORY(KittyGlowEffect, "kittyglow.json")
#include "kittyglow.moc"
