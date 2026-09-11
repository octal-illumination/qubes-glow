// glowtargets.h — halo eligibility predicate: which windows get the glow.
// Note: This code is purely AI-generated.
//
// Single responsibility: decide glow eligibility. Extracted from
// kittyglow.cpp (Rule 13) when the halo generalized from kitty-only to
// all application windows (build #15, 2026-09-10).
#ifndef GLOWTARGETS_H
#define GLOWTARGETS_H

#include <kwineffects.h>

namespace KittyGlowTargets {

// The halo adorns every real application window (active gold, inactive
// dim). Excluded: screen chrome (desktop, docks/panels) and the surfaces
// the user excluded (2026-09-10: dialog boxes and notifications) plus
// their kin — OSD banners, splash screens, tooltips, popup/combo menus
// and utility palettes. None of those frame "an application": a halo on
// a menu or a transient tooltip reads as a glitch, and notifications
// must never carry persistent screen furniture.
inline bool isGlowWindow(KWin::EffectWindow *w) {
    if (!w || w->isDeleted()) return false;
    if (w->isDesktop() || w->isDock()) return false;      // chrome
    if (w->isDialog() || w->isNotification() || w->isOnScreenDisplay())
        return false;                                     // user exclusions
    if (w->isSplash() || w->isTooltip() || w->isPopupWindow() || w->isUtility())
        return false;                                     // transients
    return true;
}

} // namespace KittyGlowTargets

#endif // GLOWTARGETS_H
