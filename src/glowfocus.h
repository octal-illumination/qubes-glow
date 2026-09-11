// glowfocus — per-window (focused) glow override state, build #18 v3.10.
// Note: This code is purely AI-generated.
//
// Single responsibility: the runtime-only set of windows whose glow the
// user toggled OFF with the per-window Meta+Shift+G. Global glow stays in
// kittyglowrc (KittyGlowState); per-window overrides deliberately do NOT
// persist — every kwin restart restores the launch default (glow on for
// all app windows; user directive 2026-09-11: "glow should be default
// when launching").
//
// The set holds EffectWindow pointers, so entries MUST be pruned when the
// window is destroyed (kittyglow.cpp connects windowDeleted → pruneWindow);
// a stale entry would be a dangling pointer consulted by the paint path.
#ifndef GLOWFOCUS_H
#define GLOWFOCUS_H

namespace KWin { class EffectWindow; }

namespace GlowFocus {

// Flip the per-window glow override for w; returns whether glow is now ON
// for w (false = the user disabled the halo on this window).
bool toggleGlow(KWin::EffectWindow *w);

// True when the window's halo may paint (the global master switch is
// checked separately in the paint path).
bool glowAllowed(KWin::EffectWindow *w);

// Drop w's entry (window destroyed) — keeps the set dangling-free.
void pruneWindow(KWin::EffectWindow *w);

// Drop all per-window overrides (a global glow toggle resets every
// per-window choice back to the launch default).
void clear();

}  // namespace GlowFocus

#endif  // GLOWFOCUS_H
