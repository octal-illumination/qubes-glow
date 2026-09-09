# Session State — kitty-glow (pi session, new)
2026-09-09T16:55:43Z

## Current Objective
LL-018 (translucent panel halo penetration) — fix shipped; awaiting visual acceptance.

## Discovered Facts
- Build #9 sha256 e7ff8627… deployed to dom0, byte-verified; loaded in kwin PID 32660 (5 /proc/maps mappings).
- KWin 5.27.8 DBus org.kde.kwin.Effects has NO loadedEffects/isLoaded methods — use /proc/PID/maps grep as load proof.
- dom0 /usr/share/kwin/effects/kittyglow/ was root:700 (chenpan unreadable) → fixed 755; audit-logged.
- dom0 bridge failures: password dialog Esc → 'Cancelled.'; expiry → 'Access denied.'. Retry pattern works; do not spam dialogs.
- kglowprobe tooling untracked at src/kwin-script/kglowprobe/ + logs/probe/ evidence (1450 px panel gold baseline bug, 48 px clean baseline).

## File Changes (this session)
- src/kittyglow.cpp: LL-018 dock/panel occluder fix (committed 64e6b36, 06309f2 follow-ups).
- HANDBOOK.md, PROJECT_CONTEXT.md (+.html): synced; logs/* ledgers updated; SESSION_STATE (this file).

## Pending Work
1. Visual panel test (maximize kitty over panel, nudge → no gold on panel).
2. Optional: re-run kglowprobe synthetic panel check (expect ≡48 px baseline).
3. Decide: commit probe tooling / gitignore logs/probe/ (Rule 22f).

## Next Agent Handoff
Read PROJECT_CONTEXT.md Last Updated. If user reports clean panel → mark LL-018 visual-verified in PROJECT_CONTEXT + CHANGELOG. If gold persists → check w->isDock() API availability at build (KWin 5.27 EffectWindow) and kwin journal.
