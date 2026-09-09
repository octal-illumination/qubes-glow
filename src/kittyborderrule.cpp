// kittyborderrule.cpp — content-based kitty window-rule toggle in kwinrulesrc.
// Note: This code is purely AI-generated.
//
// Why not match the group name: KWin re-saves kwinrulesrc with numeric group
// names ([1], [2], ...) whenever it rewrites the RuleBook, which silently
// breaks name-based lookups — the regression that killed Meta+Shift+B twice
// (LL-007 family). We match rule CONTENT instead and keep whatever name KWin
// chose; an inert kitty rule (present on disk but absent from the active
// [General] rules= list) is re-activated in place.
//
// [General] rules= is a comma-separated KConfig string list naming the active
// rule groups. The bookkeeping "count=" key is intentionally left untouched:
// this KWin build's loader keys off rules= / groupList(), and the working
// pre-reboot file had count=1 alongside a single named group (verified E2E
// on dom0, Sep 09).
#include "kittyborderrule.h"

#include <KConfigGroup>
#include <KSharedConfig>
#include <QStringList>

namespace {

constexpr auto RULES_DESCRIPTION = "kitty borderless"; // rule Description value
constexpr auto RULES_WMCLASS = "kitty";               // window class we match

bool isKittyRule(const KConfigGroup &g) {
    return g.readEntry("Description", QString()) == QLatin1String(RULES_DESCRIPTION)
        || g.readEntry("wmclass", QString()) == QLatin1String(RULES_WMCLASS);
}

} // namespace

namespace KittyBorderRule {

std::optional<bool> toggleKittyNoBorder() {
    auto cfg = KSharedConfig::openConfig(QStringLiteral("kwinrulesrc"));
    KConfigGroup general(cfg, QStringLiteral("General"));
    const QStringList active = general.readEntry("rules", QStringList());

    // 1) Active kitty rule — the normal path.
    for (const QString &name : active) {
        KConfigGroup g(cfg, name);
        if (!isKittyRule(g)) continue;
        const bool next = !g.readEntry("noborder", true);
        g.writeEntry("noborder", next);
        cfg->sync();
        return next;
    }

    // 2) Self-heal: a kitty rule exists but is inert (not listed in rules=)
    //    — re-activate it and toggle.
    const QStringList groups = cfg->groupList();
    for (const QString &name : groups) {
        if (name == QLatin1String("General")) continue;
        KConfigGroup g(cfg, name);
        if (!isKittyRule(g) || !g.hasKey("noborder")) continue;
        QStringList reactivated = active;
        if (!reactivated.contains(name)) reactivated.append(name);
        general.writeEntry("rules", reactivated);
        const bool next = !g.readEntry("noborder", true);
        g.writeEntry("noborder", next);
        cfg->sync();
        return next;
    }

    return std::nullopt; // no kitty rule anywhere — caller no-ops
}

} // namespace KittyBorderRule
