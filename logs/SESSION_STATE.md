# SESSION_STATE.md — kitty-glow
- Timestamp: 2026-09-11T10:47:10+05:30
- Objective: RESOLVED — build #18 (v3.10) shipped, user-accepted. Session
  started as handoff from 2M-token session 01a077e8; git history verified.
- Key facts: kwin 62946 running build #18 sha 7df23937; dom0 access =
  ~/.local/bin/dom0 (pre-existing qrexec dom0.AuthExec bridge, NOT a repo
  file; password dialog per call); user = chenpan; xauth=/tmp/xauth_PCByVw,
  display :0; kittyglowrc stores global noBorder+glowEnabled (kwinrulesrc
  rule retired + verified absent). Effect shortcuts register under
  kglobalaccel /component/kwin. Synthetic xdotool keys can miss live
  bindings post --replace (LL-027) — test shortcuts physically.
- Build #18 semantics: Meta+Shift+B/G = FOCUSED-window border/glow toggle
  (runtime-only); Meta+Shift+Alt+B/G = global masters (persisted; global
  glow resets per-window overrides; Alt+B sweep re-imposes default).
  Script owns noBorder writes; overrides map (windowId->bool) shields
  per-window flips from the 400 ms sweep. Glow overrides = effect-side
  pointer set pruned on windowDeleted (glowfocus.h).
- Verified live: focused-op journal lines (konsole+kitty, both directions,
  sweep-safe), focused G scope-named, Alt-masters sweep, physical-key
  acceptance by user ("everything works").
- 20/20 regression assertions; docs synced (SPEC LL-027, HANDBOOK 6b,
  PROJECT_CONTEXT); HTML regenerated; all committed.
- Next agent: read PROJECT_CONTEXT.md + SPECIFICATION.md (LL-026/LL-027);
  probes in logs/probe/ (ll018_verify.py = full behavioral suite).
