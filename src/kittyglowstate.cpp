// kittyglowstate.cpp — persistent state store for kittyglow.
// Note: This code is purely AI-generated.
//
// kittyglowrc layout (~/.config/kittyglowrc):
//   [General]
//   noBorder=true|false      (global border default; true = borderless)
//   glowEnabled=true|false   (global glow master; default true)
//
// Written by the C++ effect's GLOBAL toggle paths (Meta+Shift+Alt+B/G);
// noBorder is read by kittytoggle.cpp's getCurrentState() (kglowsync
// bootstrap) after any kwin restart. Per-window overrides are runtime-only
// and never persisted here (build #18 — see SPECIFICATION LL-027).
// See kittyglowstate.h for why kwinrulesrc is out.
#include "kittyglowstate.h"

#include <KConfigGroup>
#include <KSharedConfig>

namespace KittyGlowState {

namespace {

constexpr auto CONFIG_FILE = "kittyglowrc";
constexpr auto GROUP_NAME = "General";
constexpr auto KEY_NAME = "noBorder";

KConfigGroup stateGroup() {
    // QStringLiteral needs a raw literal (token-pasting macro) — pass the
    // strings inline rather than through the constexpr constants.
    auto cfg = KSharedConfig::openConfig(QStringLiteral("kittyglowrc"));
    return KConfigGroup(cfg, QStringLiteral("General"));
}

} // namespace

bool loadNoBorder() {
    return stateGroup().readEntry(KEY_NAME, true);
}

void saveNoBorder(bool noBorder) {
    KConfigGroup g = stateGroup();
    g.writeEntry(KEY_NAME, noBorder);
    g.sync();
}

bool toggleNoBorder() {
    const bool next = !loadNoBorder();
    saveNoBorder(next);
    return next;
}

// --- Glow master switch -------------------------------------------------

bool loadGlowEnabled() {
    return stateGroup().readEntry(QStringLiteral("glowEnabled"), true);
}

void saveGlowEnabled(bool enabled) {
    KConfigGroup g = stateGroup();
    g.writeEntry(QStringLiteral("glowEnabled"), enabled);
    g.sync();
}

bool toggleGlowEnabled() {
    const bool next = !loadGlowEnabled();
    saveGlowEnabled(next);
    return next;
}

} // namespace KittyGlowState
