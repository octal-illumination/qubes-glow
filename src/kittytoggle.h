// kittytoggle.h — seamless Meta+Shift+B plumbing (DBus pull-service).
// Note: This code is purely AI-generated.
//
// Single responsibility: stage borderless toggle values from the effect and
// answer the kglowsync KWin script's nextSource() polls; verify/enable the
// script package. The script owns the live noBorder application (KWin
// scripting can write Client.noBorder directly; the C++ Effect API cannot).
#ifndef KITTYTOGGLE_H
#define KITTYTOGGLE_H

namespace KittyToggle {

// Registers org.kde.kittyglow /sync (nextSource) on the session bus and makes
// sure the kglowsync script package is enabled in kwinrc. Call once, from the
// effect constructor.
void init();

// Stage the new kitty noBorder value; consumed by the script's next poll
// (60 ms worst-case latency). Safe to call repeatedly; last value wins.
void requestApply(bool noBorder);

}  // namespace KittyToggle

#endif  // KITTYTOGGLE_H
