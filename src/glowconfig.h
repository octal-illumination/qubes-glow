// glowconfig.h — kittyglow effect configuration (kwinrc group [Effect-kittyglow]).
// Note: This code is purely AI-generated.
#pragma once

#include <KConfigGroup>
#include <KSharedConfig>
#include <QColor>
#include <cmath>

namespace KittyGlow {

// All thickness/radius values are logical pixels; they are scaled by
// effects->renderTargetScale() at draw time (projection works in device px).
struct GlowConfig {
    float widthLeft = 16.0f;
    float widthTop = 16.0f;
    float widthRight = 16.0f;
    float widthBottom = 16.0f;
    float cornerRadius = 8.0f;
    QColor colorActive = QColor(255, 215, 0, 153);    // gold @ 60%
    QColor colorInactive = QColor(255, 215, 0, 77);   // gold @ 30%
    bool enabled = true;
    // Build #21 (v3.11, LL-032): take each VM window's hue from its Qubes
    // label (_QUBES_LABEL_COLOR) instead of the configured colors;
    // dom0-native windows always fall back to the configured gold.
    bool labelColor = true;

    int maxExtent() const {
        // Ceil, not truncate (re-audit 2 m2): the draw path uses the float
        // width, so truncation could under-widen the damage by up to 1
        // logical px.
        int m = static_cast<int>(std::ceil(widthLeft));
        m = qMax(m, static_cast<int>(std::ceil(widthTop)));
        m = qMax(m, static_cast<int>(std::ceil(widthRight)));
        m = qMax(m, static_cast<int>(std::ceil(widthBottom)));
        return m;
    }
};

inline GlowConfig loadGlowConfig(const KSharedConfigPtr &cfg)
{
    GlowConfig c;
    const KConfigGroup g = cfg->group(QStringLiteral("Effect-kittyglow"));
    const float def = g.readEntry("GlowRadius", 16);
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
    c.labelColor = g.readEntry("LabelColor", true);
    return c;
}

}  // namespace KittyGlow
