// glowconfig.h — kittyglow effect configuration (kwinrc group [Effect-kittyglow]).
// Note: This code is purely AI-generated.
#pragma once

#include <KConfigGroup>
#include <KSharedConfig>
#include <QColor>

namespace KittyGlow {

// All thickness/radius values are logical pixels; they are scaled by
// effects->renderTargetScale() at draw time (projection works in device px).
struct GlowConfig {
    float widthLeft = 32.0f;
    float widthTop = 32.0f;
    float widthRight = 32.0f;
    float widthBottom = 32.0f;
    float cornerRadius = 8.0f;
    QColor colorActive = QColor(255, 215, 0, 153);    // gold @ 60%
    QColor colorInactive = QColor(255, 215, 0, 77);   // gold @ 30%
    bool enabled = true;

    int maxExtent() const {
        int m = static_cast<int>(widthLeft);
        m = qMax(m, static_cast<int>(widthTop));
        m = qMax(m, static_cast<int>(widthRight));
        m = qMax(m, static_cast<int>(widthBottom));
        return m;
    }
};

inline GlowConfig loadGlowConfig(const KSharedConfigPtr &cfg)
{
    GlowConfig c;
    const KConfigGroup g = cfg->group(QStringLiteral("Effect-kittyglow"));
    const float def = g.readEntry("GlowRadius", 32);
    c.widthLeft = g.readEntry("GlowLeft", def);
    c.widthTop = g.readEntry("GlowTop", def);
    c.widthRight = g.readEntry("GlowRight", def);
    c.widthBottom = g.readEntry("GlowBottom", def);
    // Audit L5: bound the widths (cornerRadius was already bounded) — a
    // giant GlowLeft would otherwise be honored as a giant damage region.
    c.widthLeft = qBound(0.0f, c.widthLeft, 200.0f);
    c.widthTop = qBound(0.0f, c.widthTop, 200.0f);
    c.widthRight = qBound(0.0f, c.widthRight, 200.0f);
    c.widthBottom = qBound(0.0f, c.widthBottom, 200.0f);
    c.cornerRadius = g.readEntry("GlowCornerRadius", 8.0f);
    c.cornerRadius = qBound(0.0f, c.cornerRadius, 64.0f);

    // Audit L4: parse R,G,B (3 parts); a 4th alpha part is tolerated for
    // compatibility but ALWAYS overridden by GlowOpacity below — and every
    // component is clamped to 0..255 (QColor(int,int,int) is UB out of
    // range).
    const auto readColor = [&g](const char *key, const QColor &fallback) {
        const QString s = g.readEntry(key, QString());
        if (s.isEmpty()) return fallback;
        const QStringList parts = s.split(QLatin1Char(','));
        bool ok = (parts.size() == 3 || parts.size() == 4);
        int v[3] = {255, 215, 0};
        if (ok) {
            for (int i = 0; i < 3; ++i) {
                v[i] = qBound(0, parts.at(i).toInt(&ok), 255);
                if (!ok) break;
            }
        }
        if (!ok) return fallback;
        return QColor(v[0], v[1], v[2]);
    };

    QColor active = readColor("GlowColor", QColor(255, 215, 0));
    QColor inactive = readColor("GlowColorInactive", QColor(255, 215, 0));
    const int opacity = qBound(0, g.readEntry("GlowOpacity", 60), 100);
    const int opacityInactive = qBound(0, g.readEntry("GlowOpacityInactive", 30), 100);
    active.setAlphaF(opacity / 100.0);
    inactive.setAlphaF(opacityInactive / 100.0);
    c.colorActive = active;
    c.colorInactive = inactive;
    c.enabled = g.readEntry("Enabled", true);
    return c;
}

}  // namespace KittyGlow
