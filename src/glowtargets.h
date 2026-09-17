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
    // LL-033 (2026-09-15): reject override-redirect chrome first. EffectWindow
    // has no isUnmanaged(); the managed flag = window->isClient(), captured at
    // construction so effects can still classify popups after Deleted-reparent
    // (kwineffects.h:2588, effects.cpp:2003). Qt QMenu drop-downs, combo popups
    // and tooltips are override-redirect and set no _NET_WM_WINDOW_TYPE, so the
    // type predicates below cannot see them and they inherit the parent's
    // WM_CLASS — without this gate every open menu got its own halo (report:
    // drop-down glow + flickering bottom-edge outline).
    if (!w->isManaged()) return false;
    if (w->isDesktop() || w->isDock()) return false;      // chrome
    if (w->isDialog() || w->isNotification() || w->isOnScreenDisplay())
        return false;                                     // user exclusions
    if (w->isSplash() || w->isTooltip() || w->isPopupWindow() || w->isUtility())
        return false;                                     // transients
    // Class-based chrome exclusion (LL-026): the Qubes GUI proxy does not
    // replicate _NET_WM_WINDOW_TYPE, so every VM-proxied window looks
    // "normal" to KWin and the type predicates above never match it.
    // WM_CLASS is the one property that survives the proxy — use it for
    // the chrome the user excluded 2026-09-11: plasma surfaces (panels,
    // popups, start menu + submenus), Qubes tray-widget source windows
    // (the 16x16 ghosts pinned at (0,0) — the "ghost square", see
    // docs/research/2026-09-11-ghost-square-qubes-tray-ghosts.md), legacy
    // tray embeds, and krunner.
    const QString cls = w->windowClass().toLower();
    if (cls.contains(QLatin1String("plasmashell"))
        || cls.startsWith(QLatin1String("qui-"))
        || cls.contains(QLatin1String("xembedsniproxy"))
        || cls.contains(QLatin1String("krunner")))
        return false;
    // Size guard (2026-09-11, user ghost-square report): sub-48 px windows
    // are icons, not application windows — the Qui-* tray-widget SOURCE
    // windows are override-redirect (KWin unmanaged, invisible in
    // clientList, no WM_CLASS the class check can see) yet their 22 px
    // halo paints as a floating empty square at the screen corner. An
    // icon-size window with a halo is never wanted.
    const QRect fg = w->frameGeometry().toRect();
    if (fg.width() < 48 || fg.height() < 48) return false;
    return true;
}

} // namespace KittyGlowTargets

#endif // GLOWTARGETS_H
