#!/usr/bin/env bash
# Note: This code is purely AI-generated.
#
# kitty-glow regression assertion registry (QubesOS AGENTS Rule 20; kept
# in-repo per Rule 21 isolation, user decision 2026-09-10 — option (a)).
# Every fix records one machine-verifiable invariant. Run before marking any
# task complete:  bash scripts/regression-checks.sh
set -u
cd "$(cd "$(dirname "$0")/.." && pwd)" || exit 1
SRC=src/kittyglow.cpp
FAILS=0
NAMES=""

check() { # check <name> <command...>
  local name="$1"; shift
  case " $NAMES " in *" $name "*) echo "FAIL duplicate-check-name: $name"; FAILS=$((FAILS+1)); return 0;; esac
  NAMES="$NAMES $name"
  if "$@" >/dev/null 2>&1; then
    echo "ok   $name"
  else
    echo "FAIL $name"
    FAILS=$((FAILS+1))
  fi
}

check_not() { # check_not <name> <command...> — pass when command FAILS
  local name="$1"; shift
  case " $NAMES " in *" $name "*) echo "FAIL duplicate-check-name: $name"; FAILS=$((FAILS+1)); return 0;; esac
  NAMES="$NAMES $name"
  if "$@" >/dev/null 2>&1; then
    echo "FAIL $name"
    FAILS=$((FAILS+1))
  else
    echo "ok   $name"
  fi
}

# LL-020 (2026-09-10): scene region is frame-only and its hw-clipping boxes
# are degenerate — the halo clip source must be the halo rect minus
# occluders, drawn as unclipped 1-arg render sub-quads.
check ll020-clip-source-is-halo-rect   grep -qF 'QRegion clip = halo.toRect();' "$SRC"
check_not ll020-no-3arg-hwclipping-render  grep -qE 'render\([^)]*GL_TRIANGLES, true\)' "$SRC"
check_not ll020-no-scissor-enable          grep -qF 'glEnable(GL_SCISSOR_TEST)' "$SRC"
check ll020-unclipped-1arg-render      grep -qF 'vb->render(GL_TRIANGLES);' "$SRC"
check ll020-desktop-skip-present       grep -qF 'if (w->isDesktop()) continue;' "$SRC"

# LL-017 superseded by seamless pass-behind (2026-09-10): docks keep
# expandedGeometry; normal occluders clip at frameGeometry so the occluder's
# shadow gradient dims the halo right up to its border (no wallpaper gap).
check ll017seamless-dock-expanded      grep -qF 'w->isDock() ? w->expandedGeometry()' "$SRC"
check ll017seamless-normal-frame       grep -qF ': w->frameGeometry();' "$SRC"

# LL-016 (2026-09-09): kwinrulesrc must never carry a kitty noborder rule
# (write-revert fight with the kglowsync script); state lives in kittyglowrc.
check_not ll016-no-noborderrule-in-src     grep -rqF 'noborderrule' src/

# LL-023 (2026-09-10): all-windows glow — eligibility is a deny-list in
# glowtargets.h (dialogs, notifications/OSD, splash, tooltip, popup, utility,
# desktop, docks), plasma/Qubes-tray/krunner chrome excluded by class
# (LL-026), Meta+Shift+G is the persisted glow master switch, and
# Meta+Shift+B is the class-wide borderless toggle staged via requestApply.
check ll023-dialogs-notifications-excluded grep -qF 'isDialog() || w->isNotification()' src/glowtargets.h
check ll023-glow-gate-paint-path        bash -c '[ $(grep -cF "GlowFocus::glowAllowed(w)" src/kittyglow.cpp) -ge 2 ]'

# LL-026 (2026-09-11): qubes-gui strips _NET_WM_WINDOW_TYPE — chrome must
# be excluded by WM_CLASS (plasma surfaces, Qubes tray-widget ghosts,
# xembedsniproxy, krunner), and the shortcuts split: B = class-wide
# borderless (persist + stage via requestApply), G = glow master switch.
check ll026-chrome-class-exclusion bash -c 'grep -qF "plasmashell" src/glowtargets.h && grep -qF "qui-" src/glowtargets.h && grep -qF "xembedsniproxy" src/glowtargets.h && grep -qF "krunner" src/glowtargets.h'
check ll026-min-frame-size-guard    bash -c 'grep -qF "fg.width() < 48 || fg.height() < 48" src/glowtargets.h && grep -qF "fg.width < 48 || fg.height < 48" src/kwin-script/kglowsync/contents/code/main.js'
check ll025-bootstrap-watchdog      bash -c 'grep -qF "booted" src/kwin-script/kglowsync/contents/code/main.js && grep -qF "var bootWatch" src/kwin-script/kglowsync/contents/code/main.js'
check ll027-focused-routing         bash -c 'grep -qF "toggleBorderlessFocused" src/kittyglow.cpp && grep -qF "toggleGlowFocused" src/kittyglow.cpp && grep -qF "KittyGlowEffect::toggleBorderlessFocused" src/kittyglow.cpp && grep -qF "nextWindowOp" src/kittytoggle.cpp'
check ll027-global-masters          bash -c 'grep -qF "Toggle Borders All Windows" src/kittyglow.cpp && grep -qF "Toggle Glow All Windows" src/kittyglow.cpp && grep -qF "toggleGlowGlobal" src/kittyglow.cpp'
check ll027-override-protection     bash -c 'grep -qF "var overrides = {}" src/kwin-script/kglowsync/contents/code/main.js && grep -qF "id in overrides" src/kwin-script/kglowsync/contents/code/main.js && grep -qF "focusedBorderFlip" src/kwin-script/kglowsync/contents/code/main.js && grep -qF "GlowFocus::pruneWindow" src/kittyglow.cpp'
# LL-028 (rewritten by re-audit 2, 2026-09-11): effect-facing regions are
# LOGICAL in KWin 5.27.8 — the ONLY scale boundary is the vertex upload.
# Widening, occluders, and translation must NOT carry renderTargetScale,
# and the vertex upload must keep its * s mapping.
check ll028-logical-space-rule      bash -c 'grep -qF "const int e = m_cfg.maxExtent();" src/kittyglow.cpp && ! grep -qF "maxExtent() * KWin::effects->renderTargetScale()" src/kittyglow.cpp && ! grep -qF "gf.x() * scale" src/kittyglow.cpp && ! grep -qF "data.xTranslation() * s" src/kittyglow.cpp && grep -qF "static_cast<float>(cr.x() * s)" src/kittyglow.cpp'
check ll032-label-hue                bash -c 'grep -qF "_QUBES_LABEL_COLOR" src/glowlabel.cpp && grep -qF "GlowLabel::colorFor(w)" src/kittyglow.cpp && grep -qF "m_cfg.labelColor" src/kittyglow.cpp && grep -qF "GlowLabel::pruneWindow(w)" src/kittyglow.cpp && grep -qF "labelColor = g.readEntry(\"LabelColor\", true)" src/glowconfig.h && grep -qF "glowlabel.cpp" src/CMakeLists.txt'
check ll029-per-action-gates        bash -c 'grep -qF "m_gateBorderFocused" src/kittyglow.cpp && grep -qF "m_gateGlowFocused" src/kittyglow.cpp && grep -qF "m_gateBorderGlobal" src/kittyglow.cpp && grep -qF "m_gateGlowGlobal" src/kittyglow.cpp && ! grep -qF "m_toggleGate" src/kittyglow.cpp'
check audit-m1-v2-script-purged     bash -c '[ ! -f scripts/v2-rollout-round.sh ]'
check m5-rollback-strips-kglowsync  bash -c 'grep -qF "unloadScript string:kglowsync" scripts/kittyglow-rollback.sh && grep -qF "kglowsyncEnabled" scripts/kittyglow-rollback.sh && grep -qF "scripts/kglowsync" scripts/kittyglow-rollback.sh'
check ll026-b-stages-borderless    bash -c 'grep -qF "void KittyGlowEffect::toggleBorderlessGlobal()" src/kittyglow.cpp && grep -qF "KittyToggle::requestApply(next)" src/kittyglow.cpp && grep -qF "KittyGlowState::toggleNoBorder()" src/kittyglow.cpp'
check ll026-g-glow-master-switch   bash -c 'grep -qF "Qt::META | Qt::SHIFT | Qt::Key_G" src/kittyglow.cpp && grep -qF "connect(g, &QAction::triggered, this, &KittyGlowEffect::toggleGlowFocused);" src/kittyglow.cpp && grep -qF "connect(b, &QAction::triggered, this, &KittyGlowEffect::toggleBorderlessFocused);" src/kittyglow.cpp && grep -qF "connect(gAll, &QAction::triggered, this, &KittyGlowEffect::toggleGlowGlobal);" src/kittyglow.cpp && grep -qF "connect(bAll, &QAction::triggered, this, &KittyGlowEffect::toggleBorderlessGlobal);" src/kittyglow.cpp'
check ll026-script-app-windows     bash -c 'grep -qF "isBorderlessTarget" src/kwin-script/kglowsync/contents/code/main.js && grep -qF "appWindows()" src/kwin-script/kglowsync/contents/code/main.js && ! grep -qF "isKitty" src/kwin-script/kglowsync/contents/code/main.js'

# LL-025 (2026-09-11): kwin --replace bus-name race — kglowsync must not die
# at load when org.kde.kittyglow is still held by the old kwin instance.
# The script's bootstrap retries (bootTries loop) and slog must swallow
# callDBus failures instead of throwing at script evaluation time.
check ll025-kglowsync-resilient-bootstrap bash -c 'grep -qF "bootTries" src/kwin-script/kglowsync/contents/code/main.js && grep -qF "catch (e) {}" src/kwin-script/kglowsync/contents/code/main.js'

echo "---"
if [ "$FAILS" -eq 0 ]; then
  echo "ALL CHECKS PASSED"
else
  echo "$FAILS CHECK(S) FAILED"
  exit 1
fi
