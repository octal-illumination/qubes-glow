// glowfocus.cpp — per-window glow override set (build #18, v3.10).
// Note: This code is purely AI-generated.
//
// QSet of EffectWindow pointers pruned on windowDeleted (see glowfocus.h).
// Static storage: the effect is a kwin-load-order singleton and all access
// runs in kwin's GUI thread — no locking needed.
#include "glowfocus.h"

#include <kwineffects.h>
#include <QSet>

namespace {
QSet<KWin::EffectWindow *> s_glowOff;
}

namespace GlowFocus {

bool toggleGlow(KWin::EffectWindow *w) {
    if (!w) return false;
    if (s_glowOff.contains(w)) {
        s_glowOff.remove(w);
        return true;   // override cleared -> glow back on for w
    }
    s_glowOff.insert(w);
    return false;      // override set -> glow off for w
}

bool glowAllowed(KWin::EffectWindow *w) {
    return !s_glowOff.contains(w);
}

void pruneWindow(KWin::EffectWindow *w) {
    s_glowOff.remove(w);
}

void clear() {
    s_glowOff.clear();
}

}  // namespace GlowFocus
