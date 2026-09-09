// kittyglowstate.cpp — persistent borderless-state store for kittyglow.
// Note: This code is purely AI-generated.
//
// kittyglowrc layout (~/.config/kittyglowrc):
//   [General]
//   noBorder=true|false
//
// Written by the C++ effect's Meta+Shift+B toggle path; read by
// kittytoggle.cpp's getCurrentState() (kglowsync bootstrap) and after any
// kwin restart. See kittyglowstate.h for why kwinrulesrc is out.
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

} // namespace KittyGlowState
