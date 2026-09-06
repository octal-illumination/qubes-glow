// Kitty Glow -- soft yellow halo around kitty terminal windows.
// Purely AI-generated KWin (Plasma 5.27) compositor effect (MVP: layered-alpha halo).
#include <epoxy/gl.h>
#include <kwineffects.h>
#include <kwinglplatform.h>
#include <kwinglutils.h>

#include <QColor>
#include <QRect>
#include <QSet>

class KittyGlowEffect : public KWin::Effect
{
    Q_OBJECT

public:
    KittyGlowEffect() {
        connect(KWin::effects, &KWin::EffectsHandler::windowAdded,
                this, &KittyGlowEffect::slotWindowAdded);
        connect(KWin::effects, &KWin::EffectsHandler::windowDeleted,
                this, &KittyGlowEffect::slotWindowDeleted);
        const auto list = KWin::effects->stackingOrder();
        for (KWin::EffectWindow *w : list) {
            slotWindowAdded(w);
        }
    }

    bool isActive() const override { return !m_windows.isEmpty(); }

    void paintWindow(KWin::EffectWindow *w, int mask, QRegion region,
                    KWin::WindowPaintData &data) override {
        if (m_windows.contains(w) &&
            KWin::effects->compositingType() == KWin::OpenGLCompositing) {
            drawGlow(w);
        }
        KWin::effects->paintWindow(w, mask, region, data);
    }

private Q_SLOTS:
    void slotWindowAdded(KWin::EffectWindow *w) {
        if (w && w->windowClass().contains(QStringLiteral("kitty"), Qt::CaseInsensitive)) {
            m_windows.insert(w);
        }
    }
    void slotWindowDeleted(KWin::EffectWindow *w) {
        m_windows.remove(w);
    }

private:
    void drawGlow(KWin::EffectWindow *w) {
        if (w->isFullScreen()) {
            return; // don't tint the entire screen
        }
        const QRect g = w->frameGeometry().toRect();
        const int margin = 22;   // halo thickness in px
        const int layers = 8;    // stacked translucent rects => soft falloff
        for (int i = 0; i < layers; ++i) {
            const float t = float(i) / float(layers - 1); // 0 outer .. 1 inner
            const int inset = int(margin * (1.0f - t));
            const QRect r = g.adjusted(-margin + inset, -margin + inset,
                                       margin - inset, margin - inset);
            const int alpha = int(70 * t); // brighter toward the window edge
            paintQuad(r, QColor(255, 221, 0, alpha));
        }
    }

    void paintQuad(const QRect &r, const QColor &c) {
        KWin::ShaderManager *sm = KWin::ShaderManager::instance();
        KWin::GLShader *shader = sm->pushShader(KWin::ShaderTrait::UniformColor);
        KWin::GLVertexBuffer *vb = KWin::GLVertexBuffer::streamingBuffer();
        const float verts[8] = {
            float(r.x()),                 float(r.y()),
            float(r.x() + r.width()),     float(r.y()),
            float(r.x() + r.width()),     float(r.y() + r.height()),
            float(r.x()),                 float(r.y() + r.height())
        };
        vb->setColor(c);
        vb->setData(4, 2, verts, nullptr);
        vb->render(GL_TRIANGLE_STRIP);
        sm->popShader();
    }

    QSet<KWin::EffectWindow *> m_windows;
};

KWIN_EFFECT_FACTORY(KittyGlowEffect, "kittyglow.json")

#include "kittyglow.moc"
