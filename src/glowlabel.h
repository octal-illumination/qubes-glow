// glowlabel — per-VM Qubes label color source, build #21 (v3.11).
// Note: This code is purely AI-generated.
//
// Single responsibility: resolve a window's halo hue from its Qubes VM
// label. qubes-guid sets _QUBES_LABEL_COLOR (CARDINAL, plain 0x00RRGGBB)
// on every VM-proxied window — xprop evidence 2026-09-11: Dev-General =
// 15586304 = 0x00EDD400 = its yellow qvm-ls label. Dom0-native windows
// carry no such atom and fall back to the configured colors (LL-032).
//
// The property read is ONE synchronous xcb round trip per window
// LIFETIME — cached, and misses are cached too (sentinel bit 31 marks
// "present") so dom0-native windows never re-read. Never in the
// per-frame path. Pruned on windowDeleted (same contract as glowfocus).
// GUI-thread only, like every effect-path helper here.
#ifndef GLOWLABEL_H
#define GLOWLABEL_H

#include <QColor>

namespace KWin { class EffectWindow; }

namespace GlowLabel {

// The window's VM label color, or an invalid QColor when the window has
// none (dom0-native, non-Qubes) — callers fall back to configured colors.
QColor colorFor(KWin::EffectWindow *w);

// Drop the destroyed window's cache entry (keeps the map dangling-free).
void pruneWindow(KWin::EffectWindow *w);

}  // namespace GlowLabel

#endif  // GLOWLABEL_H
