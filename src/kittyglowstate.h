// kittyglowstate.h — persistent borderless-state store for kittyglow.
// Note: This code is purely AI-generated.
//
// Single responsibility: own the kitty borderless state in kittyglow's own
// config file (~/.config/kittyglowrc [General] noBorder). kwinrulesrc no
// longer participates: KWin's rules engine lets a loaded forcing rule
// override scripting noBorder writes (rules()->checkNoBorder() precedence),
// which is exactly the write-revert fight observed live on 2026-09-09.
// Our own file is read/written only by us — no reparse races, no
// group-rename hazards, no reconfigure needed.
#ifndef KITTYGLOWSTATE_H
#define KITTYGLOWSTATE_H

namespace KittyGlowState {

// Current persisted state. true = borderless (the feature's default and
// the value used when no state has ever been saved).
bool loadNoBorder();

// Persists the state immediately (cfg->sync()) so a kwin restart or a
// fresh kglowsync bootstrap sees the new value.
void saveNoBorder(bool noBorder);

// Convenience: load, flip, save, return the new value.
bool toggleNoBorder();

} // namespace KittyGlowState

#endif // KITTYGLOWSTATE_H
