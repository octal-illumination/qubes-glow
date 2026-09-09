// kittyborderrule.h — kitty window-rule lookup & toggle in kwinrulesrc.
// Note: This code is purely AI-generated.
//
// Single responsibility: find the ACTIVE kitty-borderless window rule in
// kwinrulesrc by CONTENT (Description / wmclass), never by KConfig group
// name, and flip its "noborder" entry. KWin renames rule groups on re-save
// (numeric [1], [2], ...), so name-based lookups rot (LL-007/LL-011).
#ifndef KITTYBORDERRULE_H
#define KITTYBORDERRULE_H

#include <optional>

namespace KittyBorderRule {

// Toggles "noborder" on the active kitty window rule. If no ACTIVE group
// matches, the first kitty rule group carrying a "noborder" entry is
// re-activated (added to the [General] rules= list) and toggled. Writes and
// syncs kwinrulesrc on success; leaves the file untouched when no kitty
// rule exists at all.
//
// Returns: the new noborder value (true = borderless, false = kitty shows
//          its border), or std::nullopt when no kitty rule was found.
std::optional<bool> toggleKittyNoBorder();

} // namespace KittyBorderRule

#endif // KITTYBORDERRULE_H
