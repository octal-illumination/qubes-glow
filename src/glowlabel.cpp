// glowlabel.cpp — per-VM Qubes label color lookup (build #21, LL-032).
// Note: This code is purely AI-generated.
#include "glowlabel.h"

#include <kwineffects.h>
#include <QColor>
#include <QHash>
#include <QtGlobal>
#include <xcb/xcb.h>
#include <cstdlib>
#include <cstring>

namespace {

// Cache entry: bit 31 = "has label color"; low 24 bits = 0x00RRGGBB.
// 0 (no bit 31) caches the MISS — dom0-native windows never re-read.
QHash<const KWin::EffectWindow *, quint32> s_cache;

bool s_atomTried = false;
xcb_atom_t s_atom = XCB_ATOM_NONE;

QColor decode(quint32 entry) {
    if (!(entry & 0x80000000u)) return QColor();  // cached miss
    const quint32 rgb = entry & 0x00FFFFFFu;
    return QColor((rgb >> 16) & 0xFFu, (rgb >> 8) & 0xFFu, rgb & 0xFFu);
}

// Interned once per kwin process; never re-queried after the first try.
xcb_atom_t labelColorAtom() {
    if (!s_atomTried) {
        s_atomTried = true;
        if (xcb_connection_t *c = KWin::effects->xcbConnection()) {
            const char name[] = "_QUBES_LABEL_COLOR";
            xcb_intern_atom_cookie_t ck =
                xcb_intern_atom(c, 0, sizeof(name) - 1, name);
            if (xcb_intern_atom_reply_t *rep =
                    xcb_intern_atom_reply(c, ck, nullptr)) {
                s_atom = rep->atom;
                free(rep);
            }
        }
    }
    return s_atom;
}

}  // namespace

namespace GlowLabel {

QColor colorFor(KWin::EffectWindow *w) {
    if (!w) return QColor();
    const auto it = s_cache.constFind(w);
    if (it != s_cache.cend()) return decode(it.value());

    quint32 entry = 0;  // cached miss until proven otherwise
    xcb_connection_t *c = KWin::effects->xcbConnection();
    const xcb_atom_t atom = labelColorAtom();
    if (c && atom != XCB_ATOM_NONE) {
        // windowId() is the X11 WId on X11 (kwineffects.h:2684); the atom
        // is CARDINAL/32 so one 32-bit word is the whole value.
        xcb_get_property_cookie_t ck = xcb_get_property(
            c, false, static_cast<xcb_window_t>(w->windowId()), atom,
            XCB_ATOM_CARDINAL, 0, 1);
        if (xcb_get_property_reply_t *rep =
                xcb_get_property_reply(c, ck, nullptr)) {
            if (rep->type == XCB_ATOM_CARDINAL
                && xcb_get_property_value_length(rep) >= 4) {
                const quint32 v =
                    *static_cast<const quint32 *>(xcb_get_property_value(rep));
                // Mask to 0x00RRGGBB — qubes-guid writes plain RGB; any
                // stray high bits (alpha-ish encodings) never leak in.
                entry = 0x80000000u | (v & 0x00FFFFFFu);
            }
            free(rep);
        }
    }
    s_cache.insert(w, entry);
    if (entry & 0x80000000u) {
        // One trace per window lifetime — verification and debugging
        // without per-frame journal noise.
        qWarning().noquote() << "kittyglow: label hue"
                             << w->windowClass() << "=>"
                             << QColor((entry >> 16) & 0xFFu,
                                       (entry >> 8) & 0xFFu,
                                       entry & 0xFFu).name();
    }
    return decode(entry);
}

void pruneWindow(KWin::EffectWindow *w) {
    s_cache.remove(w);
}

}  // namespace GlowLabel
