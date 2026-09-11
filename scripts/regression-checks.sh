#!/usr/bin/env bash
# Note: This code is purely AI-generated.
#
# kitty-glow regression assertion registry (QubesOS AGENTS Rule 20; kept
# in-repo per Rule 21 isolation, user decision 2026-09-10 — option (a)).
# Every fix records one machine-verifiable invariant. Run before marking any
# task complete:  bash scripts/regression-checks.sh
set -u
cd "$(cd "$(dirname "$0")/.." && pwd)"
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
# desktop, docks), and Meta+Shift+B is the persisted glow master switch
# gated per paint in both paint path entry points.
check ll023-dialogs-notifications-excluded grep -qF 'isDialog() || w->isNotification()' src/glowtargets.h
check ll023-glow-gate-paint-path        bash -c '[ $(grep -cF "if (!m_glowEnabled || !KittyGlowTargets::isGlowWindow(w)) return;" src/kittyglow.cpp) -ge 2 ]'

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
